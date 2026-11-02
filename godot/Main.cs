using System;
using Godot;

namespace HiveWeGodot;

public partial class Main : Node3D
{
    private const float Pitch = 0.95f;

    private Node3D root = null!;
    private string dataDir = string.Empty;

    private TerrainView view = null!;
    private EditorUi ui = null!;
    private Camera3D camera = null!;
    private MeshInstance3D brushPreview = null!;

    private EditorUi.ToolMode tool = EditorUi.ToolMode.Texture;
    private int textureIndex;
    private int brushSize = 8;
    private int brushShape;
    private string doodadType = string.Empty;

    private Vector3 focus;
    private float distance = 190f;
    private bool painting;
    private bool panning;
    private Vector3 lastHit;
    private bool hasHit;

    private readonly string screenshotPath = OS.GetEnvironment("HIVE_SCREENSHOT");
    private readonly string autoTest = OS.GetEnvironment("HIVE_AUTOTEST");
    private readonly string reloadTest = OS.GetEnvironment("HIVE_RELOAD_TEST");
    private int frames;
    private bool loaded;

    public override void _Ready()
    {
        root = new Node3D();
        AddChild(root);

        string projectRoot = ProjectSettings.GlobalizePath("res://");
        dataDir = projectRoot + "../testdata/warcraft";
        LoadMap(projectRoot + "../testdata/maps/(2)BootyBay.w3m");
    }

    private void LoadMap(string mapPath)
    {
        if (loaded)
        {
            HiveCore.Close();
            foreach (Node child in root.GetChildren())
            {
                root.RemoveChild(child);
                child.QueueFree();
            }
        }

        if (!HiveCore.Open(mapPath, dataDir, out string error))
        {
            GD.PrintErr(error);
            ui?.SetStatus($"打开失败：{error}");
            return;
        }

        loaded = true;
        tool = EditorUi.ToolMode.Texture;
        textureIndex = 0;
        brushSize = 8;
        brushShape = 0;
        focus = new Vector3(HiveCore.MapInfo.TilesX * 0.5f, 0, HiveCore.MapInfo.TilesY * 0.5f);
        distance = 190f;

        view = new TerrainView();
        root.AddChild(view);
        view.Build();
        view.BuildDoodads();
        view.BuildUnits();
        view.BuildRegions();

        camera = new Camera3D { Fov = 60 };
        root.AddChild(camera);
        camera.Current = true;
        UpdateCamera();

        brushPreview = new MeshInstance3D
        {
            Mesh = new QuadMesh { Size = new Vector2(brushSize, brushSize) },
            MaterialOverride = new StandardMaterial3D
            {
                ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
                Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
                AlbedoColor = new Color(0.1f, 1f, 0.3f, 0.22f),
            },
            CastShadow = GeometryInstance3D.ShadowCastingSetting.Off,
            Visible = false,
        };
        root.AddChild(brushPreview);

        ui = new EditorUi();
        root.AddChild(ui);
        ui.Build(view);

        ui.TextureSelected += SelectTexture;
        ui.ToolSelected += SelectTool;
        ui.BrushSizeChanged += SetBrushSize;
        ui.BrushShapeChanged += SetBrushShape;
        ui.UndoRequested += Undo;
        ui.RedoRequested += Redo;
        ui.SaveRequested += SaveMap;
        ui.OpenRequested += ShowOpenDialog;
        ui.PathingToggled += view.SetPathingVisible;
        ui.RegionsToggled += view.SetRegionsVisible;
        ui.MinimapClicked += world =>
        {
            focus = new Vector3(world.X, 0, world.Y);
            UpdateCamera();
        };

        ui.SetDoodadTypes(HiveCore.DoodadTypes());
        ui.DoodadTypeSelected += id =>
        {
            doodadType = id;
            tool = EditorUi.ToolMode.DoodadPlace;
            ApplyTool();
            UpdateStatus();
        };
        ui.SetBrushSize(brushSize);
        if (OS.GetEnvironment("HIVE_SHOW_PATHING").Length > 0)
        {
            ui.SetPathingChecked(true);
        }
        ApplyTool();
        UpdateStatus();
        GD.Print($"已打开 {mapPath}：{view.Info.Width}×{view.Info.Height}");
    }

    private void ShowOpenDialog()
    {
        var dialog = new FileDialog
        {
            FileMode = FileDialog.FileModeEnum.OpenFile,
            Access = FileDialog.AccessEnum.Filesystem,
            Filters = new[] { "*.w3m, *.w3x ; 地图" },
            UseNativeDialog = true,
        };
        dialog.FileSelected += path =>
        {
            LoadMap(path);
            dialog.QueueFree();
        };
        root.AddChild(dialog);
        dialog.PopupCentered(new Vector2I(900, 600));
    }

    private void UpdateCamera()
    {
        camera.Position = focus + new Vector3(0, MathF.Sin(Pitch) * distance, MathF.Cos(Pitch) * distance);
        camera.LookAt(focus, Vector3.Up);
    }

    private void SelectTexture(int index)
    {
        textureIndex = index;
        tool = EditorUi.ToolMode.Texture;
        ApplyTool();
        UpdateStatus();
    }

    private void SelectTool(EditorUi.ToolMode mode)
    {
        tool = mode;
        ApplyTool();
        UpdateStatus();
    }

    private void SetBrushSize(int size)
    {
        brushSize = Math.Clamp(size, 1, 48);
        HiveCore.BrushSize(brushSize);
        brushPreview.Mesh = new QuadMesh { Size = new Vector2(brushSize, brushSize) };
        UpdateStatus();
    }

    private void SetBrushShape(int shape)
    {
        brushShape = shape;
        HiveCore.BrushShape(shape);
        brushPreview.Visible = shape != 1 && hasHit;
        UpdateStatus();
    }

    private void ApplyTool()
    {
        string name;
        switch (tool)
        {
            case EditorUi.ToolMode.Texture:
                HiveCore.SetTextureTool(HiveCore.TextureId(textureIndex) ?? "Ldrt");
                name = $"纹理笔 {HiveCore.TextureId(textureIndex)}";
                break;
            case EditorUi.ToolMode.HeightRaise:
                HiveCore.SetHeightTool(0);
                name = "高度抬升";
                break;
            case EditorUi.ToolMode.HeightLower:
                HiveCore.SetHeightTool(1);
                name = "高度降低";
                break;
            case EditorUi.ToolMode.HeightPlateau:
                HiveCore.SetHeightTool(2);
                name = "高度削平";
                break;
            case EditorUi.ToolMode.HeightRipple:
                HiveCore.SetHeightTool(3);
                name = "高度起伏";
                break;
            case EditorUi.ToolMode.HeightSmooth:
                HiveCore.SetHeightTool(4);
                name = "高度平滑";
                break;
            case EditorUi.ToolMode.WaterAdd:
                HiveCore.SetCellTool(0);
                name = "加水";
                break;
            case EditorUi.ToolMode.WaterRemove:
                HiveCore.SetCellTool(1);
                name = "去水";
                break;
            case EditorUi.ToolMode.CliffRaise:
                HiveCore.SetCliffTool(3, null);
                name = "悬崖升一级";
                break;
            case EditorUi.ToolMode.CliffLower:
                HiveCore.SetCliffTool(1, null);
                name = "悬崖降一级";
                break;
            case EditorUi.ToolMode.CliffLevel:
                HiveCore.SetCliffTool(2, null);
                name = "悬崖抹平";
                break;
            case EditorUi.ToolMode.CliffRamp:
                HiveCore.SetCliffTool(7, null);
                name = "悬崖斜坡";
                break;
            case EditorUi.ToolMode.DoodadPlace:
                name = $"放置装饰物 {doodadType}";
                break;
            default:
                name = "?";
                break;
        }

        HiveCore.BrushSize(brushSize);
        HiveCore.BrushShape(brushShape);
        ui.SetActiveTool(name);
    }

    private void UpdateStatus()
    {
        if (!loaded)
        {
            return;
        }
        string position = hasHit ? $"{lastHit.X:F1}, {lastHit.Z:F1}" : "—";
        ui.SetStatus($"笔刷 {brushSize}　位置 {position}　水面 {view.WetTiles} 格　"
            + "左键绘制　右键拖动　滚轮缩放　Ctrl+S 保存　1-9 选纹理");
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (!loaded)
        {
            return;
        }

        switch (@event)
        {
            case InputEventMouseButton button:
                HandleMouseButton(button);
                break;
            case InputEventMouseMotion motion:
                HandleMouseMotion(motion);
                break;
            case InputEventKey key when key.Pressed && !key.Echo:
                HandleKey(key);
                break;
        }
    }

    private void HandleMouseButton(InputEventMouseButton button)
    {
        switch (button.ButtonIndex)
        {
            case MouseButton.WheelUp when button.Pressed:
                distance = Math.Max(24f, distance * 0.88f);
                UpdateCamera();
                break;
            case MouseButton.WheelDown when button.Pressed:
                distance = Math.Min(1400f, distance * 1.14f);
                UpdateCamera();
                break;
            case MouseButton.Right:
                panning = button.Pressed;
                break;
            case MouseButton.Left:
                if (button.Pressed && tool == EditorUi.ToolMode.DoodadPlace)
                {
                    if (view.Pick(camera, button.Position, out Vector3 spot))
                    {
                        PlaceDoodad(spot);
                    }
                    break;
                }

                if (button.Pressed && view.Pick(camera, button.Position, out Vector3 first))
                {
                    painting = true;
                    HiveCore.BrushMove(first.X, first.Z);
                    HiveCore.BrushBegin();
                    HiveCore.BrushApply(0.016);
                    ReloadView();
                }
                else if (!button.Pressed && painting)
                {
                    painting = false;
                    HiveCore.BrushEnd();
                    ReloadView();
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
            focus.X = Mathf.Clamp(focus.X, 0f, view.Info.TilesX);
            focus.Z = Mathf.Clamp(focus.Z, 0f, view.Info.TilesY);
            UpdateCamera();
            return;
        }

        if (!view.Pick(camera, motion.Position, out Vector3 hit))
        {
            hasHit = false;
            brushPreview.Visible = false;
            UpdateStatus();
            return;
        }

        hasHit = true;
        lastHit = hit;
        ShowBrush(hit);

        if (painting)
        {
            HiveCore.BrushMove(hit.X, hit.Z);
            HiveCore.BrushApply(0.016);
            ReloadView();
        }
    }

    private void ShowBrush(Vector3 hit)
    {
        if (brushShape == 1)
        {
            brushPreview.Visible = false;
            return;
        }

        float half = brushSize * 0.5f;
        brushPreview.Visible = true;
        brushPreview.Position = new Vector3(
            MathF.Floor(hit.X / brushSize) * brushSize + half,
            hit.Y + 0.06f,
            MathF.Floor(hit.Z / brushSize) * brushSize + half);
    }

    private void HandleKey(InputEventKey key)
    {
        switch (key.Keycode)
        {
            case Key.Z:
                Undo();
                break;
            case Key.Y:
                Redo();
                break;
            case Key.S when key.CtrlPressed:
                SaveMap();
                break;
            case Key.O when key.CtrlPressed:
                ShowOpenDialog();
                break;
            case Key.Q:
                ui.SetBrushSize(--brushSize);
                SetBrushSize(brushSize);
                break;
            case Key.E:
                ui.SetBrushSize(++brushSize);
                SetBrushSize(brushSize);
                break;
            case Key.Key1:
            case Key.Key2:
            case Key.Key3:
            case Key.Key4:
            case Key.Key5:
            case Key.Key6:
            case Key.Key7:
            case Key.Key8:
            case Key.Key9:
                int index = (int)key.Keycode - (int)Key.Key1;
                if (index < view.Info.TextureCount)
                {
                    SelectTexture(index);
                }
                break;
        }
    }

    private void PlaceDoodad(Vector3 spot)
    {
        if (doodadType.Length == 0)
        {
            return;
        }

        int index = HiveCore.AddDoodad(doodadType, spot.X, spot.Z, spot.Y, 0f);
        if (index < 0)
        {
            ui.SetStatus($"放置 {doodadType} 失败");
            return;
        }

        view.BuildDoodads();
        ui.SetStatus($"已放置 {doodadType} 于 {spot.X:F1}, {spot.Z:F1}");
    }

    private void ReloadView()
    {
        view.Reload();
        ui.RefreshMinimap(view);
        UpdateStatus();
    }

    private void Undo()
    {
        HiveCore.Undo();
        ReloadView();
    }

    private void Redo()
    {
        HiveCore.Redo();
        ReloadView();
    }

    private void SaveMap()
    {
        string outputDir = ProjectSettings.GlobalizePath("res://") + "../output";
        System.IO.Directory.CreateDirectory(outputDir);
        string target = System.IO.Path.Combine(outputDir, "edited.w3m");

        if (HiveCore.SaveMap(target, out string error))
        {
            GD.Print($"已保存：{target}");
            ui.SetStatus($"已保存到 {target}");
        }
        else
        {
            GD.PrintErr($"保存失败：{error}");
            ui.SetStatus($"保存失败：{error}");
        }
    }

    public override void _Process(double delta)
    {
        if (screenshotPath.Length == 0 || !loaded)
        {
            return;
        }

        frames++;

        if (reloadTest.Length > 0)
        {
            if (frames == 8)
            {
                string projectRoot = ProjectSettings.GlobalizePath("res://");
                LoadMap(projectRoot + "../testdata/maps/(12)IceCrown.w3m");
            }
            else if (frames == 20)
            {
                Capture(screenshotPath);
                GD.Print($"换图后：{view.Info.Width}×{view.Info.Height}，水面 {view.WetTiles} 格");
                GetTree().Quit();
            }
            return;
        }

        if (autoTest.Length == 0 && frames == 14)
        {
            Capture(screenshotPath);
            GetTree().Quit();
            return;
        }

        if (autoTest.Length > 0)
        {
            if (frames == 10)
            {
                focus = new Vector3(96f, 0, 48f);
                distance = 42f;
                UpdateCamera();
            }
            else if (frames == 14)
            {
                Capture(autoTest + "_before.png");
                RunAutoTest();
            }
            else if (frames == 24)
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
        SelectTexture(3);
        SetBrushSize(28);
        SetBrushShape(1);
        Paint(96f, 48f, 1.0);

        SelectTool(EditorUi.ToolMode.HeightRaise);
        SetBrushSize(20);
        Paint(62f, 40f, 12.0);

        SelectTool(EditorUi.ToolMode.WaterAdd);
        SetBrushSize(16);
        Paint(140f, 74f, 1.0);

        SelectTool(EditorUi.ToolMode.Texture);
        SetBrushSize(8);
        SetBrushShape(0);

        doodadType = "LTlt";
        for (int i = 0; i < 6; i++)
        {
            var spot = new Vector3(86f + i * 3f, 0f, 52f);
            spot.Y = view.HeightAt(spot.X, spot.Z);
            PlaceDoodad(spot);
        }
        GD.Print($"自动测试：水面 {view.WetTiles} 格，装饰物 {HiveCore.MapInfo.DoodadCount} 个");
        SaveMap();
    }

    private void Paint(float x, float z, double rounds)
    {
        HiveCore.BrushMove(x, z);
        HiveCore.BrushBegin();
        for (int i = 0; i < (int)rounds; i++)
        {
            HiveCore.BrushApply(0.05);
        }
        HiveCore.BrushEnd();
        ReloadView();
    }

    public override void _ExitTree()
    {
        HiveCore.Close();
    }
}
