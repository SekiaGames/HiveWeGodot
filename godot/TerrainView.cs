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
    private ShaderMaterial terrainMaterial = null!;

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

        terrainMaterial = new ShaderMaterial { Shader = GD.Load<Shader>("res://Shaders/terrain.gdshader") };
        terrainMaterial.SetShaderParameter("ground_tex", textureArray);
        terrainMaterial.SetShaderParameter("height_map", heightTexture);
        terrainMaterial.SetShaderParameter("vertex_size", new Vector2(Info.Width, Info.Height));
        BuildPathingOverlay();

        var material = terrainMaterial;

        AddChild(new MultiMeshInstance3D
        {
            Multimesh = terrainMesh,
            MaterialOverride = material,
            Name = "Terrain",
        });
    }

    private void BuildPathingOverlay()
    {
        byte[]? cells = HiveCore.Pathing(out int width, out int height);
        if (cells == null)
        {
            return;
        }

        byte[] rgba = new byte[width * height * 4];
        int blocked = 0;
        for (int i = 0; i < cells.Length; i++)
        {
            const int unwalkable = 0x02;
            const int water = 0x40;

            if ((cells[i] & unwalkable) != 0 && (cells[i] & water) == 0)
            {
                rgba[i * 4] = 220;
                rgba[i * 4 + 1] = 45;
                rgba[i * 4 + 2] = 45;
                rgba[i * 4 + 3] = 95;
                blocked++;
            }
        }

        Image image = Image.CreateFromData(width, height, false, Image.Format.Rgba8, rgba);
        terrainMaterial.SetShaderParameter("pathing_overlay", ImageTexture.CreateFromImage(image));
        PathingBlocked = blocked;
        GD.Print($"路径图 {width}×{height}，不可走 {blocked} 格");
    }

    public int PathingBlocked { get; private set; }

    public void SetPathingVisible(bool visible)
    {
        terrainMaterial.SetShaderParameter("show_pathing", visible);
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

    /// War3 模型坐标以 1/128 tile 为单位，地形是一格一单位。
    private const float ModelScale = 1f / 128f;

    private readonly System.Collections.Generic.Dictionary<int, ArrayMesh?> modelMeshes = new();
    private readonly System.Collections.Generic.Dictionary<string, Texture2D?> textureCache = new();

    private Node3D? doodadRoot;

    public void BuildDoodads()
    {
        if (doodadRoot != null)
        {
            RemoveChild(doodadRoot);
            doodadRoot.QueueFree();
        }
        doodadRoot = new Node3D { Name = "Doodads" };
        AddChild(doodadRoot);

        HiveDoodad[] doodads = HiveCore.Doodads();
        var byModel = new System.Collections.Generic.Dictionary<int, System.Collections.Generic.List<int>>();

        for (int i = 0; i < doodads.Length; i++)
        {
            if (doodads[i].Solid == 0)
            {
                continue;
            }
            int model = HiveCore.ModelFromDoodad(i);
            if (model < 0)
            {
                continue;
            }
            if (!byModel.TryGetValue(model, out var list))
            {
                list = new System.Collections.Generic.List<int>();
                byModel[model] = list;
            }
            list.Add(i);
        }

        int placed = 0;
        foreach (var pair in byModel)
        {
            ArrayMesh? mesh = ModelMesh(pair.Key);
            if (mesh == null)
            {
                continue;
            }

            var multi = new MultiMesh
            {
                TransformFormat = MultiMesh.TransformFormatEnum.Transform3D,
                Mesh = mesh,
                InstanceCount = pair.Value.Count,
            };

            for (int k = 0; k < pair.Value.Count; k++)
            {
                HiveDoodad doodad = doodads[pair.Value[k]];
                Basis basis = Basis.Identity.Rotated(Vector3.Up, -doodad.Angle)
                    .Scaled(new Vector3(doodad.ScaleX, doodad.ScaleY, doodad.ScaleZ));
                multi.SetInstanceTransform(k, new Transform3D(basis, new Vector3(doodad.X, doodad.Z, doodad.Y)));
            }

            doodadRoot.AddChild(new MultiMeshInstance3D { Multimesh = multi, Name = "Doodads" });
            placed += pair.Value.Count;
        }

        GD.Print($"装饰物 {placed} / {doodads.Length}，{byModel.Count} 种模型");
    }

    private MeshInstance3D? regionRoot;

    public void BuildRegions()
    {
        HiveRegion[] regions = HiveCore.Regions();
        if (regions.Length == 0)
        {
            return;
        }

        var mesh = new ImmediateMesh();
        mesh.SurfaceBegin(Mesh.PrimitiveType.Lines);

        foreach (HiveRegion region in regions)
        {
            mesh.SurfaceSetColor(new Color(region.Red / 255f, region.Green / 255f, region.Blue / 255f));
            AddRegionEdge(mesh, region.Left, region.Bottom, region.Right, region.Bottom);
            AddRegionEdge(mesh, region.Right, region.Bottom, region.Right, region.Top);
            AddRegionEdge(mesh, region.Right, region.Top, region.Left, region.Top);
            AddRegionEdge(mesh, region.Left, region.Top, region.Left, region.Bottom);
        }

        mesh.SurfaceEnd();

        regionRoot = new MeshInstance3D
        {
            Mesh = mesh,
            Name = "Regions",
            CastShadow = GeometryInstance3D.ShadowCastingSetting.Off,
            MaterialOverride = new StandardMaterial3D
            {
                ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
                VertexColorUseAsAlbedo = true,
                Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
                NoDepthTest = true,
            },
        };
        AddChild(regionRoot);
        GD.Print($"区域 {regions.Length} 个");
    }

    private void AddRegionEdge(ImmediateMesh mesh, float x0, float z0, float x1, float z1)
    {
        int steps = Math.Max(1, (int)(MathF.Max(MathF.Abs(x1 - x0), MathF.Abs(z1 - z0)) * 2f));
        for (int i = 0; i < steps; i++)
        {
            float t0 = (float)i / steps;
            float t1 = (float)(i + 1) / steps;
            float ax = Mathf.Lerp(x0, x1, t0);
            float az = Mathf.Lerp(z0, z1, t0);
            float bx = Mathf.Lerp(x0, x1, t1);
            float bz = Mathf.Lerp(z0, z1, t1);
            mesh.SurfaceAddVertex(new Vector3(ax, HeightAt(ax, az) + 0.15f, az));
            mesh.SurfaceAddVertex(new Vector3(bx, HeightAt(bx, bz) + 0.15f, bz));
        }
    }

    private MeshInstance3D? selectionBox;
    private MeshInstance3D? dragGhost;

    public void ShowDragGhost(bool isUnit, int index, float x, float y, float z)
    {
        int model = isUnit ? HiveCore.ModelFromUnit(index) : HiveCore.ModelFromDoodad(index);
        if (model < 0)
        {
            return;
        }

        if (dragGhost == null)
        {
            ArrayMesh? mesh = ModelMesh(model);
            if (mesh == null)
            {
                return;
            }
            dragGhost = new MeshInstance3D
            {
                Mesh = mesh,
                Name = "DragGhost",
                CastShadow = GeometryInstance3D.ShadowCastingSetting.Off,
            };
            AddChild(dragGhost);
        }

        dragGhost.Position = new Vector3(x, z, y);
    }

    public void HideDragGhost()
    {
        if (dragGhost != null)
        {
            RemoveChild(dragGhost);
            dragGhost.QueueFree();
            dragGhost = null;
        }
    }

    public void ShowSelection(bool isUnit, int index)
    {
        ClearSelection();
        if (!HiveCore.SelectionBox(isUnit, index, out Vector3 minimum, out Vector3 maximum))
        {
            return;
        }

        Vector3 size = maximum - minimum;
        if (size.X < 0.05f || size.Y < 0.05f || size.Z < 0.05f)
        {
            minimum -= new Vector3(0.05f, 0.05f, 0.05f);
            maximum += new Vector3(0.05f, 0.05f, 0.05f);
        }

        var corners = new Vector3[8];
        for (int i = 0; i < 8; i++)
        {
            corners[i] = new Vector3(
                (i & 1) == 0 ? minimum.X : maximum.X,
                (i & 2) == 0 ? minimum.Y : maximum.Y,
                (i & 4) == 0 ? minimum.Z : maximum.Z);
        }

        int[] edges =
        {
            0, 1, 1, 3, 3, 2, 2, 0,
            4, 5, 5, 7, 7, 6, 6, 4,
            0, 4, 1, 5, 2, 6, 3, 7,
        };

        var mesh = new ImmediateMesh();
        mesh.SurfaceBegin(Mesh.PrimitiveType.Lines);
        mesh.SurfaceSetColor(new Color(1f, 0.85f, 0.2f));
        foreach (int corner in edges)
        {
            mesh.SurfaceAddVertex(corners[corner]);
        }
        mesh.SurfaceEnd();

        selectionBox = new MeshInstance3D
        {
            Mesh = mesh,
            Name = "Selection",
            CastShadow = GeometryInstance3D.ShadowCastingSetting.Off,
            MaterialOverride = new StandardMaterial3D
            {
                ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
                VertexColorUseAsAlbedo = true,
                NoDepthTest = true,
            },
        };
        AddChild(selectionBox);
    }

    public void ClearSelection()
    {
        if (selectionBox != null)
        {
            RemoveChild(selectionBox);
            selectionBox.QueueFree();
            selectionBox = null;
        }
    }

    private sealed class ClipboardGroup
    {
        public MultiMesh Multi = null!;
        public int[] Slots = Array.Empty<int>();
    }

    private Node3D? clipboardRoot;
    private readonly System.Collections.Generic.List<ClipboardGroup> clipboardGroups = new();
    private Vector3[] ghostOffsets = Array.Empty<Vector3>();
    private float[] ghostAngles = Array.Empty<float>();
    private Vector3[] ghostScales = Array.Empty<Vector3>();

    public void BeginClipboardGhost(bool isUnit)
    {
        HideClipboardGhost();

        Vector2 anchor;
        var models = new System.Collections.Generic.List<int>();
        if (isUnit)
        {
            HiveUnit[] units = HiveCore.UnitClipboard();
            anchor = HiveCore.UnitClipboardAnchor();
            ghostOffsets = new Vector3[units.Length];
            ghostAngles = new float[units.Length];
            ghostScales = new Vector3[units.Length];
            for (int i = 0; i < units.Length; i++)
            {
                ghostOffsets[i] = new Vector3(units[i].X - anchor.X, 0f, units[i].Y - anchor.Y);
                ghostAngles[i] = units[i].Angle;
                ghostScales[i] = new Vector3(units[i].ScaleX, units[i].ScaleY, units[i].ScaleZ);
                models.Add(HiveCore.ModelFromUnitClipboard(i));
            }
        }
        else
        {
            HiveDoodad[] doodads = HiveCore.Clipboard();
            anchor = HiveCore.ClipboardAnchor();
            ghostOffsets = new Vector3[doodads.Length];
            ghostAngles = new float[doodads.Length];
            ghostScales = new Vector3[doodads.Length];
            for (int i = 0; i < doodads.Length; i++)
            {
                ghostOffsets[i] = new Vector3(doodads[i].X - anchor.X, 0f, doodads[i].Y - anchor.Y);
                ghostAngles[i] = doodads[i].Angle;
                ghostScales[i] = new Vector3(doodads[i].ScaleX, doodads[i].ScaleY, doodads[i].ScaleZ);
                models.Add(HiveCore.ModelFromClipboard(i));
            }
        }

        if (ghostOffsets.Length == 0)
        {
            return;
        }

        var byModel = new System.Collections.Generic.Dictionary<int, System.Collections.Generic.List<int>>();
        for (int i = 0; i < models.Count; i++)
        {
            if (models[i] < 0)
            {
                continue;
            }
            if (!byModel.TryGetValue(models[i], out var list))
            {
                list = new System.Collections.Generic.List<int>();
                byModel[models[i]] = list;
            }
            list.Add(i);
        }

        clipboardRoot = new Node3D { Name = "Clipboard" };
        AddChild(clipboardRoot);

        foreach (var pair in byModel)
        {
            ArrayMesh? mesh = ModelMesh(pair.Key);
            if (mesh == null)
            {
                continue;
            }
            var multi = new MultiMesh
            {
                TransformFormat = MultiMesh.TransformFormatEnum.Transform3D,
                Mesh = mesh,
                InstanceCount = pair.Value.Count,
            };
            clipboardRoot.AddChild(new MultiMeshInstance3D { Multimesh = multi, Name = "Clipboard" });
            clipboardGroups.Add(new ClipboardGroup { Multi = multi, Slots = pair.Value.ToArray() });
        }

        GD.Print($"剪贴板预览 {ghostOffsets.Length} 个对象，{clipboardGroups.Count} 组模型");
    }

    public void UpdateClipboardGhost(float x, float z)
    {
        foreach (ClipboardGroup group in clipboardGroups)
        {
            for (int k = 0; k < group.Slots.Length; k++)
            {
                int slot = group.Slots[k];
                float px = x + ghostOffsets[slot].X;
                float pz = z + ghostOffsets[slot].Z;
                Basis basis = Basis.Identity.Rotated(Vector3.Up, -ghostAngles[slot]).Scaled(ghostScales[slot]);
                group.Multi.SetInstanceTransform(k, new Transform3D(basis, new Vector3(px, HeightAt(px, pz), pz)));
            }
        }
    }

    public void HideClipboardGhost()
    {
        clipboardGroups.Clear();
        ghostOffsets = Array.Empty<Vector3>();
        ghostAngles = Array.Empty<float>();
        ghostScales = Array.Empty<Vector3>();
        if (clipboardRoot != null)
        {
            RemoveChild(clipboardRoot);
            clipboardRoot.QueueFree();
            clipboardRoot = null;
        }
    }

    public void SetRegionsVisible(bool visible)
    {
        if (regionRoot != null)
        {
            regionRoot.Visible = visible;
        }
    }

    private Node3D? unitRoot;

    public void BuildUnits()
    {
        if (unitRoot != null)
        {
            RemoveChild(unitRoot);
            unitRoot.QueueFree();
        }
        unitRoot = new Node3D { Name = "Units" };
        AddChild(unitRoot);

        HiveUnit[] units = HiveCore.Units();
        var byModel = new System.Collections.Generic.Dictionary<int, System.Collections.Generic.List<int>>();

        for (int i = 0; i < units.Length; i++)
        {
            int model = HiveCore.ModelFromUnit(i);
            if (model < 0)
            {
                continue;
            }
            if (!byModel.TryGetValue(model, out var list))
            {
                list = new System.Collections.Generic.List<int>();
                byModel[model] = list;
            }
            list.Add(i);
        }

        int placed = 0;
        foreach (var pair in byModel)
        {
            ArrayMesh? mesh = ModelMesh(pair.Key);
            if (mesh == null)
            {
                continue;
            }

            var multi = new MultiMesh
            {
                TransformFormat = MultiMesh.TransformFormatEnum.Transform3D,
                Mesh = mesh,
                InstanceCount = pair.Value.Count,
            };

            for (int k = 0; k < pair.Value.Count; k++)
            {
                HiveUnit unit = units[pair.Value[k]];
                Basis basis = Basis.Identity.Rotated(Vector3.Up, -unit.Angle)
                    .Scaled(new Vector3(unit.ScaleX, unit.ScaleY, unit.ScaleZ));
                multi.SetInstanceTransform(k, new Transform3D(basis, new Vector3(unit.X, unit.Z, unit.Y)));
            }

            unitRoot.AddChild(new MultiMeshInstance3D { Multimesh = multi, Name = "Units" });
            placed += pair.Value.Count;
        }

        GD.Print($"单位 {placed} / {units.Length}，{byModel.Count} 种模型");
    }

    private ArrayMesh? ModelMesh(int model)
    {
        if (modelMeshes.TryGetValue(model, out ArrayMesh? cached))
        {
            return cached;
        }

        HiveModelInfo info = HiveCore.ModelInfo(model);
        if (info.VertexCount == 0 || info.GeosetCount == 0)
        {
            modelMeshes[model] = null;
            return null;
        }

        float[] positions = HiveCore.ModelPositions(model, info.VertexCount);
        float[] normals = HiveCore.ModelNormals(model, info.VertexCount);
        float[] uvs = HiveCore.ModelUvs(model, info.VertexCount);

        var mesh = new ArrayMesh();
        for (int g = 0; g < info.GeosetCount; g++)
        {
            HiveGeosetInfo geoset = HiveCore.ModelGeoset(model, g);
            if (geoset.IndexCount < 3 || geoset.VertexCount == 0)
            {
                continue;
            }

            Vector3[] vertices = new Vector3[geoset.VertexCount];
            Vector3[] normalsOut = new Vector3[geoset.VertexCount];
            Vector2[] texCoords = new Vector2[geoset.VertexCount];

            for (int i = 0; i < geoset.VertexCount; i++)
            {
                int p = (geoset.VertexOffset + i) * 3;
                vertices[i] = new Vector3(positions[p], positions[p + 2], positions[p + 1]) * ModelScale;
                normalsOut[i] = new Vector3(normals[p], normals[p + 2], normals[p + 1]);
                int q = (geoset.VertexOffset + i) * 2;
                texCoords[i] = new Vector2(uvs[q], uvs[q + 1]);
            }

            uint[] raw = HiveCore.ModelIndices(model, g, geoset.IndexCount);
            int[] indices = new int[raw.Length];
            for (int i = 0; i < raw.Length; i++)
            {
                indices[i] = (int)raw[i];
            }

            var arrays = new Godot.Collections.Array();
            arrays.Resize((int)Mesh.ArrayType.Max);
            arrays[(int)Mesh.ArrayType.Vertex] = vertices;
            arrays[(int)Mesh.ArrayType.Normal] = normalsOut;
            arrays[(int)Mesh.ArrayType.TexUV] = texCoords;
            arrays[(int)Mesh.ArrayType.Index] = indices;

            int surface = mesh.GetSurfaceCount();
            mesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Triangles, arrays);
            mesh.SurfaceSetMaterial(surface, BuildMaterial(model, g, geoset));
        }

        if (mesh.GetSurfaceCount() == 0)
        {
            modelMeshes[model] = null;
            return null;
        }

        modelMeshes[model] = mesh;
        return mesh;
    }

    private Material BuildMaterial(int model, int geoset, HiveGeosetInfo info)
    {
        var material = new StandardMaterial3D
        {
            CullMode = info.TwoSided != 0 ? BaseMaterial3D.CullModeEnum.Disabled : BaseMaterial3D.CullModeEnum.Back,
            Transparency = BaseMaterial3D.TransparencyEnum.AlphaScissor,
            AlphaScissorThreshold = 0.5f,
            Roughness = 0.9f,
            Metallic = 0f,
            TextureFilter = BaseMaterial3D.TextureFilterEnum.Linear,
        };

        if (info.Unshaded != 0)
        {
            material.ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded;
        }

        string? path = HiveCore.ModelGeosetTexture(model, geoset);
        if (path != null)
        {
            material.AlbedoTexture = TextureAt(path);
        }

        return material;
    }

    private Texture2D? TextureAt(string path)
    {
        if (textureCache.TryGetValue(path, out Texture2D? cached))
        {
            return cached;
        }
        Image? image = HiveCore.ImageAt(path);
        Texture2D? texture = image == null ? null : ImageTexture.CreateFromImage(image);
        textureCache[path] = texture;
        return texture;
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
