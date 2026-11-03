using System;
using Godot;

namespace HiveWeGodot;

public partial class ModelWindow : AcceptDialog
{
    private TerrainView view = null!;
    private LineEdit pathEdit = null!;
    private OptionButton sequencePicker = null!;
    private CheckBox playBox = null!;
    private HSlider frameSlider = null!;
    private Label status = null!;
    private SubViewport preview = null!;
    private MeshInstance3D instance = null!;

    private CheckBox boxToggle = null!;
    private CheckBox gridToggle = null!;
    private Tree geosetTree = null!;
    private MeshInstance3D? boxNode;
    private MeshInstance3D? gridNode;

    private int model = -1;
    private int sequence;
    private int start;
    private int end;
    private double clock;

    public void Build(TerrainView terrainView, string startPath)
    {
        view = terrainView;
        Title = "模型预览";
        OkButtonText = "关闭";

        var column = new VBoxContainer
        {
            CustomMinimumSize = new Vector2(720, 620),
        };
        column.AddThemeConstantOverride("separation", 6);
        AddChild(column);

        var pathRow = new HBoxContainer();
        pathRow.AddThemeConstantOverride("separation", 6);
        column.AddChild(pathRow);

        pathEdit = new LineEdit
        {
            Text = startPath,
            PlaceholderText = "模型路径，如 Units/Human/Footman/Footman.mdx",
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
        };
        pathEdit.TextSubmitted += _ => Open(pathEdit.Text);
        pathRow.AddChild(pathEdit);

        var openButton = new Button { Text = "打开" };
        openButton.Pressed += () => Open(pathEdit.Text);
        pathRow.AddChild(openButton);

        var controlRow = new HBoxContainer();
        controlRow.AddThemeConstantOverride("separation", 10);
        column.AddChild(controlRow);

        controlRow.AddChild(new Label { Text = "序列" });
        sequencePicker = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        sequencePicker.ItemSelected += index =>
        {
            sequence = (int)index;
            (start, end) = HiveCore.ModelSequenceRange(model, sequence);
            frameSlider.MaxValue = Math.Max(0, end - start);
            frameSlider.Value = 0;
            clock = 0;
            Apply(FrameAt(0));
        };
        controlRow.AddChild(sequencePicker);

        playBox = new CheckBox { Text = "播放", ButtonPressed = true };
        controlRow.AddChild(playBox);

        boxToggle = new CheckBox { Text = "包围盒" };
        boxToggle.Toggled += on =>
        {
            if (boxNode != null)
            {
                boxNode.Visible = on;
            }
        };
        controlRow.AddChild(boxToggle);

        gridToggle = new CheckBox { Text = "网格", ButtonPressed = true };
        gridToggle.Toggled += on =>
        {
            if (gridNode != null)
            {
                gridNode.Visible = on;
            }
        };
        controlRow.AddChild(gridToggle);

        frameSlider = new HSlider
        {
            MinValue = 0,
            MaxValue = 0,
            Step = 1,
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            CustomMinimumSize = new Vector2(240, 0),
        };
        frameSlider.ValueChanged += value => Apply(FrameAt((int)value));
        controlRow.AddChild(frameSlider);

        var container = new SubViewportContainer
        {
            Stretch = true,
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            CustomMinimumSize = new Vector2(0, 420),
        };
        column.AddChild(container);

        preview = new SubViewport
        {
            Size = new Vector2I(720, 420),
            RenderTargetUpdateMode = SubViewport.UpdateMode.Always,
            OwnWorld3D = true,
        };
        container.AddChild(preview);

        preview.AddChild(new Camera3D { Fov = 45, Current = true });

        var light = new DirectionalLight3D
        {
            RotationDegrees = new Vector3(-50, -40, 0),
            LightEnergy = 1.2f,
        };
        preview.AddChild(light);

        var environment = new WorldEnvironment
        {
            Environment = new Godot.Environment
            {
                BackgroundMode = Godot.Environment.BGMode.Color,
                BackgroundColor = new Color(0.16f, 0.17f, 0.2f),
                AmbientLightSource = Godot.Environment.AmbientSource.Color,
                AmbientLightColor = new Color(0.55f, 0.58f, 0.62f),
                AmbientLightEnergy = 0.9f,
            },
        };
        preview.AddChild(environment);

        instance = new MeshInstance3D { Name = "Preview" };
        preview.AddChild(instance);

        var split = new VSplitContainer { SizeFlagsVertical = Control.SizeFlags.ExpandFill };
        column.AddChild(split);

        geosetTree = new Tree
        {
            Columns = 6,
            ColumnTitlesVisible = true,
            HideRoot = true,
            CustomMinimumSize = new Vector2(0, 140),
        };
        string[] titles = { "层", "顶点", "面", "贴图", "双面", "alpha" };
        for (int i = 0; i < titles.Length; i++)
        {
            geosetTree.SetColumnTitle(i, titles[i]);
            geosetTree.SetColumnExpandRatio(i, i == 3 ? 4 : 1);
        }
        split.AddChild(geosetTree);

        status = new Label { Text = "输入模型路径后打开" };
        column.AddChild(status);

        if (startPath.Length > 0)
        {
            Open(startPath);
        }
    }

    private int FrameAt(int offset)
    {
        int length = Math.Max(1, end - start);
        return start + (int)Mathf.PosMod(offset, length);
    }

    private void Open(string path)
    {
        if (path.Length == 0)
        {
            status.Text = "路径为空";
            return;
        }

        int opened = HiveCore.ModelFromPath(path);
        if (opened < 0)
        {
            status.Text = $"打不开 {path}";
            return;
        }

        model = opened;
        HiveModelInfo info = HiveCore.ModelInfo(model);
        if (info.VertexCount == 0)
        {
            status.Text = $"{path} 没有几何体";
            return;
        }

        sequencePicker.Clear();
        int count = HiveCore.ModelSequenceCount(model);
        for (int i = 0; i < count; i++)
        {
            sequencePicker.AddItem(HiveCore.ModelSequenceName(model, i) ?? $"序列 {i}", i);
        }

        sequence = 0;
        if (count > 0)
        {
            sequencePicker.Select(0);
            (start, end) = HiveCore.ModelSequenceRange(model, 0);
        }
        else
        {
            start = 0;
            end = 1;
        }

        frameSlider.MaxValue = Math.Max(0, end - start);
        frameSlider.Value = 0;
        clock = 0;

        float extent = MathF.Max(0.6f, TerrainView.ModelExtent(model));
        var camera = preview.GetChild<Camera3D>(0);
        var center = new Vector3(0f, extent * 0.45f, 0f);
        camera.Position = center + new Vector3(0.7f, 0.75f, 1f).Normalized() * extent * 1.9f;
        camera.LookAt(center, Vector3.Up);

        Apply(start);
        BuildBox(info);
        BuildGrid();
        FillGeosets(info);
        status.Text = $"{path}　{info.GeosetCount} 层　{info.VertexCount} 顶点　{count} 个序列"
            + $"　{HiveCore.ModelMaterialCount(model)} 材质　{HiveCore.ModelTextureCount(model)} 贴图"
            + $"　{HiveCore.ModelNodeCount(model)} 节点";
    }

    private void FillGeosets(HiveModelInfo info)
    {
        geosetTree.Clear();
        TreeItem root = geosetTree.CreateItem();
        for (int i = 0; i < info.GeosetCount; i++)
        {
            HiveGeosetInfo geoset = HiveCore.ModelGeoset(model, i);
            TreeItem item = geosetTree.CreateItem(root);
            item.SetText(0, i.ToString());
            item.SetText(1, geoset.VertexCount.ToString());
            item.SetText(2, (geoset.IndexCount / 3).ToString());
            item.SetText(3, HiveCore.ModelGeosetTexture(model, i) ?? "（无）");
            item.SetText(4, geoset.TwoSided != 0 ? "是" : "否");
            item.SetText(5, geoset.Alpha.ToString("F2"));
        }
    }

    private void BuildBox(HiveModelInfo info)
    {
        if (boxNode != null)
        {
            preview.RemoveChild(boxNode);
            boxNode.QueueFree();
            boxNode = null;
        }

        const float scale = 1f / 128f;
        var min = new Vector3(info.MinX, info.MinZ, info.MinY) * scale;
        var max = new Vector3(info.MaxX, info.MaxZ, info.MaxY) * scale;

        var mesh = new ImmediateMesh();
        mesh.SurfaceBegin(Mesh.PrimitiveType.Lines);
        mesh.SurfaceSetColor(new Color(1f, 0.85f, 0.2f));
        Vector3[] corners =
        {
            new(min.X, min.Y, min.Z), new(max.X, min.Y, min.Z), new(max.X, min.Y, max.Z), new(min.X, min.Y, max.Z),
            new(min.X, max.Y, min.Z), new(max.X, max.Y, min.Z), new(max.X, max.Y, max.Z), new(min.X, max.Y, max.Z),
        };
        int[] edges =
        {
            0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7,
        };
        foreach (int index in edges)
        {
            mesh.SurfaceAddVertex(corners[index]);
        }
        mesh.SurfaceEnd();

        boxNode = new MeshInstance3D
        {
            Mesh = mesh,
            Visible = boxToggle.ButtonPressed,
            CastShadow = GeometryInstance3D.ShadowCastingSetting.Off,
            MaterialOverride = new StandardMaterial3D
            {
                ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
                VertexColorUseAsAlbedo = true,
            },
        };
        preview.AddChild(boxNode);
    }

    private void BuildGrid()
    {
        if (gridNode != null)
        {
            preview.RemoveChild(gridNode);
            gridNode.QueueFree();
            gridNode = null;
        }

        var mesh = new ImmediateMesh();
        mesh.SurfaceBegin(Mesh.PrimitiveType.Lines);
        mesh.SurfaceSetColor(new Color(0.45f, 0.47f, 0.5f));
        const int extent = 4;
        for (int i = -extent; i <= extent; i++)
        {
            mesh.SurfaceAddVertex(new Vector3(i, 0f, -extent));
            mesh.SurfaceAddVertex(new Vector3(i, 0f, extent));
            mesh.SurfaceAddVertex(new Vector3(-extent, 0f, i));
            mesh.SurfaceAddVertex(new Vector3(extent, 0f, i));
        }
        mesh.SurfaceEnd();

        gridNode = new MeshInstance3D
        {
            Mesh = mesh,
            Visible = gridToggle.ButtonPressed,
            CastShadow = GeometryInstance3D.ShadowCastingSetting.Off,
            MaterialOverride = new StandardMaterial3D
            {
                ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
                VertexColorUseAsAlbedo = true,
            },
        };
        preview.AddChild(gridNode);
    }

    private void Apply(int frame)
    {
        if (model < 0)
        {
            return;
        }
        if (end > start)
        {
            HiveCore.ModelPose(model, sequence, frame);
        }
        ArrayMesh? mesh = view.PreviewMesh(model);
        if (mesh != null)
        {
            instance.Mesh = mesh;
        }
    }

    public override void _Process(double delta)
    {
        if (model < 0 || end - start <= 1 || !playBox.ButtonPressed || !Visible)
        {
            return;
        }

        clock += delta * 1000.0;
        frameSlider.SetValueNoSignal((int)Mathf.PosMod((int)clock, end - start));
        Apply(FrameAt((int)clock));
    }
}
