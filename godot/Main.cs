using System;
using System.Collections.Generic;
using System.Text;
using Godot;

namespace HiveWeGodot;

public partial class Main : Node3D
{
    private Label label = null!;

    public override void _Ready()
    {
        label = new Label
        {
            Position = new Vector2(24, 24),
            Size = new Vector2(900, 400),
        };
        var layer = new CanvasLayer();
        AddChild(layer);
        layer.AddChild(label);

        string projectRoot = ProjectSettings.GlobalizePath("res://");
        string mapPath = projectRoot + "../testdata/maps/(2)BootyBay.w3m";
        string dataDir = projectRoot + "../testdata/warcraft";

        if (!HiveCore.Open(mapPath, dataDir, out string error))
        {
            GD.PrintErr(error);
            label.Text = "打开失败：" + error;
            return;
        }

        HiveMapInfo info = HiveCore.MapInfo;
        float[] heights = HiveCore.Heights();
        HiveTile[] tiles = HiveCore.Tiles();

        label.Text = Describe(info, heights, tiles);

        BuildTerrain(info, heights, tiles);
        BuildCamera(info);
    }

    private static string Describe(HiveMapInfo info, float[] heights, HiveTile[] tiles)
    {
        StringBuilder report = new();
        report.AppendLine($"{info.Width}×{info.Height} 顶点，{info.TilesX}×{info.TilesY} 格");
        report.AppendLine($"纹理 {info.TextureCount}，装饰物 {info.DoodadCount}，单位 {info.UnitCount}，区域 {info.RegionCount}");

        float lowest = heights[0];
        float highest = heights[0];
        foreach (float height in heights)
        {
            lowest = Math.Min(lowest, height);
            highest = Math.Max(highest, height);
        }
        report.AppendLine($"高度 {lowest:F2} ~ {highest:F2}，格子 [0] 变体 {tiles[0].Variation}");
        return report.ToString();
    }

    private void BuildTerrain(HiveMapInfo info, float[] heights, HiveTile[] tiles)
    {
        byte[] heightBytes = new byte[heights.Length * 4];
        Buffer.BlockCopy(heights, 0, heightBytes, 0, heightBytes.Length);
        Image heightImage = Image.CreateFromData(info.Width, info.Height, false, Image.Format.Rf, heightBytes);
        ImageTexture heightTexture = ImageTexture.CreateFromImage(heightImage);

        var layerImages = new Godot.Collections.Array<Image>();
        int[] layerOffset = new int[info.TextureCount];
        int totalLayers = 0;
        int tileSize = 0;

        for (int i = 0; i < info.TextureCount; i++)
        {
            HiveCore.TextureInfo(i, out int size, out int layers);
            tileSize = Math.Max(tileSize, size);
            layerOffset[i] = totalLayers;
            totalLayers += layers;
        }

        GD.Print($"纹理数组：{totalLayers} 层，单层 {tileSize}×{tileSize}");

        for (int i = 0; i < info.TextureCount; i++)
        {
            HiveCore.TextureInfo(i, out int size, out int layers);
            byte[] pixels = HiveCore.TexturePixels(i, size, layers);
            int layerBytes = size * size * 4;

            for (int layer = 0; layer < layers; layer++)
            {
                byte[] slice = new byte[layerBytes];
                Buffer.BlockCopy(pixels, layer * layerBytes, slice, 0, layerBytes);
                Image layerImage = Image.CreateFromData(size, size, false, Image.Format.Rgba8, slice);

                if (size != tileSize)
                {
                    layerImage.Resize(tileSize, tileSize, Image.Interpolation.Bilinear);
                }
                layerImages.Add(layerImage);
            }
        }

        byte[] blank = new byte[tileSize * tileSize * 4];
        layerImages.Add(Image.CreateFromData(tileSize, tileSize, false, Image.Format.Rgba8, blank));

        var textureArray = new Texture2DArray();
        Error arrayError = textureArray.CreateFromImages(layerImages);
        if (arrayError != Error.Ok)
        {
            GD.PrintErr($"纹理数组创建失败：{arrayError}");
            return;
        }

        ArrayMesh quad = BuildQuad();
        var multiMesh = new MultiMesh
        {
            TransformFormat = MultiMesh.TransformFormatEnum.Transform3D,
            UseCustomData = true,
            Mesh = quad,
            InstanceCount = info.TilesX * info.TilesY,
        };

        for (int y = 0; y < info.TilesY; y++)
        {
            for (int x = 0; x < info.TilesX; x++)
            {
                int index = y * info.TilesX + x;
                multiMesh.SetInstanceTransform(index, new Transform3D(Basis.Identity, new Vector3(x, 0, y)));

                HiveTile tile = tiles[index];
                multiMesh.SetInstanceCustomData(index, new Color(
                    PackLayer(tile.Tex0, layerOffset, info.TextureCount, totalLayers),
                    PackLayer(tile.Tex1, layerOffset, info.TextureCount, totalLayers),
                    PackLayer(tile.Tex2, layerOffset, info.TextureCount, totalLayers),
                    PackLayer(tile.Tex3, layerOffset, info.TextureCount, totalLayers)));
            }
        }

        var material = new ShaderMaterial
        {
            Shader = GD.Load<Shader>("res://Shaders/terrain.gdshader"),
        };
        material.SetShaderParameter("ground_tex", textureArray);
        material.SetShaderParameter("height_map", heightTexture);
        material.SetShaderParameter("vertex_size", new Vector2(info.Width, info.Height));

        var terrain = new MultiMeshInstance3D
        {
            Multimesh = multiMesh,
            MaterialOverride = material,
            Name = "Terrain",
        };
        AddChild(terrain);

        var light = new DirectionalLight3D
        {
            RotationDegrees = new Vector3(-55, -35, 0),
            LightEnergy = 1.6f,
        };
        AddChild(light);

        var environment = new Godot.Environment
        {
            BackgroundMode = Godot.Environment.BGMode.Color,
            BackgroundColor = new Color(0.12f, 0.13f, 0.15f),
            AmbientLightSource = Godot.Environment.AmbientSource.Color,
            AmbientLightColor = new Color(0.55f, 0.57f, 0.62f),
            AmbientLightEnergy = 0.9f,
        };
        AddChild(new WorldEnvironment { Environment = environment });
    }

    private static float PackLayer(uint packed, int[] layerOffset, int textureCount, int blankLayer)
    {
        int index = (int)(packed & 0xFFFF);
        if (index == 0xFFFF || index >= textureCount)
        {
            return blankLayer;
        }

        int variation = (int)(packed >> 16);
        return layerOffset[index] + variation;
    }

    private static ArrayMesh BuildQuad()
    {
        var arrays = new Godot.Collections.Array();
        arrays.Resize((int)Mesh.ArrayType.Max);
        arrays[(int)Mesh.ArrayType.Vertex] = new[]
        {
            new Vector3(0, 0, 0),
            new Vector3(1, 0, 0),
            new Vector3(0, 0, 1),
            new Vector3(1, 0, 1),
        };
        arrays[(int)Mesh.ArrayType.Normal] = new[]
        {
            Vector3.Up, Vector3.Up, Vector3.Up, Vector3.Up,
        };
        arrays[(int)Mesh.ArrayType.TexUV] = new[]
        {
            new Vector2(0, 0),
            new Vector2(1, 0),
            new Vector2(0, 1),
            new Vector2(1, 1),
        };
        arrays[(int)Mesh.ArrayType.Index] = new[] { 0, 2, 1, 1, 2, 3 };

        var mesh = new ArrayMesh();
        mesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Triangles, arrays);
        return mesh;
    }

    private void BuildCamera(HiveMapInfo info)
    {
        var camera = new Camera3D
        {
            Position = new Vector3(info.TilesX * 0.5f, info.TilesY * 0.9f, info.TilesY * 0.85f),
            Fov = 60,
        };
        AddChild(camera);
        camera.LookAt(new Vector3(info.TilesX * 0.5f, 0, info.TilesY * 0.5f), Vector3.Up);
    }

    private int frames;
    private readonly string screenshotPath = OS.GetEnvironment("HIVE_SCREENSHOT");

    public override void _Process(double delta)
    {
        if (screenshotPath.Length == 0)
        {
            return;
        }

        frames++;
        if (frames == 12)
        {
            Image shot = GetViewport().GetTexture().GetImage();
            shot.SavePng(screenshotPath);
            GD.Print($"截图已保存：{screenshotPath}");
            GetTree().Quit();
        }
    }

    public override void _ExitTree()
    {
        HiveCore.Close();
    }
}
