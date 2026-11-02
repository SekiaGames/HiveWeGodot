using System;
using Godot;

namespace HiveWeGodot;

public partial class TerrainView : Node3D
{
    public HiveMapInfo Info { get; private set; }
    public float[] Heights { get; private set; } = Array.Empty<float>();

    private HiveTile[] tiles = Array.Empty<HiveTile>();
    private float[] waterHeights = Array.Empty<float>();
    private byte[] waterExists = Array.Empty<byte>();
    private float waterOffset;

    private MultiMesh terrainMesh = null!;
    private MultiMesh waterMesh = null!;
    private ImageTexture heightTexture = null!;
    private int[] layerOffset = Array.Empty<int>();
    private int totalLayers;
    private int tileSize;
    private Texture2DArray textureArray = null!;

    public int LayerCount => totalLayers;
    public int TileSize => tileSize;

    public void Build()
    {
        Info = HiveCore.MapInfo;
        Heights = HiveCore.Heights();
        tiles = HiveCore.Tiles();
        waterHeights = HiveCore.WaterHeights();
        waterExists = HiveCore.WaterExists();
        waterOffset = HiveCore.WaterOffset();

        BuildTextureArray();
        BuildTerrainMesh();
        BuildWaterMesh();
        BuildLighting();
    }

    private void BuildTextureArray()
    {
        byte[] heightBytes = new byte[Heights.Length * 4];
        Buffer.BlockCopy(Heights, 0, heightBytes, 0, heightBytes.Length);
        heightTexture = ImageTexture.CreateFromImage(
            Image.CreateFromData(Info.Width, Info.Height, false, Image.Format.Rf, heightBytes));

        var layers = new Godot.Collections.Array<Image>();
        layerOffset = new int[Info.TextureCount];

        for (int i = 0; i < Info.TextureCount; i++)
        {
            HiveCore.TextureInfo(i, out int size, out int count);
            tileSize = Math.Max(tileSize, size);
            layerOffset[i] = totalLayers;
            totalLayers += count;
        }

        for (int i = 0; i < Info.TextureCount; i++)
        {
            HiveCore.TextureInfo(i, out int size, out int count);
            byte[] pixels = HiveCore.TexturePixels(i, size, count);
            int layerBytes = size * size * 4;

            for (int layer = 0; layer < count; layer++)
            {
                byte[] slice = new byte[layerBytes];
                Buffer.BlockCopy(pixels, layer * layerBytes, slice, 0, layerBytes);
                Image image = Image.CreateFromData(size, size, false, Image.Format.Rgba8, slice);
                if (size != tileSize)
                {
                    image.Resize(tileSize, tileSize, Image.Interpolation.Bilinear);
                }
                layers.Add(image);
            }
        }

        layers.Add(Image.CreateFromData(tileSize, tileSize, false, Image.Format.Rgba8, new byte[tileSize * tileSize * 4]));

        textureArray = new Texture2DArray();
        if (textureArray.CreateFromImages(layers) != Error.Ok)
        {
            GD.PrintErr("纹理数组创建失败");
        }
    }

    private void BuildTerrainMesh()
    {
        terrainMesh = new MultiMesh
        {
            TransformFormat = MultiMesh.TransformFormatEnum.Transform3D,
            UseCustomData = true,
            Mesh = BuildQuad(),
            InstanceCount = Info.TilesX * Info.TilesY,
        };

        for (int y = 0; y < Info.TilesY; y++)
        {
            for (int x = 0; x < Info.TilesX; x++)
            {
                terrainMesh.SetInstanceTransform(y * Info.TilesX + x, new Transform3D(Basis.Identity, new Vector3(x, 0, y)));
            }
        }
        RefreshTiles();

        var material = new ShaderMaterial { Shader = GD.Load<Shader>("res://Shaders/terrain.gdshader") };
        material.SetShaderParameter("ground_tex", textureArray);
        material.SetShaderParameter("height_map", heightTexture);
        material.SetShaderParameter("vertex_size", new Vector2(Info.Width, Info.Height));

        AddChild(new MultiMeshInstance3D
        {
            Multimesh = terrainMesh,
            MaterialOverride = material,
            Name = "Terrain",
        });
    }

    private void BuildWaterMesh()
    {
        waterMesh = new MultiMesh
        {
            TransformFormat = MultiMesh.TransformFormatEnum.Transform3D,
            UseCustomData = true,
            Mesh = BuildQuad(),
            InstanceCount = Info.TilesX * Info.TilesY,
        };

        var material = new ShaderMaterial { Shader = GD.Load<Shader>("res://Shaders/water.gdshader") };

        AddChild(new MultiMeshInstance3D
        {
            Multimesh = waterMesh,
            MaterialOverride = material,
            Name = "Water",
            CastShadow = GeometryInstance3D.ShadowCastingSetting.Off,
        });

        RefreshWater();
    }

    private void BuildLighting()
    {
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

    public void Reload()
    {
        HiveCore.CopyHeights(Heights);
        HiveCore.CopyTiles(tiles);
        HiveCore.CopyWaterHeights(waterHeights);
        HiveCore.CopyWaterExists(waterExists);

        byte[] heightBytes = new byte[Heights.Length * 4];
        Buffer.BlockCopy(Heights, 0, heightBytes, 0, heightBytes.Length);
        heightTexture.Update(Image.CreateFromData(Info.Width, Info.Height, false, Image.Format.Rf, heightBytes));

        RefreshTiles();
        RefreshWater();
    }

    private void RefreshTiles()
    {
        for (int i = 0; i < tiles.Length; i++)
        {
            HiveTile tile = tiles[i];
            terrainMesh.SetInstanceCustomData(i, new Color(
                PackLayer(tile.Tex0), PackLayer(tile.Tex1), PackLayer(tile.Tex2), PackLayer(tile.Tex3)));
        }
    }

    private void RefreshWater()
    {
        int wet = 0;
        for (int y = 0; y < Info.TilesY; y++)
        {
            for (int x = 0; x < Info.TilesX; x++)
            {
                int index = y * Info.TilesX + x;
                int c00 = y * Info.Width + x;
                int c10 = c00 + 1;
                int c01 = c00 + Info.Width;
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
        WetTiles = wet;
    }

    public int WetTiles { get; private set; }

    private float PackLayer(uint packed)
    {
        int index = (int)(packed & 0xFFFF);
        if (index == 0xFFFF || index >= Info.TextureCount)
        {
            return totalLayers;
        }
        return layerOffset[index] + (int)(packed >> 16);
    }

    public float HeightAt(float x, float z)
    {
        float fx = Mathf.Clamp(x, 0f, Info.Width - 1.001f);
        float fz = Mathf.Clamp(z, 0f, Info.Height - 1.001f);
        int x0 = (int)fx;
        int z0 = (int)fz;
        int x1 = Math.Min(x0 + 1, Info.Width - 1);
        int z1 = Math.Min(z0 + 1, Info.Height - 1);
        float tx = fx - x0;
        float tz = fz - z0;

        float h00 = Heights[z0 * Info.Width + x0];
        float h10 = Heights[z0 * Info.Width + x1];
        float h01 = Heights[z1 * Info.Width + x0];
        float h11 = Heights[z1 * Info.Width + x1];

        return Mathf.Lerp(Mathf.Lerp(h00, h10, tx), Mathf.Lerp(h01, h11, tx), tz);
    }

    public bool Pick(Camera3D camera, Vector2 screen, out Vector3 hit)
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
        }
        return false;
    }

    public Texture2D? Minimap()
    {
        Image? image = HiveCore.Minimap();
        return image == null ? null : ImageTexture.CreateFromImage(image);
    }

    public Texture2D? TextureThumbnail(int index)
    {
        if (!HiveCore.TextureInfo(index, out int size, out int layers) || size == 0)
        {
            return null;
        }
        byte[] pixels = HiveCore.TexturePixels(index, size, layers);
        byte[] slice = new byte[size * size * 4];
        Buffer.BlockCopy(pixels, 0, slice, 0, slice.Length);
        return ImageTexture.CreateFromImage(Image.CreateFromData(size, size, false, Image.Format.Rgba8, slice));
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
}
