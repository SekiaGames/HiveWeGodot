using System;
using System.Text;
using Godot;

namespace HiveWeGodot;

public partial class Main : Node3D
{
    private enum Tool
    {
        Texture,
        Height,
        Water,
        Cliff,
    }

    private const float Pitch = 0.95f;

    private HiveMapInfo info;
    private float[] heights = Array.Empty<float>();
    private HiveTile[] tiles = Array.Empty<HiveTile>();
    private float[] waterHeights = Array.Empty<float>();
    private byte[] waterExists = Array.Empty<byte>();
    private float waterOffset;
    private int[] layerOffset = Array.Empty<int>();
    private int totalLayers;
    private int tileSize;

    private Camera3D camera = null!;
    private MultiMesh multiMesh = null!;
    private MultiMesh waterMesh = null!;
    private ImageTexture heightTexture = null!;
    private MeshInstance3D brushPreview = null!;
    private Label label = null!;

    private Vector3 focus;
    private float distance = 260f;
    private int brushSize = 8;
    private Tool tool = Tool.Texture;
    private int textureIndex;
    private bool painting;
    private bool panning;

    public override void _Ready()
    {
        label = new Label
        {
            Position = new Vector2(24, 24),
            Size = new Vector2(1000, 400),
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

        info = HiveCore.MapInfo;
        heights = HiveCore.Heights();
        tiles = HiveCore.Tiles();
        waterHeights = HiveCore.WaterHeights();
        waterExists = HiveCore.WaterExists();
        waterOffset = HiveCore.WaterOffset();

        BuildTerrain();
        BuildWater();
        BuildCamera();
        BuildBrushPreview();
        ApplyTool();
        UpdateLabel();
    }

    private void BuildTerrain()
    {
        byte[] heightBytes = new byte[heights.Length * 4];
        Buffer.BlockCopy(heights, 0, heightBytes, 0, heightBytes.Length);
        Image heightImage = Image.CreateFromData(info.Width, info.Height, false, Image.Format.Rf, heightBytes);
        heightTexture = ImageTexture.CreateFromImage(heightImage);

        var layerImages = new Godot.Collections.Array<Image>();
        layerOffset = new int[info.TextureCount];
        totalLayers = 0;

        for (int i = 0; i < info.TextureCount; i++)
        {
            HiveCore.TextureInfo(i, out int size, out int layers);
            tileSize = Math.Max(tileSize, size);
            layerOffset[i] = totalLayers;
            totalLayers += layers;
        }

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

        layerImages.Add(Image.CreateFromData(tileSize, tileSize, false, Image.Format.Rgba8, new byte[tileSize * tileSize * 4]));

        var textureArray = new Texture2DArray();
        if (textureArray.CreateFromImages(layerImages) != Error.Ok)
        {
            GD.PrintErr("纹理数组创建失败");
            return;
        }

        GD.Print($"纹理数组：{totalLayers} 层，单层 {tileSize}×{tileSize}");

        multiMesh = new MultiMesh
        {
            TransformFormat = MultiMesh.TransformFormatEnum.Transform3D,
            UseCustomData = true,
            Mesh = BuildQuad(),
            InstanceCount = info.TilesX * info.TilesY,
        };

        for (int y = 0; y < info.TilesY; y++)
        {
            for (int x = 0; x < info.TilesX; x++)
            {
                multiMesh.SetInstanceTransform(y * info.TilesX + x, new Transform3D(Basis.Identity, new Vector3(x, 0, y)));
            }
        }
        RefreshTiles();

        var material = new ShaderMaterial
        {
            Shader = GD.Load<Shader>("res://Shaders/terrain.gdshader"),
        };
        material.SetShaderParameter("ground_tex", textureArray);
        material.SetShaderParameter("height_map", heightTexture);
        material.SetShaderParameter("vertex_size", new Vector2(info.Width, info.Height));

        AddChild(new MultiMeshInstance3D
        {
            Multimesh = multiMesh,
            MaterialOverride = material,
            Name = "Terrain",
        });

        AddChild(new DirectionalLight3D
        {
            RotationDegrees = new Vector3(-55, -35, 0),
            LightEnergy = 1.6f,
        });

        AddChild(new WorldEnvironment
        {
            Environment = new Godot.Environment
            {
                BackgroundMode = Godot.Environment.BGMode.Color,
                BackgroundColor = new Color(0.12f, 0.13f, 0.15f),
                AmbientLightSource = Godot.Environment.AmbientSource.Color,
                AmbientLightColor = new Color(0.55f, 0.57f, 0.62f),
                AmbientLightEnergy = 0.9f,
            },
        });
    }

    private void RefreshTiles()
    {
        for (int i = 0; i < tiles.Length; i++)
        {
            HiveTile tile = tiles[i];
            multiMesh.SetInstanceCustomData(i, new Color(
                PackLayer(tile.Tex0),
                PackLayer(tile.Tex1),
                PackLayer(tile.Tex2),
                PackLayer(tile.Tex3)));
        }
    }

    private void BuildWater()
    {
        waterMesh = new MultiMesh
        {
            TransformFormat = MultiMesh.TransformFormatEnum.Transform3D,
            UseCustomData = true,
            Mesh = BuildQuad(),
            InstanceCount = info.TilesX * info.TilesY,
        };

        for (int y = 0; y < info.TilesY; y++)
        {
            for (int x = 0; x < info.TilesX; x++)
            {
                waterMesh.SetInstanceTransform(y * info.TilesX + x, new Transform3D(Basis.Identity, new Vector3(x, 0, y)));
            }
        }

        var material = new ShaderMaterial
        {
            Shader = GD.Load<Shader>("res://Shaders/water.gdshader"),
        };

        AddChild(new MultiMeshInstance3D
        {
            Multimesh = waterMesh,
            MaterialOverride = material,
            Name = "Water",
            CastShadow = GeometryInstance3D.ShadowCastingSetting.Off,
        });

        RefreshWater();
    }

    private void RefreshWater()
    {
        int wet = 0;
        for (int y = 0; y < info.TilesY; y++)
        {
            for (int x = 0; x < info.TilesX; x++)
            {
                int index = y * info.TilesX + x;
                int c00 = y * info.Width + x;
                int c10 = c00 + 1;
                int c01 = c00 + info.Width;
                int c11 = c01 + 1;

                bool covered = waterExists[c00] != 0 && waterExists[c10] != 0
                    && waterExists[c01] != 0 && waterExists[c11] != 0;

                if (!covered)
                {
                    waterMesh.SetInstanceTransform(index, new Transform3D(new Basis(Vector3.Zero, Vector3.Zero, Vector3.Zero), Vector3.Zero));
                    continue;
                }

                wet++;
                waterMesh.SetInstanceTransform(index, new Transform3D(Basis.Identity, new Vector3(x, 0, y)));
                waterMesh.SetInstanceCustomData(index, new Color(
                    waterHeights[c00] + waterOffset,
                    waterHeights[c10] + waterOffset,
                    waterHeights[c01] + waterOffset,
                    waterHeights[c11] + waterOffset));
            }
        }
        GD.Print($"水面 {wet} 格");
    }

    private void RefreshHeights()
    {
        byte[] heightBytes = new byte[heights.Length * 4];
        Buffer.BlockCopy(heights, 0, heightBytes, 0, heightBytes.Length);
        heightTexture.Update(Image.CreateFromData(info.Width, info.Height, false, Image.Format.Rf, heightBytes));
    }

    private float PackLayer(uint packed)
    {
        int index = (int)(packed & 0xFFFF);
        if (index == 0xFFFF || index >= info.TextureCount)
        {
            return totalLayers;
        }
        return layerOffset[index] + (int)(packed >> 16);
    }

    private static ArrayMesh BuildQuad()
    {
        var arrays = new Godot.Collections.Array();
        arrays.Resize((int)Mesh.ArrayType.Max);
        arrays[(int)Mesh.ArrayType.Vertex] = new[]
        {
            new Vector3(0, 0, 0), new Vector3(1, 0, 0), new Vector3(0, 0, 1), new Vector3(1, 0, 1),
        };
        arrays[(int)Mesh.ArrayType.Normal] = new[] { Vector3.Up, Vector3.Up, Vector3.Up, Vector3.Up };
        arrays[(int)Mesh.ArrayType.TexUV] = new[]
        {
            new Vector2(0, 0), new Vector2(1, 0), new Vector2(0, 1), new Vector2(1, 1),
        };
        arrays[(int)Mesh.ArrayType.Index] = new[] { 0, 2, 1, 1, 2, 3 };

        var mesh = new ArrayMesh();
        mesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Triangles, arrays);
        return mesh;
    }

    private void BuildCamera()
    {
        focus = new Vector3(info.TilesX * 0.5f, 0, info.TilesY * 0.5f);
        camera = new Camera3D { Fov = 60 };
        AddChild(camera);
        camera.Current = true;
        UpdateCamera();
    }

    private void UpdateCamera()
    {
        float height = MathF.Sin(Pitch) * distance;
        float back = MathF.Cos(Pitch) * distance;
        camera.Position = focus + new Vector3(0, height, back);
        camera.LookAt(focus, Vector3.Up);
    }

    private void BuildBrushPreview()
    {
        var previewMaterial = new StandardMaterial3D
        {
            ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
            Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
            AlbedoColor = new Color(0.1f, 1f, 0.3f, 0.22f),
            NoDepthTest = false,
        };

        brushPreview = new MeshInstance3D
        {
            Mesh = new QuadMesh { Size = new Vector2(brushSize, brushSize) },
            MaterialOverride = previewMaterial,
            CastShadow = GeometryInstance3D.ShadowCastingSetting.Off,
        };
        AddChild(brushPreview);
    }

    private float HeightAt(float x, float z)
    {
        float fx = Mathf.Clamp(x, 0f, info.Width - 1.001f);
        float fz = Mathf.Clamp(z, 0f, info.Height - 1.001f);
        int x0 = (int)fx;
        int z0 = (int)fz;
        int x1 = Math.Min(x0 + 1, info.Width - 1);
        int z1 = Math.Min(z0 + 1, info.Height - 1);
        float tx = fx - x0;
        float tz = fz - z0;

        float h00 = heights[z0 * info.Width + x0];
        float h10 = heights[z0 * info.Width + x1];
        float h01 = heights[z1 * info.Width + x0];
        float h11 = heights[z1 * info.Width + x1];

        return Mathf.Lerp(Mathf.Lerp(h00, h10, tx), Mathf.Lerp(h01, h11, tx), tz);
    }

    private bool PickTerrain(Vector2 screen, out Vector3 hit)
    {
        hit = Vector3.Zero;
        Vector3 origin = camera.ProjectRayOrigin(screen);
        Vector3 direction = camera.ProjectRayNormal(screen);

        float travelled = 0f;
        Vector3 point = origin;
        for (int i = 0; i < 512; i++)
        {
            float ground = HeightAt(point.X, point.Z);
            float above = point.Y - ground;
            if (above <= 0f)
            {
                hit = new Vector3(point.X, ground, point.Z);
                return true;
            }
            if (travelled > 5000f)
            {
                break;
            }
            travelled += Math.Max(above * 0.5f, 0.15f);
            point = origin + direction * travelled;
            if (point.X < -64f || point.Z < -64f || point.X > info.TilesX + 64f || point.Z > info.TilesY + 64f)
            {
                if (travelled > 32f)
                {
                    break;
                }
            }
        }
        return false;
    }

    private void ApplyTool()
    {
        switch (tool)
        {
            case Tool.Texture:
                HiveCore.SetTextureTool(HiveCore.TextureId(textureIndex) ?? "Ldrt");
                break;
            case Tool.Height:
                HiveCore.SetHeightTool(0);
                break;
            case Tool.Water:
                HiveCore.SetCellTool(0);
                break;
            case Tool.Cliff:
                HiveCore.SetCliffTool(3, null);
                break;
        }
        HiveCore.BrushShape(0);
        HiveCore.BrushSize(brushSize);
        UpdateLabel();
    }

    private void UpdateLabel()
    {
        string toolName = tool switch
        {
            Tool.Texture => $"纹理笔（{HiveCore.TextureId(textureIndex)}）",
            Tool.Height => "高度抬升",
            Tool.Water => "加水",
            Tool.Cliff => "悬崖抬升",
            _ => "?",
        };

        label.Text = $"{info.Width}×{info.Height} 顶点，{info.TilesX}×{info.TilesY} 格\n"
            + $"工具 {toolName}　笔刷 {brushSize}　缩放 {distance:F0}\n"
            + "1 纹理 2 抬升 3 加水 4 悬崖　Tab 换纹理　Q/E 笔刷　Z/Y 撤销重做\n"
            + "右键拖动平移，滚轮缩放，左键绘制，Ctrl+S 保存";
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (@event is InputEventMouseButton button)
        {
            HandleMouseButton(button);
        }
        else if (@event is InputEventMouseMotion motion)
        {
            HandleMouseMotion(motion);
        }
        else if (@event is InputEventKey key && key.Pressed && !key.Echo)
        {
            HandleKey(key);
        }
    }

    private void HandleMouseButton(InputEventMouseButton button)
    {
        switch (button.ButtonIndex)
        {
            case MouseButton.WheelUp when button.Pressed:
                distance = Math.Max(24f, distance * 0.88f);
                UpdateCamera();
                UpdateLabel();
                break;
            case MouseButton.WheelDown when button.Pressed:
                distance = Math.Min(1400f, distance * 1.14f);
                UpdateCamera();
                UpdateLabel();
                break;
            case MouseButton.Right:
                panning = button.Pressed;
                break;
            case MouseButton.Left:
                if (button.Pressed)
                {
                    if (PickTerrain(button.Position, out Vector3 hit))
                    {
                        painting = true;
                        HiveCore.BrushMove(hit.X, hit.Z);
                        HiveCore.BrushBegin();
                        HiveCore.BrushApply(0.016);
                        RefreshAfterEdit();
                    }
                }
                else if (painting)
                {
                    painting = false;
                    HiveCore.BrushEnd();
                    RefreshAfterEdit();
                }
                break;
        }
    }

    private void HandleMouseMotion(InputEventMouseMotion motion)
    {
        if (panning)
        {
            float scale = distance * 0.0016f;
            focus -= new Vector3(motion.Relative.X * scale, 0, motion.Relative.Y * scale);
            focus.X = Mathf.Clamp(focus.X, 0f, info.TilesX);
            focus.Z = Mathf.Clamp(focus.Z, 0f, info.TilesY);
            UpdateCamera();
            return;
        }

        if (!PickTerrain(motion.Position, out Vector3 hit))
        {
            brushPreview.Visible = false;
            return;
        }

        brushPreview.Visible = true;
        brushPreview.Position = new Vector3(
            MathF.Floor(hit.X / brushSize) * brushSize + brushSize * 0.5f,
            hit.Y + 0.05f,
            MathF.Floor(hit.Z / brushSize) * brushSize + brushSize * 0.5f);

        if (painting)
        {
            HiveCore.BrushMove(hit.X, hit.Z);
            HiveCore.BrushApply(0.016);
            RefreshAfterEdit();
        }
    }

    private void HandleKey(InputEventKey key)
    {
        switch (key.Keycode)
        {
            case Key.Key1:
                tool = Tool.Texture;
                ApplyTool();
                break;
            case Key.Key2:
                tool = Tool.Height;
                ApplyTool();
                break;
            case Key.Key3:
                tool = Tool.Water;
                ApplyTool();
                break;
            case Key.Key4:
                tool = Tool.Cliff;
                ApplyTool();
                break;
            case Key.Tab:
                textureIndex = (textureIndex + 1) % info.TextureCount;
                if (tool != Tool.Texture)
                {
                    tool = Tool.Texture;
                }
                ApplyTool();
                break;
            case Key.Q:
                brushSize = Math.Max(1, brushSize - 1);
                ApplyTool();
                brushPreview.Mesh = new QuadMesh { Size = new Vector2(brushSize, brushSize) };
                break;
            case Key.E:
                brushSize = Math.Min(48, brushSize + 1);
                ApplyTool();
                brushPreview.Mesh = new QuadMesh { Size = new Vector2(brushSize, brushSize) };
                break;
            case Key.Z:
                HiveCore.Undo();
                RefreshAfterEdit();
                break;
            case Key.Y:
                HiveCore.Redo();
                RefreshAfterEdit();
                break;
            case Key.S when key.CtrlPressed:
                SaveMap();
                break;
        }
    }

    private void SaveMap()
    {
        string outputDir = ProjectSettings.GlobalizePath("res://") + "../output";
        System.IO.Directory.CreateDirectory(outputDir);
        string target = System.IO.Path.Combine(outputDir, "edited.w3m");

        if (HiveCore.SaveMap(target, out string error))
        {
            GD.Print($"已保存：{target}");
            label.Text = $"已保存到 {target}\n" + label.Text;
        }
        else
        {
            GD.PrintErr($"保存失败：{error}");
            label.Text = $"保存失败：{error}\n" + label.Text;
        }
    }

    private void RefreshAfterEdit()
    {
        HiveCore.CopyHeights(heights);
        HiveCore.CopyTiles(tiles);
        HiveCore.CopyWaterHeights(waterHeights);
        HiveCore.CopyWaterExists(waterExists);
        RefreshHeights();
        RefreshTiles();
        RefreshWater();
    }

    private int frames;
    private readonly string screenshotPath = OS.GetEnvironment("HIVE_SCREENSHOT");
    private readonly string autoTest = OS.GetEnvironment("HIVE_AUTOTEST");

    public override void _Process(double delta)
    {
        if (screenshotPath.Length == 0)
        {
            return;
        }

        frames++;

        if (autoTest.Length == 0 && frames == 12)
        {
            Capture(screenshotPath);
            GetTree().Quit();
            return;
        }

        if (autoTest.Length > 0)
        {
            if (frames == 8)
            {
                focus = new Vector3(96f, 0, 48f);
                distance = 78f;
                UpdateCamera();
            }
            else if (frames == 12)
            {
                Capture(autoTest + "_before.png");
                RunAutoTest();
            }
            else if (frames == 22)
            {
                Capture(autoTest + "_after.png");
                GetTree().Quit();
            }
        }
    }

    private void Capture(string path)
    {
        Image shot = GetViewport().GetTexture().GetImage();
        shot.SavePng(path);
        GD.Print($"截图：{path}");
    }

    private void RunAutoTest()
    {
        HiveTile[] before = (HiveTile[])tiles.Clone();
        float[] heightsBefore = (float[])heights.Clone();

        PaintTexture("Lrok", 96f, 48f, 28);
        PaintHeight(62f, 40f, 20, 12);
        PaintWater(140f, 74f, 16);
        RenderTexture();

        int changedTiles = 0;
        for (int i = 0; i < tiles.Length; i++)
        {
            if (tiles[i].Tex0 != before[i].Tex0 || tiles[i].Tex1 != before[i].Tex1
                || tiles[i].Tex2 != before[i].Tex2 || tiles[i].Tex3 != before[i].Tex3)
            {
                changedTiles++;
            }
        }

        int changedHeights = 0;
        float raised = 0f;
        for (int i = 0; i < heights.Length; i++)
        {
            if (MathF.Abs(heights[i] - heightsBefore[i]) > 0.0001f)
            {
                changedHeights++;
                raised = MathF.Max(raised, heights[i] - heightsBefore[i]);
            }
        }

        GD.Print($"自动测试：改了 {changedTiles} 个格子，{changedHeights} 个高度点，最大抬升 {raised:F2}");

        SaveMap();
    }

    private void PaintTexture(string id, float x, float y, int size)
    {
        HiveCore.SetTextureTool(id);
        HiveCore.BrushShape(1);
        HiveCore.BrushSize(size);
        HiveCore.BrushMove(x, y);
        HiveCore.BrushBegin();
        HiveCore.BrushApply(0.016);
        HiveCore.BrushEnd();
        RefreshAfterEdit();
    }

    private void PaintHeight(float x, float y, int size, int rounds)
    {
        HiveCore.SetHeightTool(0);
        HiveCore.BrushShape(1);
        HiveCore.BrushSize(size);
        HiveCore.BrushMove(x, y);
        HiveCore.BrushBegin();
        for (int i = 0; i < rounds; i++)
        {
            HiveCore.BrushApply(0.05);
        }
        HiveCore.BrushEnd();
        RefreshAfterEdit();
    }

    private void PaintWater(float x, float y, int size)
    {
        HiveCore.SetCellTool(0);
        HiveCore.BrushShape(1);
        HiveCore.BrushSize(size);
        HiveCore.BrushMove(x, y);
        HiveCore.BrushBegin();
        HiveCore.BrushApply(0.016);
        HiveCore.BrushEnd();
        RefreshAfterEdit();
    }

    private void RenderTexture()
    {
        HiveCore.BrushShape(0);
        HiveCore.BrushSize(8);
        HiveCore.SetTextureTool(HiveCore.TextureId(0) ?? "Ldrt");
    }

    public override void _ExitTree()
    {
        HiveCore.Close();
    }
}
