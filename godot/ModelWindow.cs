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
        status.Text = $"{path}　{info.GeosetCount} 层　{info.VertexCount} 顶点　{count} 个序列";
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
