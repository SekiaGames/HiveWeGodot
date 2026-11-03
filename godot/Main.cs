using System;
using Godot;

namespace HiveWeGodot;

public partial class Main : Node3D
{
    private const float Pitch = 0.95f;

    private Node3D root = null!;
    private string dataDir = string.Empty;
    private string currentMapPath = string.Empty;

    private TerrainView view = null!;
    private EditorUi ui = null!;
    private Camera3D camera = null!;
    private MeshInstance3D brushPreview = null!;

    private EditorUi.ToolMode tool = EditorUi.ToolMode.Texture;
    private int textureIndex;
    private int brushSize = 8;
    private int brushShape;
    private string doodadType = string.Empty;
    private string unitType = string.Empty;
    private bool selectionIsUnit;
    private int[] selection = Array.Empty<int>();
    private int[] rectBase = Array.Empty<int>();
    private bool draggingObject;
    private bool rotatingObject;
    private bool selectingRect;
    private Vector3 selectStart;
    private bool pasting;
    private bool clipboardIsUnit;

    private Vector3 focus;
    private float distance = 190f;
    private bool painting;
    private bool panning;
    private Vector3 lastHit;
    private bool hasHit;
    private bool brushVisible = true;

    private readonly string screenshotPath = OS.GetEnvironment("HIVE_SCREENSHOT");
    private readonly string autoTest = OS.GetEnvironment("HIVE_AUTOTEST");
    private readonly string reloadTest = OS.GetEnvironment("HIVE_RELOAD_TEST");
    private readonly string pickTest = OS.GetEnvironment("HIVE_PICK_TEST");
    private readonly string clipTest = OS.GetEnvironment("HIVE_CLIP_TEST");
    private readonly string unitTest = OS.GetEnvironment("HIVE_UNIT_TEST");
    private readonly string teamTest = OS.GetEnvironment("HIVE_TEAM_TEST");
    private int startPlayer;
    private int pathingMask;
    private int pathingOperation;
    private string cliffType = string.Empty;
    private readonly string animTest = OS.GetEnvironment("HIVE_ANIM_TEST");
    private readonly string modelTest = OS.GetEnvironment("HIVE_MODEL_TEST");
    private readonly string startTest = OS.GetEnvironment("HIVE_START_TEST");
    private readonly string objectPickTest = OS.GetEnvironment("HIVE_OBJECT_PICK_TEST");
    private readonly string pathingPaintTest = OS.GetEnvironment("HIVE_PATHING_PAINT_TEST");
    private readonly string playerTest = OS.GetEnvironment("HIVE_PLAYER_TEST");
    private readonly string constantTest = OS.GetEnvironment("HIVE_CONSTANT_TEST");
    private readonly string newMapTest = OS.GetEnvironment("HIVE_NEWMAP_TEST");
    private readonly string objectEditorTest = OS.GetEnvironment("HIVE_OBJECT_EDITOR_TEST");
    private readonly string jassTest = OS.GetEnvironment("HIVE_JASS_TEST");
    private readonly string protectTest = OS.GetEnvironment("HIVE_PROTECT_TEST");
    private readonly string visibleTest = OS.GetEnvironment("HIVE_VISIBLE_TEST");
    private readonly string tilesetTest = OS.GetEnvironment("HIVE_TILESET_TEST");
    private readonly string tilePathTest = OS.GetEnvironment("HIVE_TILEPATH_TEST");
    private readonly string heightmapTest = OS.GetEnvironment("HIVE_HEIGHTMAP_TEST");
    private string[] doodadTypes = Array.Empty<string>();
    private string[] unitTypes = Array.Empty<string>();
    private readonly string selectTest = OS.GetEnvironment("HIVE_SELECT_TEST");
    private readonly string objectTest = OS.GetEnvironment("HIVE_OBJECT_TEST");
    private readonly string customTest = OS.GetEnvironment("HIVE_CUSTOM_TEST");
    private readonly string infoTest = OS.GetEnvironment("HIVE_INFO_TEST");
    private readonly string resizeTest = OS.GetEnvironment("HIVE_RESIZE_TEST");
    private readonly string triggerTest = OS.GetEnvironment("HIVE_TRIGGER_TEST");
    private readonly string regionTest = OS.GetEnvironment("HIVE_REGION_TEST");
    private ModelWindow? modelWindow;
    private string customTestId = string.Empty;
    private int customTestCategory;
    private int frames;
    private bool loaded;

    private const string SettingsPath = "user://settings.cfg";

    public override void _Ready()
    {
        root = new Node3D();
        AddChild(root);

        // 开发目录里有 testdata，直接开样例地图；打包后没有，让用户选客户端目录
        string projectRoot = ProjectSettings.GlobalizePath("res://");
        string bundled = projectRoot + "../testdata/warcraft";
        if (System.IO.Directory.Exists(bundled))
        {
            dataDir = bundled;
            LoadMap(projectRoot + "../testdata/maps/(2)BootyBay.w3m");
            return;
        }

        dataDir = ReadSetting("data_dir");
        if (dataDir.Length > 0 && System.IO.Directory.Exists(dataDir))
        {
            OpenLastMapOrNew();
            return;
        }

        GD.Print("没有数据目录，等用户选魔兽争霸 III 的安装目录");
        CallDeferred(nameof(AskDataDir));
    }

    private void AskDataDir()
    {
        var dialog = new FileDialog
        {
            FileMode = FileDialog.FileModeEnum.OpenDir,
            Access = FileDialog.AccessEnum.Filesystem,
            Title = "选择魔兽争霸 III 目录（里面要有 WAR3.MPQ）",
            UseNativeDialog = true,
        };
        dialog.DirSelected += path =>
        {
            dialog.QueueFree();
            dataDir = path;
            WriteSetting("data_dir", path);
            GD.Print($"数据目录：{path}");
            OpenLastMapOrNew();
        };
        dialog.Canceled += () =>
        {
            dialog.QueueFree();
            GD.Print("没有选数据目录，可以用工具条上的「打开」选一张地图");
        };
        AddChild(dialog);
        dialog.PopupCentered(new Vector2I(900, 600));
    }

    private void OpenLastMapOrNew()
    {
        string lastMap = ReadSetting("last_map");
        if (lastMap.Length > 0 && System.IO.File.Exists(lastMap))
        {
            LoadMap(lastMap);
            return;
        }
        NewMap();
    }

    private static string ReadSetting(string key)
    {
        var config = new ConfigFile();
        if (config.Load(SettingsPath) != Error.Ok)
        {
            return string.Empty;
        }
        return config.GetValue("paths", key, string.Empty).AsString();
    }

    private static void WriteSetting(string key, string value)
    {
        var config = new ConfigFile();
        config.Load(SettingsPath);
        config.SetValue("paths", key, value);
        config.Save(SettingsPath);
    }

    private void NewMap()
    {
        if (loaded)
        {
            HiveCore.Close();
            CloseEditor();
            loaded = false;
        }

        if (!HiveCore.NewMap(dataDir, out string error))
        {
            GD.PrintErr(error);
            ui?.SetStatus($"新建失败：{error}");
            return;
        }

        loaded = true;
        ResetEditorState();
        BuildEditor();
        ui.SetStatus("空白地图 64×64，Ctrl+S 另存为 w3m");
        GD.Print($"新建地图：{view.Info.Width}×{view.Info.Height}");
    }

    private void LoadMap(string mapPath)
    {
        if (loaded)
        {
            HiveCore.Close();
            CloseEditor();
        }

        if (!HiveCore.Open(mapPath, dataDir, out string error))
        {
            GD.PrintErr(error);
            ui?.SetStatus($"打开失败：{error}");
            return;
        }

        loaded = true;
        currentMapPath = mapPath;
        WriteSetting("last_map", mapPath);
        ResetEditorState();
        BuildEditor();
        GD.Print($"已打开 {mapPath}：{view.Info.Width}×{view.Info.Height}");
    }

    private void CloseEditor()
    {
        foreach (Node child in root.GetChildren())
        {
            root.RemoveChild(child);
            child.QueueFree();
        }
    }

    private void ResetEditorState()
    {
        tool = EditorUi.ToolMode.Texture;
        textureIndex = 0;
        brushSize = 8;
        brushShape = 0;
        doodadType = string.Empty;
        unitType = string.Empty;
        selection = Array.Empty<int>();
        selectionIsUnit = false;
        pasting = false;
        draggingObject = false;
        rotatingObject = false;
        selectingRect = false;
        focus = new Vector3(HiveCore.MapInfo.TilesX * 0.5f, 0, HiveCore.MapInfo.TilesY * 0.5f);
        distance = 190f;
    }

    private void BuildEditor()
    {
        view = new TerrainView();
        root.AddChild(view);
        view.Build();
        view.BuildDoodads();
        view.BuildUnits();
        view.BuildRegions();
        view.BuildStartLocations();

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
        ui.ExportRequested += ShowExportDialog;
        ui.SaveAsRequested += ShowSaveAsDialog;
        ui.TileSetRequested += ShowTileSetterWindow;
        ui.TilePatherRequested += ShowTilePatherWindow;
        ui.ImportHeightmapRequested += ShowHeightmapDialog;
        ui.OpenRequested += ShowOpenDialog;
        ui.NewMapRequested += NewMap;
        ui.PathingToggled += view.SetPathingVisible;
        ui.VisibilityToggled += (layer, on) =>
        {
            switch (layer)
            {
                case EditorUi.ViewLayer.Units:
                    view.SetUnitsVisible(on);
                    break;
                case EditorUi.ViewLayer.Doodads:
                    view.SetDoodadsVisible(on);
                    break;
                case EditorUi.ViewLayer.Water:
                    view.SetWaterVisible(on);
                    break;
                case EditorUi.ViewLayer.Brush:
                    brushVisible = on;
                    brushPreview.Visible = on && hasHit && brushShape != 1;
                    break;
                case EditorUi.ViewLayer.Lighting:
                    view.SetLightingVisible(on);
                    break;
                case EditorUi.ViewLayer.ClickHelpers:
                    view.SetClickHelpersVisible(on);
                    break;
                case EditorUi.ViewLayer.Wireframe:
                    GetViewport().DebugDraw = on ? Viewport.DebugDrawEnum.Wireframe : Viewport.DebugDrawEnum.Disabled;
                    break;
            }
        };
        ui.CameraResetRequested += ResetCamera;
        ui.RegionsToggled += view.SetRegionsVisible;
        ui.ObjectValueChanged += (category, id, index, value) =>
        {
            ReloadObjects();
            ui.SetStatus($"已改 {id} 的属性，Ctrl+S 保存到地图");
        };
        ui.NewObjectRequested += CreateObjectFromSelection;
        ui.DeleteObjectRequested += DeleteCustomObject;
        ui.MapInfoRequested += ShowMapInfoDialog;
        ui.PlayersRequested += ShowPlayerWindow;
        ui.ConstantsRequested += ShowConstantWindow;
        ui.ObjectEditorRequested += ShowObjectEditorWindow;
        ui.DoodadRandomChanged += HiveCore.SetDoodadRandom;
        ui.TriggerRequested += () => ShowTriggerWindow();
        ui.RegionRequested += ShowRegionWindow;
        ui.ModelRequested += () => ShowModelWindow(string.Empty);
        ui.MinimapClicked += world =>
        {
            focus = new Vector3(world.X, 0, world.Y);
            UpdateCamera();
        };

        doodadTypes = HiveCore.DoodadTypes();
        ui.SetDoodadTypes(doodadTypes.Length);
        ui.DoodadPickerRequested += () => ShowObjectPicker(false);
        ui.CliffTypeSelected += id =>
        {
            cliffType = id;
            ApplyTool();
            UpdateStatus();
        };
        ui.PathingBrushRequested += (mask, operation) =>
        {
            if (HiveCore.SetPathingTool(mask, operation) != 0)
            {
                ui.SetStatus("切路径笔刷失败");
                return;
            }
            pathingMask = mask;
            pathingOperation = operation;
            tool = EditorUi.ToolMode.PathingPaint;
            ApplyTool();
            UpdateStatus();
        };
        ui.StartLocationRequested += player =>
        {
            startPlayer = player;
            tool = EditorUi.ToolMode.StartLocation;
            ApplyTool();
            UpdateStatus();
        };
        ui.StartLocationRemoveRequested += player =>
        {
            int removed = HiveCore.RemoveStartLocation(player) ? 1 : 0;
            // sloc 从单位表里删掉后，后面的下标整体前移
            selection = Array.Empty<int>();
            view.ClearSelection();
            ui.ClearObjectProperties();
            view.BuildStartLocations();
            UpdateStatus();
            ui.SetStatus(removed > 0 ? $"已移除玩家 {player + 1} 的开始点" : $"玩家 {player + 1} 没有开始点");
        };
        unitTypes = HiveCore.UnitTypes();
        ui.SetUnitTypes(unitTypes.Length);
        ui.UnitPickerRequested += () => ShowObjectPicker(true);
        cliffType = ui.CurrentCliffType();
        ui.SetBrushSize(brushSize);
        if (OS.GetEnvironment("HIVE_SHOW_PATHING").Length > 0)
        {
            ui.SetPathingChecked(true);
        }
        ApplyTool();
        UpdateStatus();
    }

    private void ShowSaveAsDialog()
    {
        string suggested = currentMapPath.Length > 0
            ? System.IO.Path.GetFileName(currentMapPath)
            : "edited.w3m";

        var dialog = new FileDialog
        {
            FileMode = FileDialog.FileModeEnum.SaveFile,
            Access = FileDialog.AccessEnum.Filesystem,
            Filters = new[] { "*.w3m, *.w3x ; 地图" },
            CurrentFile = suggested,
            UseNativeDialog = true,
        };
        dialog.FileSelected += path =>
        {
            dialog.QueueFree();
            if (HiveCore.SaveMap(path, out string error))
            {
                GD.Print($"已保存：{path}");
                ui.SetStatus($"已保存到 {path}");
                return;
            }
            GD.PrintErr($"保存失败：{error}");
            ui.SetStatus($"保存失败：{error}");
        };
        root.AddChild(dialog);
        dialog.PopupCentered(new Vector2I(900, 600));
    }

    private void ShowHeightmapDialog()
    {
        var dialog = new FileDialog
        {
            FileMode = FileDialog.FileModeEnum.OpenFile,
            Access = FileDialog.AccessEnum.Filesystem,
            Filters = new[] { "*.png, *.jpg, *.jpeg, *.tga, *.bmp ; 高度图" },
            UseNativeDialog = true,
        };
        dialog.FileSelected += path =>
        {
            dialog.QueueFree();
            Image? image = Image.LoadFromFile(path);
            if (image == null)
            {
                ui.SetStatus($"读不出 {path}");
                return;
            }
            if (!HiveCore.ImportHeightmap(image, out string error))
            {
                ui.SetStatus($"导入失败：{error}");
                return;
            }
            ReloadView();
            ui.SetStatus($"已按 {System.IO.Path.GetFileName(path)} 导入高度");
        };
        root.AddChild(dialog);
        dialog.PopupCentered(new Vector2I(900, 600));
    }

    private void ShowTilePatherWindow()
    {
        var window = new TilePatherWindow();
        root.AddChild(window);
        window.Build();
        window.Applied += () =>
        {
            view.RefreshPathing();
            ui.SetStatus("瓦片路径已更新");
        };
        window.PopupCentered(new Vector2I(600, 520));
    }

    private void ShowTileSetterWindow()
    {
        var window = new TileSetterWindow();
        root.AddChild(window);
        window.Build();
        window.Applied += () =>
        {
            CloseEditor();
            ResetEditorState();
            BuildEditor();
            ui.SetStatus("地形集已换，撤销栈已清空");
        };
        window.PopupCentered(new Vector2I(660, 620));
    }

    private void ShowExportDialog()
    {
        var window = new MapProtectionWindow();
        root.AddChild(window);
        window.Build();
        window.Confirmed += () =>
        {
            HiveProtection protection = window.Options();
            window.QueueFree();
            ShowExportPathDialog(protection);
        };
        window.PopupCentered(new Vector2I(560, 400));
    }

    private void ShowExportPathDialog(HiveProtection protection)
    {
        string suggested = currentMapPath.Length > 0
            ? System.IO.Path.GetFileNameWithoutExtension(currentMapPath) + ".w3x"
            : "exported.w3x";

        var dialog = new FileDialog
        {
            FileMode = FileDialog.FileModeEnum.SaveFile,
            Access = FileDialog.AccessEnum.Filesystem,
            Filters = new[] { "*.w3x, *.w3m ; 地图" },
            CurrentFile = suggested,
            UseNativeDialog = true,
        };
        dialog.FileSelected += path =>
        {
            dialog.QueueFree();
            if (!HiveCore.ExportMap(path, protection, out string error))
            {
                ui.SetStatus($"导出失败：{error}");
                return;
            }
            ui.SetStatus($"已导出 {path}");
        };
        root.AddChild(dialog);
        dialog.PopupCentered(new Vector2I(900, 600));
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

    private void ResetCamera()
    {
        focus = new Vector3(view.Info.TilesX * 0.5f, 0f, view.Info.TilesY * 0.5f);
        distance = 190f;
        UpdateCamera();
        UpdateStatus();
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
        if (pasting)
        {
            pasting = false;
            view.HideClipboardGhost();
        }

        string name;
        switch (tool)
        {
            case EditorUi.ToolMode.Select:
                name = "选择对象";
                break;
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
                HiveCore.SetCliffTool(3, CliffOrNull());
                name = $"悬崖升一级（{cliffType}）";
                break;
            case EditorUi.ToolMode.CliffLower:
                HiveCore.SetCliffTool(1, CliffOrNull());
                name = $"悬崖降一级（{cliffType}）";
                break;
            case EditorUi.ToolMode.CliffLevel:
                HiveCore.SetCliffTool(2, CliffOrNull());
                name = $"悬崖抹平（{cliffType}）";
                break;
            case EditorUi.ToolMode.CliffRamp:
                HiveCore.SetCliffTool(7, CliffOrNull());
                name = $"悬崖斜坡（{cliffType}）";
                break;
            case EditorUi.ToolMode.DoodadPlace:
                name = $"放置装饰物 {doodadType}";
                break;
            case EditorUi.ToolMode.UnitPlace:
                name = $"放置单位 {unitType}";
                break;
            case EditorUi.ToolMode.StartLocation:
                name = $"放置开始点（玩家 {startPlayer + 1}）";
                break;
            case EditorUi.ToolMode.PathingPaint:
                string[] flags = { "不可走", "不可飞", "不可建" };
                var picked = new System.Collections.Generic.List<string>();
                int[] bits = { 0x02, 0x04, 0x08 };
                for (int i = 0; i < 3; i++)
                {
                    if ((pathingMask & bits[i]) != 0)
                    {
                        picked.Add(flags[i]);
                    }
                }
                name = $"画路径：{(picked.Count == 0 ? "什么都不标" : string.Join("、", picked))}"
                    + $"（{new[] { "覆盖", "加", "减" }[pathingOperation]}）";
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
        string selected;
        if (selection.Length == 0)
        {
            selected = "无";
        }
        else if (selection.Length == 1)
        {
            int index = selection[0];
            selected = $"{(selectionIsUnit ? "单位" : "装饰物")} #{index} "
                + (selectionIsUnit ? HiveCore.UnitId(index) : HiveCore.DoodadId(index));
        }
        else
        {
            selected = $"{selection.Length} 个{(selectionIsUnit ? "单位" : "装饰物")}";
        }
        string hint = pasting
            ? "左键放下　Esc 取消"
            : "左键绘制　右键拖动　滚轮缩放　Ctrl+S 保存　Ctrl+C 复制　Ctrl+V 粘贴　1-9 选纹理";
        ui.SetStatus($"笔刷 {brushSize}　位置 {position}　选中 {selected}　水面 {view.WetTiles} 格　{hint}");
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
                if (pasting)
                {
                    if (button.Pressed)
                    {
                        PlaceClipboardAt(button.Position);
                    }
                    break;
                }

                if (tool == EditorUi.ToolMode.Select)
                {
                    if (button.Pressed)
                    {
                        BeginSelectAction(button.Position);
                    }
                    else
                    {
                        EndSelectAction();
                    }
                    break;
                }

                if (button.Pressed && (tool == EditorUi.ToolMode.DoodadPlace || tool == EditorUi.ToolMode.UnitPlace
                        || tool == EditorUi.ToolMode.StartLocation))
                {
                    if (view.Pick(camera, button.Position, out Vector3 spot))
                    {
                        if (tool == EditorUi.ToolMode.DoodadPlace)
                        {
                            PlaceDoodad(spot);
                        }
                        else if (tool == EditorUi.ToolMode.UnitPlace)
                        {
                            PlaceUnit(spot);
                        }
                        else
                        {
                            PlaceStartLocation(spot);
                        }
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
                    if (tool == EditorUi.ToolMode.PathingPaint)
                    {
                        view.RefreshPathing();
                    }
                    else
                    {
                        ReloadView();
                    }
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

        if (selectingRect)
        {
            UpdateRectSelection(hit);
            UpdateStatus();
            return;
        }

        if (rotatingObject && selection.Length > 0)
        {
            RotateToward(motion.Position);
            UpdateStatus();
            return;
        }

        if (draggingObject && selection.Length > 0)
        {
            HiveCore.MoveGroupTo(selectionIsUnit, hit.X, hit.Z);
            view.UpdateGroupGhost(selectionIsUnit);
            view.ShowSelection(selectionIsUnit, selection);
            UpdateStatus();
            return;
        }

        if (pasting)
        {
            view.UpdateClipboardGhost(hit.X, hit.Z);
            UpdateStatus();
            return;
        }

        ShowBrush(hit);

        if (painting)
        {
            HiveCore.BrushMove(hit.X, hit.Z);
            HiveCore.BrushApply(0.016);
            if (tool == EditorUi.ToolMode.PathingPaint)
            {
                view.RefreshPathing();
            }
            else
            {
                ReloadView();
            }
        }
    }

    private string? CliffOrNull() => cliffType.Length == 0 ? null : cliffType;

    private void ShowBrush(Vector3 hit)
    {
        if (brushShape == 1 || !brushVisible)
        {
            brushPreview.Visible = false;
            return;
        }

        brushPreview.Visible = true;
        if (tool == EditorUi.ToolMode.PathingPaint)
        {
            // 路径图一格是 1/4 tile，笔刷尺寸也按这一格算
            float cells = brushSize * 0.25f;
            float half = cells * 0.5f;
            brushPreview.Scale = new Vector3(0.25f, 1f, 0.25f);
            brushPreview.Position = new Vector3(
                MathF.Floor(hit.X * 4f / brushSize) * cells + half,
                hit.Y + 0.06f,
                MathF.Floor(hit.Z * 4f / brushSize) * cells + half);
            return;
        }

        brushPreview.Scale = Vector3.One;
        float halfSize = brushSize * 0.5f;
        brushPreview.Position = new Vector3(
            MathF.Floor(hit.X / brushSize) * brushSize + halfSize,
            hit.Y + 0.06f,
            MathF.Floor(hit.Z / brushSize) * brushSize + halfSize);
    }

    private void HandleKey(InputEventKey key)
    {
        switch (key.Keycode)
        {
            case Key.Delete:
                DeleteSelection();
                break;
            case Key.Escape:
                if (pasting)
                {
                    CancelPaste();
                }
                break;
            case Key.C when key.CtrlPressed:
                CopySelectionObjects();
                break;
            case Key.V when key.CtrlPressed:
                StartPaste();
                break;
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

    private void BeginSelectAction(Vector2 screen)
    {
        if (!view.Pick(camera, screen, out Vector3 hit))
        {
            return;
        }

        Vector3 origin = camera.ProjectRayOrigin(screen);
        Vector3 direction = camera.ProjectRayNormal(screen);
        float reach = distance * 2f;

        bool isUnit = false;
        int index = HiveCore.PickDoodad(origin, direction, reach);
        if (index < 0)
        {
            index = HiveCore.PickUnit(origin, direction, reach);
            isUnit = index >= 0;
        }

        bool shift = Input.IsKeyPressed(Key.Shift);
        bool alt = Input.IsKeyPressed(Key.Alt);

        if (index < 0)
        {
            rectBase = shift || alt ? selection : Array.Empty<int>();
            selectingRect = true;
            selectStart = hit;
            view.ShowSelectionRect(hit.X, hit.Z, hit.X, hit.Z);
            return;
        }

        if (Input.IsKeyPressed(Key.Ctrl))
        {
            if (selectionIsUnit != isUnit || Array.IndexOf(selection, index) < 0)
            {
                SetSelection(isUnit, new[] { index });
            }
            StartRotate(screen, hit);
            return;
        }

        if (shift || alt)
        {
            var list = new System.Collections.Generic.List<int>(selection);
            if (selectionIsUnit != isUnit)
            {
                list.Clear();
            }
            if (shift && !list.Contains(index))
            {
                list.Add(index);
            }
            else if (alt)
            {
                list.Remove(index);
            }
            SetSelection(isUnit, list.ToArray());
        }
        else if (selectionIsUnit != isUnit || Array.IndexOf(selection, index) < 0)
        {
            SetSelection(isUnit, new[] { index });
        }

        StartDrag(hit);
    }

    private void EndSelectAction()
    {
        if (selectingRect)
        {
            selectingRect = false;
            view.HideSelectionRect();
            return;
        }

        if (draggingObject || rotatingObject)
        {
            EndDrag();
        }
    }

    private void UpdateRectSelection(Vector3 hit)
    {
        view.ShowSelectionRect(selectStart.X, selectStart.Z, hit.X, hit.Z);

        int[] hits = HiveCore.QueryArea(selectStart.X, selectStart.Z, hit.X, hit.Z, false);
        if (rectBase.Length == 0)
        {
            SetSelection(false, hits);
            return;
        }

        bool removed = Input.IsKeyPressed(Key.Alt);
        var list = new System.Collections.Generic.List<int>(rectBase);
        foreach (int index in hits)
        {
            if (removed)
            {
                list.Remove(index);
            }
            else if (!list.Contains(index))
            {
                list.Add(index);
            }
        }
        SetSelection(false, list.ToArray());
    }

    private void StartDrag(Vector3 hit)
    {
        if (selection.Length == 0 || !HiveCore.BeginGroup(selectionIsUnit, selection, hit.X, hit.Z))
        {
            return;
        }
        draggingObject = true;
        view.ShowGroupGhost(selectionIsUnit, selection);
    }

    private void StartRotate(Vector2 screen, Vector3 hit)
    {
        if (selection.Length == 0 || !HiveCore.BeginGroup(selectionIsUnit, selection, hit.X, hit.Z))
        {
            return;
        }
        rotatingObject = true;
        view.ShowGroupGhost(selectionIsUnit, selection);
        RotateToward(screen);
    }

    private void RotateToward(Vector2 screen)
    {
        if (!view.Pick(camera, screen, out Vector3 hit))
        {
            return;
        }

        HiveCore.RotateGroupTo(selectionIsUnit, hit.X, hit.Z);
        view.UpdateGroupGhost(selectionIsUnit);
        view.ShowSelection(selectionIsUnit, selection);
    }

    private void EndDrag()
    {
        bool rotated = rotatingObject;
        draggingObject = false;
        rotatingObject = false;
        view.HideGroupGhost();
        HiveCore.EndGroup(selectionIsUnit);
        ReloadObjects();
        ui.SetStatus($"已{(rotated ? "旋转" : "移动")} {selection.Length} 个对象，Ctrl+Z 可撤销");
    }

    private void SetSelection(bool isUnit, int[] indices)
    {
        selectionIsUnit = isUnit;
        selection = indices;
        view.ShowSelection(isUnit, indices);
        ShowObjectPanel(isUnit, indices);
        UpdateStatus();
    }

    private void ShowMapInfoDialog()
    {
        var dialog = new AcceptDialog
        {
            Title = "地图信息",
            OkButtonText = "保存并关闭",
        };

        var grid = new GridContainer { Columns = 2 };
        grid.AddThemeConstantOverride("h_separation", 10);
        grid.AddThemeConstantOverride("v_separation", 8);

        var nameEdit = new LineEdit
        {
            Text = HiveCore.MapName ?? string.Empty,
            CustomMinimumSize = new Vector2(340, 0),
        };
        var authorEdit = new LineEdit { Text = HiveCore.MapAuthor ?? string.Empty };
        var suggestedEdit = new LineEdit { Text = HiveCore.MapSuggestedPlayers ?? string.Empty };
        var descriptionEdit = new TextEdit
        {
            Text = HiveCore.MapDescription ?? string.Empty,
            CustomMinimumSize = new Vector2(340, 100),
        };

        int[] versions = HiveCore.MapVersions();
        var (playableWidth, playableHeight) = HiveCore.MapPlayableSize();

        grid.AddChild(new Label { Text = "名称" });
        grid.AddChild(nameEdit);
        grid.AddChild(new Label { Text = "作者" });
        grid.AddChild(authorEdit);
        grid.AddChild(new Label { Text = "建议玩家" });
        grid.AddChild(suggestedEdit);
        grid.AddChild(new Label { Text = "描述" });
        grid.AddChild(descriptionEdit);
        grid.AddChild(new Label { Text = "尺寸" });
        grid.AddChild(new Label
        {
            Text = $"{view.Info.Width}×{view.Info.Height}，可玩 {playableWidth}×{playableHeight}",
        });
        grid.AddChild(new Label { Text = "版本" });
        grid.AddChild(new Label
        {
            Text = $"地图 {versions[0]}，编辑器 {versions[1]}，游戏 {versions[2]}.{versions[3]}.{versions[4]}.{versions[5]}",
        });

        var sizeRow = new HBoxContainer();
        sizeRow.AddThemeConstantOverride("separation", 6);
        SpinBox Delta(string prefix) => new()
        {
            Prefix = prefix,
            MinValue = -100,
            MaxValue = 100,
            Step = 1,
            Value = 0,
            CustomMinimumSize = new Vector2(88, 0),
        };
        SpinBox leftDelta = Delta("左 ");
        SpinBox rightDelta = Delta("右 ");
        SpinBox topDelta = Delta("上 ");
        SpinBox bottomDelta = Delta("下 ");
        sizeRow.AddChild(leftDelta);
        sizeRow.AddChild(rightDelta);
        sizeRow.AddChild(topDelta);
        sizeRow.AddChild(bottomDelta);

        var applySize = new Button { Text = "调整" };
        applySize.Pressed += () =>
        {
            int left = (int)leftDelta.Value;
            int right = (int)rightDelta.Value;
            int top = (int)topDelta.Value;
            int bottom = (int)bottomDelta.Value;
            if (left == 0 && right == 0 && top == 0 && bottom == 0)
            {
                return;
            }
            if (HiveCore.ResizeMap(left, right, top, bottom) != 0)
            {
                ui.SetStatus("尺寸超出 32..480 的范围");
                return;
            }
            dialog.QueueFree();
            RebuildAfterResize();
        };
        sizeRow.AddChild(applySize);

        grid.AddChild(new Label { Text = "调整尺寸" });
        grid.AddChild(sizeRow);

        (int insetLeft, int insetRight, int insetTop, int insetBottom) = HiveCore.MapPlayableInsets();
        SpinBox Inset(string prefix, int value) => new()
        {
            Prefix = prefix,
            MinValue = 0,
            MaxValue = 64,
            Step = 1,
            Value = value,
            CustomMinimumSize = new Vector2(88, 0),
        };
        SpinBox leftInset = Inset("左 ", insetLeft);
        SpinBox rightInset = Inset("右 ", insetRight);
        SpinBox topInset = Inset("上 ", insetTop);
        SpinBox bottomInset = Inset("下 ", insetBottom);

        var insetRow = new HBoxContainer();
        insetRow.AddThemeConstantOverride("separation", 6);
        insetRow.AddChild(leftInset);
        insetRow.AddChild(rightInset);
        insetRow.AddChild(topInset);
        insetRow.AddChild(bottomInset);

        var applyInsets = new Button { Text = "应用" };
        applyInsets.Pressed += () =>
        {
            if (HiveCore.SetPlayableArea((int)leftInset.Value, (int)rightInset.Value, (int)topInset.Value,
                    (int)bottomInset.Value) != 0)
            {
                ui.SetStatus("可玩区域超出地图范围");
                return;
            }
            ui.SetStatus("可玩区域已改，Ctrl+S 存进地图");
        };
        insetRow.AddChild(applyInsets);

        grid.AddChild(new Label { Text = "可玩区域" });
        grid.AddChild(insetRow);

        var tabs = new TabContainer { CustomMinimumSize = new Vector2(640, 520) };
        var descriptionPage = new ScrollContainer
        {
            Name = "描述",
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
        };
        grid.SizeFlagsHorizontal = Control.SizeFlags.ExpandFill;
        descriptionPage.AddChild(grid);
        tabs.AddChild(descriptionPage);

        Action applyOptions = BuildMapOptionsTab(tabs);
        Action applyLoadingScreen = BuildLoadingScreenTab(tabs);
        dialog.AddChild(tabs);

        dialog.Confirmed += () =>
        {
            HiveCore.SetMapName(nameEdit.Text);
            HiveCore.SetMapAuthor(authorEdit.Text);
            HiveCore.SetMapSuggestedPlayers(suggestedEdit.Text);
            HiveCore.SetMapDescription(descriptionEdit.Text);
            applyOptions();
            applyLoadingScreen();
            ui.SetStatus("地图信息已改，Ctrl+S 存进地图");
            dialog.QueueFree();
        };
        dialog.Canceled += () => dialog.QueueFree();

        root.AddChild(dialog);
        dialog.PopupCentered(new Vector2I(700, 600));
    }

    private Action BuildLoadingScreenTab(TabContainer tabs)
    {
        var page = new ScrollContainer
        {
            Name = "载入画面",
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
        };
        var column = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        column.AddThemeConstantOverride("separation", 8);
        page.AddChild(column);
        tabs.AddChild(page);

        var modeGroup = new ButtonGroup();
        var modeRow = new HBoxContainer();
        modeRow.AddThemeConstantOverride("separation", 6);
        column.AddChild(modeRow);

        int screenNumber = HiveCore.MapLoadingScreen();
        string screenModel = HiveCore.MapLoadingScreenModel() ?? string.Empty;
        int mode = screenNumber >= 0 ? 2 : (screenModel.Length > 0 ? 1 : 0);

        Button ModeButton(string text, int value)
        {
            var button = new Button
            {
                Text = text,
                ToggleMode = true,
                ButtonGroup = modeGroup,
                ButtonPressed = mode == value,
                SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            };
            modeRow.AddChild(button);
            return button;
        }

        Button defaultButton = ModeButton("默认", 0);
        Button importedButton = ModeButton("导入的模型", 1);
        Button campaignButton = ModeButton("阵营画面", 2);

        column.AddChild(new Label { Text = "导入的模型" });
        var importedBox = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        int importedCount = HiveCore.ImportedModelCount();
        for (int i = 0; i < importedCount; i++)
        {
            importedBox.AddItem(HiveCore.ImportedModel(i) ?? "?");
        }
        if (importedCount == 0)
        {
            importedBox.AddItem("地图里没有 .mdx");
        }
        for (int i = 0; i < importedCount; i++)
        {
            if ((HiveCore.ImportedModel(i) ?? string.Empty) == screenModel)
            {
                importedBox.Select(i);
            }
        }
        column.AddChild(importedBox);

        column.AddChild(new Label { Text = "阵营画面" });
        var campaignBox = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        for (int i = 0; i < HiveCore.LoadingScreenCount(); i++)
        {
            campaignBox.AddItem(HiveCore.LoadingScreenName(i) ?? "?");
        }
        campaignBox.Select(Math.Clamp(screenNumber, 0, Math.Max(0, campaignBox.ItemCount - 1)));
        column.AddChild(campaignBox);

        void SyncEnabled()
        {
            importedBox.Disabled = !importedButton.ButtonPressed;
            campaignBox.Disabled = !campaignButton.ButtonPressed;
        }
        defaultButton.Toggled += _ => SyncEnabled();
        importedButton.Toggled += _ => SyncEnabled();
        campaignButton.Toggled += _ => SyncEnabled();
        SyncEnabled();

        column.AddChild(new Label { Text = "标题" });
        var titleEdit = new LineEdit { Text = HiveCore.MapLoadingScreenTitle() ?? string.Empty };
        column.AddChild(titleEdit);

        column.AddChild(new Label { Text = "副标题" });
        var subtitleEdit = new LineEdit { Text = HiveCore.MapLoadingScreenSubtitle() ?? string.Empty };
        column.AddChild(subtitleEdit);

        column.AddChild(new Label { Text = "提示文本" });
        var textEdit = new TextEdit
        {
            Text = HiveCore.MapLoadingScreenText() ?? string.Empty,
            CustomMinimumSize = new Vector2(0, 140),
        };
        column.AddChild(textEdit);

        return () =>
        {
            if (importedButton.ButtonPressed)
            {
                HiveCore.SetMapLoadingScreen(-1);
                HiveCore.SetMapLoadingScreenModel(importedCount > 0 ? HiveCore.ImportedModel(importedBox.Selected) ?? string.Empty : string.Empty);
            }
            else if (campaignButton.ButtonPressed)
            {
                HiveCore.SetMapLoadingScreenModel(string.Empty);
                HiveCore.SetMapLoadingScreen(Math.Max(0, campaignBox.Selected));
            }
            else
            {
                HiveCore.SetMapLoadingScreenModel(string.Empty);
                HiveCore.SetMapLoadingScreen(-1);
            }

            HiveCore.SetMapLoadingScreenTitle(titleEdit.Text);
            HiveCore.SetMapLoadingScreenSubtitle(subtitleEdit.Text);
            HiveCore.SetMapLoadingScreenText(textEdit.Text);
        };
    }

    private Action BuildMapOptionsTab(TabContainer tabs)
    {
        var page = new ScrollContainer
        {
            Name = "选项",
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
        };
        var column = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        column.AddThemeConstantOverride("separation", 8);
        page.AddChild(column);
        tabs.AddChild(page);

        int flags = HiveCore.MapOptionFlags();
        var flagBoxes = new System.Collections.Generic.List<(CheckBox Box, int Bit)>();
        var flagGrid = new GridContainer { Columns = 2 };
        flagGrid.AddThemeConstantOverride("h_separation", 12);
        column.AddChild(flagGrid);
        foreach ((string label, int bit) in new[]
                 {
                     ("近战地图", 0x01), ("隐藏小地图预览", 0x02), ("遮蔽区域半可见", 0x04),
                     ("悬崖岸边水波", 0x08), ("缓坡岸边水波", 0x10), ("物品分类系统", 0x20), ("水面着色", 0x40),
                 })
        {
            var box = new CheckBox { Text = label, ButtonPressed = (flags & bit) != 0 };
            flagGrid.AddChild(box);
            flagBoxes.Add((box, bit));
        }

        var (fogStyle, fogStart, fogEnd, fogDensity, fogRed, fogGreen, fogBlue) = HiveCore.MapFog();
        var fogGrid = new GridContainer { Columns = 2 };
        fogGrid.AddThemeConstantOverride("h_separation", 12);
        column.AddChild(fogGrid);

        fogGrid.AddChild(new Label { Text = "地形雾" });
        var fogStyleBox = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        foreach (string text in new[] { "无", "线性", "指数 1", "指数 2" })
        {
            fogStyleBox.AddItem(text);
        }
        fogStyleBox.Select(Math.Clamp(fogStyle, 0, 3));
        fogGrid.AddChild(fogStyleBox);

        fogGrid.AddChild(new Label { Text = "雾起始高度 / 结束高度 / 浓度" });
        var fogRow = new HBoxContainer();
        fogRow.AddThemeConstantOverride("separation", 6);
        SpinBox FogSpin(float value, double min, double max, double step) => new()
        {
            MinValue = min,
            MaxValue = max,
            Step = step,
            Value = value,
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
        };
        SpinBox fogStartBox = FogSpin(fogStart, -10000, 10000, 100);
        SpinBox fogEndBox = FogSpin(fogEnd, -10000, 10000, 100);
        SpinBox fogDensityBox = FogSpin(fogDensity, 0, 10, 0.01);
        fogRow.AddChild(fogStartBox);
        fogRow.AddChild(fogEndBox);
        fogRow.AddChild(fogDensityBox);
        fogGrid.AddChild(fogRow);

        fogGrid.AddChild(new Label { Text = "雾颜色" });
        var fogColor = new ColorPickerButton
        {
            Color = new Color(fogRed / 255f, fogGreen / 255f, fogBlue / 255f),
            EditAlpha = false,
        };
        fogGrid.AddChild(fogColor);

        var weatherIds = new System.Collections.Generic.List<string>();
        fogGrid.AddChild(new Label { Text = "全局天气" });
        var weatherBox = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        weatherBox.AddItem("无");
        weatherIds.Add(string.Empty);
        for (int i = 0; i < HiveCore.WeatherCount(); i++)
        {
            weatherBox.AddItem(HiveCore.WeatherName(i) ?? HiveCore.WeatherId(i) ?? "?");
            weatherIds.Add(HiveCore.WeatherId(i) ?? string.Empty);
        }
        weatherBox.Select(Math.Max(0, weatherIds.IndexOf(HiveCore.MapWeatherId() ?? string.Empty)));
        fogGrid.AddChild(weatherBox);

        var environmentIds = new System.Collections.Generic.List<string>();
        fogGrid.AddChild(new Label { Text = "环境音效" });
        var environmentBox = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        environmentBox.AddItem("无");
        environmentIds.Add(string.Empty);
        for (int i = 0; i < HiveCore.AmbientCount(); i++)
        {
            environmentBox.AddItem(HiveCore.AmbientName(i) ?? HiveCore.AmbientId(i) ?? "?");
            environmentIds.Add(HiveCore.AmbientId(i) ?? string.Empty);
        }
        environmentBox.Select(Math.Max(0, environmentIds.IndexOf(HiveCore.MapSoundEnvironment() ?? string.Empty)));
        fogGrid.AddChild(environmentBox);

        var tilesetKeys = new System.Collections.Generic.List<int>();
        fogGrid.AddChild(new Label { Text = "自定义光照地形集" });
        var lightBox = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        lightBox.AddItem("跟随地图");
        tilesetKeys.Add(0);
        for (int i = 0; i < HiveCore.TilesetCount(); i++)
        {
            lightBox.AddItem(HiveCore.TilesetName(i) ?? HiveCore.TilesetKey(i) ?? "?");
            tilesetKeys.Add((HiveCore.TilesetKey(i) ?? "\0")[0]);
        }
        lightBox.Select(Math.Max(0, tilesetKeys.IndexOf(HiveCore.MapCustomLightTileset())));
        fogGrid.AddChild(lightBox);

        fogGrid.AddChild(new Label { Text = "自定义氛围地形集" });
        var ambienceBox = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        ambienceBox.AddItem("跟随地图");
        for (int i = 1; i < lightBox.ItemCount; i++)
        {
            ambienceBox.AddItem(lightBox.GetItemText(i));
        }
        ambienceBox.Select(Math.Max(0, tilesetKeys.IndexOf(HiveCore.MapCustomAmbienceTileset())));
        fogGrid.AddChild(ambienceBox);

        var (waterRed, waterGreen, waterBlue, waterAlpha) = HiveCore.MapWaterColor();
        fogGrid.AddChild(new Label { Text = "水面颜色" });
        var waterColor = new ColorPickerButton
        {
            Color = new Color(waterRed / 255f, waterGreen / 255f, waterBlue / 255f, waterAlpha / 255f),
        };
        fogGrid.AddChild(waterColor);

        fogGrid.AddChild(new Label { Text = "游戏数据版本" });
        var dataSetBox = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        foreach (string text in new[] { "默认（按近战状态）", "自定义（TFT 1.02, RoC 1.01）", "近战（最新补丁）" })
        {
            dataSetBox.AddItem(text);
        }
        dataSetBox.Select(Math.Clamp(HiveCore.MapGameDataSet(), 0, 2));
        fogGrid.AddChild(dataSetBox);

        return () =>
        {
            int mask = 0;
            foreach ((CheckBox box, int bit) in flagBoxes)
            {
                if (box.ButtonPressed)
                {
                    mask |= bit;
                }
            }
            HiveCore.SetMapOptionFlags(mask);

            Color fog = fogColor.Color;
            HiveCore.SetMapFog(fogStyleBox.Selected, (float)fogStartBox.Value, (float)fogEndBox.Value,
                (float)fogDensityBox.Value, (int)(fog.R * 255f), (int)(fog.G * 255f), (int)(fog.B * 255f));
            HiveCore.SetMapWeatherId(weatherIds[Math.Max(0, weatherBox.Selected)]);
            HiveCore.SetMapSoundEnvironment(environmentIds[Math.Max(0, environmentBox.Selected)]);
            HiveCore.SetMapCustomLightTileset(tilesetKeys[Math.Max(0, lightBox.Selected)]);
            HiveCore.SetMapCustomAmbienceTileset(tilesetKeys[Math.Max(0, ambienceBox.Selected)]);

            Color water = waterColor.Color;
            HiveCore.SetMapWaterColor((int)(water.R * 255f), (int)(water.G * 255f), (int)(water.B * 255f),
                (int)(water.A * 255f));
            HiveCore.SetMapGameDataSet(dataSetBox.Selected);
        };
    }

    private TriggerWindow ShowTriggerWindow()
    {
        var window = new TriggerWindow();
        root.AddChild(window);
        window.Build();
        window.PopupCentered(new Vector2I(860, 600));
        return window;
    }

    private void ShowRegionWindow()
    {
        var window = new RegionWindow();
        root.AddChild(window);
        window.RegionsChanged += () =>
        {
            view.BuildRegions();
            ui.SetStatus("区域已改，Ctrl+S 存进地图");
        };
        window.Build(() => new Vector2(focus.X, focus.Z));
        window.PopupCentered(new Vector2I(520, 520));
    }

    private void ShowObjectEditorWindow()
    {
        var window = new ObjectEditorWindow();
        root.AddChild(window);
        window.Build();
        window.PopupCentered(new Vector2I(1020, 690));
    }

    private void ShowConstantWindow()
    {
        var window = new GameplayConstantWindow();
        root.AddChild(window);
        window.Build();
        window.PopupCentered(new Vector2I(800, 660));
    }

    private void ShowPlayerWindow()
    {
        var window = new PlayerWindow();
        root.AddChild(window);
        window.Build();
        window.PopupCentered(new Vector2I(760, 620));
        ui.SetStatus("玩家设置改完记得应用");
    }

    private ModelWindow ShowModelWindow(string path)
    {
        var window = new ModelWindow();
        root.AddChild(window);
        window.Build(view, path);
        window.PopupCentered(new Vector2I(760, 680));
        return window;
    }

    private void RebuildAfterResize()
    {
        CloseEditor();
        ResetEditorState();
        BuildEditor();
        ui.SetStatus($"地图尺寸已改为 {view.Info.Width}×{view.Info.Height}，撤销栈已清空");
    }

    private void CreateObjectFromSelection()
    {
        if (ui.CurrentObject is not { } current)
        {
            return;
        }

        string suggested = HiveCore.UniqueObjectId(current.Id) ?? string.Empty;

        var dialog = new ConfirmationDialog
        {
            Title = $"用 {current.Id} 复制出新对象",
            OkButtonText = "创建",
            CancelButtonText = "取消",
        };

        var column = new VBoxContainer();
        column.AddChild(new Label { Text = "新对象 ID（4 个字符）" });
        var edit = new LineEdit { Text = suggested, MaxLength = 4 };
        column.AddChild(edit);
        dialog.AddChild(column);

        dialog.Confirmed += () =>
        {
            string newId = edit.Text.Trim();
            if (newId.Length != 4)
            {
                ui.SetStatus("ID 必须是 4 个字符");
            }
            else if (!HiveCore.CreateObject(current.Category, current.Id, newId))
            {
                ui.SetStatus($"创建 {newId} 失败，ID 可能已被占用");
            }
            else
            {
                RefreshPickers();
                ui.ShowObjectProperties(current.Category, newId, $"自定义 {newId}（原型 {current.Id}）");
                ui.SetStatus($"已创建 {newId}，可从左侧下拉放置");
            }
            dialog.QueueFree();
        };
        dialog.Canceled += () => dialog.QueueFree();

        root.AddChild(dialog);
        dialog.PopupCentered(new Vector2I(360, 180));
    }

    private void DeleteCustomObject()
    {
        if (ui.CurrentObject is not { } current)
        {
            return;
        }

        if (!HiveCore.IsCustomObject(current.Category, current.Id))
        {
            ui.SetStatus($"{current.Id} 是基础对象，删不掉");
            return;
        }

        if (!HiveCore.DeleteObject(current.Category, current.Id))
        {
            ui.SetStatus("删除失败");
            return;
        }

        RefreshPickers();
        ui.ClearObjectProperties();
        ReloadObjects();
        ui.SetStatus($"已删除 {current.Id}");
    }

    private void RefreshPickers()
    {
        doodadTypes = HiveCore.DoodadTypes();
        unitTypes = HiveCore.UnitTypes();
        ui.SetDoodadTypes(doodadTypes.Length);
        ui.SetUnitTypes(unitTypes.Length);
    }

    private void ShowObjectPanel(bool isUnit, int[] indices)
    {
        if (indices.Length != 1)
        {
            ui.ClearObjectProperties();
            return;
        }

        int category = 0;
        string? id = HiveCore.UnitId(indices[0]);
        if (!isUnit)
        {
            id = HiveCore.DoodadId(indices[0]);
            category = id == null ? -1 : HiveCore.DoodadCategory(id);
        }

        if (id == null || category < 0)
        {
            ui.ClearObjectProperties();
            return;
        }

        ui.ShowObjectProperties(category, id, $"{(isUnit ? "单位" : "装饰物")} {id}");
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

    private void ShowObjectPicker(bool isUnit)
    {
        string[] ids = isUnit ? unitTypes : doodadTypes;
        if (ids.Length == 0)
        {
            ui.SetStatus("没有可放置的对象");
            return;
        }

        var picker = new ObjectPicker();
        root.AddChild(picker);
        picker.Build(isUnit, ids, id =>
        {
            if (isUnit)
            {
                unitType = id;
                ui.SetUnitChoice(id, HiveCore.ObjectTypeName(0, id) ?? id);
                tool = EditorUi.ToolMode.UnitPlace;
            }
            else
            {
                doodadType = id;
                int category = HiveCore.DoodadCategory(id);
                ui.SetDoodadChoice(id, category < 0 ? id : HiveCore.ObjectTypeName(category, id) ?? id);
                tool = EditorUi.ToolMode.DoodadPlace;
            }
            ApplyTool();
            UpdateStatus();
        });
        picker.PopupCentered(new Vector2I(780, 580));
    }

    private void PlaceStartLocation(Vector3 spot)
    {
        if (!HiveCore.SetStartLocation(startPlayer, spot.X, spot.Z))
        {
            ui.SetStatus($"放置玩家 {startPlayer + 1} 的开始点失败");
            return;
        }

        view.BuildStartLocations();
        ui.SetStatus($"玩家 {startPlayer + 1} 的开始点放在 {spot.X:F1}, {spot.Z:F1}");
    }

    private void PlaceUnit(Vector3 spot)
    {
        if (unitType.Length == 0)
        {
            return;
        }

        int index = HiveCore.AddUnit(unitType, spot.X, spot.Z, 0f, 0);
        if (index < 0)
        {
            ui.SetStatus($"放置 {unitType} 失败");
            return;
        }

        view.BuildUnits();
        ui.SetStatus($"已放置 {unitType} 于 {spot.X:F1}, {spot.Z:F1}");
    }

    private void ReloadView()
    {
        view.Reload();
        view.BuildStartLocations();
        ui.RefreshMinimap(view);
        UpdateStatus();
    }

    private void DeleteSelection()
    {
        if (selection.Length == 0)
        {
            return;
        }

        int removed = HiveCore.RemoveSelection(selectionIsUnit, selection);
        if (removed == 0)
        {
            ui.SetStatus("删除失败");
            return;
        }

        SetSelection(selectionIsUnit, Array.Empty<int>());
        ReloadObjects();
        ui.SetStatus($"已删除 {removed} 个，Ctrl+Z 可撤销");
    }

    private void CopySelectionObjects()
    {
        if (selection.Length == 0)
        {
            ui.SetStatus("没有选中对象");
            return;
        }

        int count = selectionIsUnit
            ? HiveCore.CopyUnitSelection(selection)
            : HiveCore.CopySelection(selection);

        if (count == 0)
        {
            ui.SetStatus("复制失败");
            return;
        }

        clipboardIsUnit = selectionIsUnit;
        ui.SetStatus($"已复制 {count} 个{(selectionIsUnit ? "单位" : "装饰物")}，Ctrl+V 放置");
    }

    private void StartPaste()
    {
        int count = clipboardIsUnit ? HiveCore.UnitClipboardCount : HiveCore.ClipboardCount;
        if (count == 0)
        {
            ui.SetStatus("剪贴板是空的");
            return;
        }

        pasting = true;
        view.BeginClipboardGhost(clipboardIsUnit);
        ui.SetStatus($"放置 {count} 个{(clipboardIsUnit ? "单位" : "装饰物")}：左键放下，Esc 取消");
    }

    private void CancelPaste()
    {
        pasting = false;
        view.HideClipboardGhost();
        UpdateStatus();
    }

    private void PlaceClipboardAt(Vector2 screen)
    {
        if (!view.Pick(camera, screen, out Vector3 spot))
        {
            return;
        }

        bool isUnit = clipboardIsUnit;
        int count = isUnit ? HiveCore.UnitClipboardCount : HiveCore.ClipboardCount;
        int first = isUnit
            ? HiveCore.PlaceUnitClipboard(spot.X, spot.Z)
            : HiveCore.PlaceClipboard(spot.X, spot.Z);

        pasting = false;
        view.HideClipboardGhost();

        if (first < 0)
        {
            ui.SetStatus("放置失败");
            return;
        }

        selection = Array.Empty<int>();
        view.ClearSelection();
        ReloadObjects();
        ui.SetStatus($"已放置 {count} 个{(isUnit ? "单位" : "装饰物")}，Ctrl+Z 可撤销");
    }

    private void ReloadObjects()
    {
        view.BuildDoodads();
        view.BuildUnits();
        view.RefreshPathing();
        UpdateStatus();
    }

    private void Undo()
    {
        HiveCore.Undo();
        ReloadView();
        ReloadObjects();
    }

    private void Redo()
    {
        HiveCore.Redo();
        ReloadView();
        ReloadObjects();
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

        if (regionTest.Length > 0)
        {
            if (frames == 8)
            {
                int index = HiveCore.CreateRegion(60f, 30f, 80f, 50f);
                GD.Print($"新建区域 {index}，名字 {HiveCore.RegionName(index)}，总数 {HiveCore.MapInfo.RegionCount}");
                view.BuildRegions();
                focus = new Vector3(70f, 0f, 40f);
                distance = 50f;
                UpdateCamera();
            }
            else if (frames == 16)
            {
                Capture(screenshotPath);
                GetTree().Quit();
            }
            return;
        }

        if (triggerTest.Length > 0)
        {
            if (frames == 8)
            {
                HiveCore.AddTriggerNode(0, 2, "IfThenElseMultiple", -1, 0);
                int nodes = HiveCore.TriggerBuildNodes(0);
                int ifNode = nodes - 1;
                HiveCore.AddTriggerNode(0, 2, "CommentString", ifNode, 1);
                GD.Print($"节点 {HiveCore.TriggerBuildNodes(0)} 个，If/Then/Else 分组种类 "
                    + $"{HiveCore.TriggerNodeGroupKind(0, ifNode)}");

                TriggerWindow window = ShowTriggerWindow();
                window.SelectFirstEca();
            }
            else if (frames == 16)
            {
                Capture(screenshotPath);
                GD.Print($"触发器 {HiveCore.TriggerCount} 个，分类 {HiveCore.TriggerCategoryCount} 个，"
                    + $"变量 {HiveCore.TriggerVariableCount} 个");
                GetTree().Quit();
            }
            return;
        }

        if (resizeTest.Length > 0)
        {
            if (frames == 8)
            {
                GD.Print($"调整前 {view.Info.Width}×{view.Info.Height}，装饰物 {HiveCore.MapInfo.DoodadCount}");
                int result = HiveCore.ResizeMap(10, 10, 10, 10);
                GD.Print($"调整返回 {result}");
                RebuildAfterResize();
                GD.Print($"调整后 {view.Info.Width}×{view.Info.Height}，装饰物 {HiveCore.MapInfo.DoodadCount}");
                focus = new Vector3(view.Info.TilesX * 0.5f, 0f, view.Info.TilesY * 0.5f);
                distance = 260f;
                UpdateCamera();
            }
            else if (frames == 16)
            {
                Capture(resizeTest);
                GetTree().Quit();
            }
            return;
        }

        if (infoTest.Length > 0)
        {
            if (frames == 8)
            {
                ShowMapInfoDialog();
            }
            else if (frames == 14)
            {
                var (left, right, top, bottom) = HiveCore.MapPlayableInsets();
                GD.Print($"地图信息：{HiveCore.MapName}，可玩 {HiveCore.MapPlayableSize()}，四边 {left}/{right}/{top}/{bottom}");
                HiveCore.SetPlayableArea(left + 2, right + 2, top + 2, bottom + 2);
                GD.Print($"加 2 格后可玩 {HiveCore.MapPlayableSize()}");
                Capture(screenshotPath);
                GetTree().Quit();
            }
            return;
        }

        if (customTest.Length > 0)
        {
            if (frames == 8)
            {
                focus = new Vector3(27f, 0f, 17.5f);
                distance = 20f;
                UpdateCamera();
                SetSelection(false, new[] { 0 });

                string source = HiveCore.DoodadId(0) ?? string.Empty;
                customTestCategory = HiveCore.DoodadCategory(source);
                customTestId = HiveCore.UniqueObjectId(source) ?? string.Empty;

                bool created = HiveCore.CreateObject(customTestCategory, source, customTestId);
                GD.Print($"从 {source} 建 {customTestId}：{created}，字段 {HiveCore.ObjectFieldCount(customTestCategory, customTestId)}");
                RefreshPickers();
                ui.ShowObjectProperties(customTestCategory, customTestId, $"自定义 {customTestId}（原型 {source}）");
            }
            else if (frames == 14)
            {
                Capture(screenshotPath);
            }
            else if (frames == 20)
            {
                bool deleted = HiveCore.DeleteObject(customTestCategory, customTestId);
                GD.Print($"删掉自定义对象 {customTestId}：{deleted}，剩 {HiveCore.ObjectFieldCount(customTestCategory, customTestId)} 个字段");
                ui.ClearObjectProperties();
                GetTree().Quit();
            }
            return;
        }

        if (objectTest.Length > 0)
        {
            if (frames == 8)
            {
                focus = new Vector3(27f, 0f, 17.5f);
                distance = 20f;
                UpdateCamera();
                SetSelection(false, new[] { 0 });

                string id = HiveCore.DoodadId(0) ?? string.Empty;
                int category = HiveCore.DoodadCategory(id);
                int count = HiveCore.ObjectFieldCount(category, id);
                GD.Print($"属性面板：{id} 分类 {category}，{count} 个字段");

                for (int i = 0; i < count; i++)
                {
                    if (HiveCore.ObjectFieldKey(category, i) == "bfxr")
                    {
                        string before = HiveCore.ObjectValue(category, id, i) ?? string.Empty;
                        HiveCore.SetObjectValue(category, id, i, "45");
                        GD.Print($"固定旋转 {before} -> {HiveCore.ObjectValue(category, id, i)}，"
                            + $"改过 {HiveCore.ObjectValueModified(category, id, i)}");
                        break;
                    }
                }

                ReloadObjects();
                SetSelection(false, new[] { 0 });
            }
            else if (frames == 14)
            {
                Capture(screenshotPath);
            }
            else if (frames == 20)
            {
                Capture(screenshotPath);
                GetTree().Quit();
            }
            return;
        }

        if (selectTest.Length > 0)
        {
            if (frames == 8)
            {
                focus = new Vector3(40f, 0f, 25f);
                distance = 62f;
                UpdateCamera();

                Vector3 empty = new(102f, view.HeightAt(102f, 74f), 74f);
                BeginSelectAction(camera.UnprojectPosition(empty));
                GD.Print($"空白处按下开始框选：{selectingRect}");

                UpdateRectSelection(new Vector3(20f, view.HeightAt(20f, 10f), 10f));
                EndSelectAction();
                GD.Print($"框选命中 {selection.Length} 个装饰物");
            }
            else if (frames == 14)
            {
                Capture(screenshotPath);
                int before = HiveCore.MapInfo.DoodadCount;
                int selected = selection.Length;
                DeleteSelection();
                int after = HiveCore.MapInfo.DoodadCount;
                HiveCore.Undo();
                ReloadObjects();
                GD.Print($"批量删除 {selected} 个：{before} -> {after} -> 撤销 {HiveCore.MapInfo.DoodadCount}");
            }
            else if (frames == 20)
            {
                Capture(selectTest);
                GetTree().Quit();
            }
            return;
        }

        if (jassTest.Length > 0)
        {
            if (frames == 8)
            {
                const string sample = "globals\n    constant integer MAX = 3\nendglobals\n"
                    + "native CreateUnit takes player id, integer unitid, real x, real y, real face returns unit\n"
                    + "function Init takes nothing returns nothing\n    local unit u = CreateUnit(null, 0, 0, 0, 0)\n"
                    + "    // 注释\n    call KillUnit(u)\nendfunction\n";
                CodeHighlighter highlighter = JassHighlighter.Build(sample);
                GD.Print($"JASS 高亮：call 有没有颜色 {highlighter.HasKeywordColor("call")}，"
                    + $"integer {highlighter.HasKeywordColor("integer")}，"
                    + $"CreateUnit {highlighter.HasKeywordColor("CreateUnit")}，"
                    + $"Init {highlighter.HasKeywordColor("Init")}，"
                    + $"MAX {highlighter.HasKeywordColor("MAX")}");
                GD.Print($"CreateUnit 颜色 {highlighter.GetKeywordColor("CreateUnit")}，"
                    + $"MAX 颜色 {highlighter.GetKeywordColor("MAX")}");
                string[] api = JassHighlighter.ApiDeclarations();
                GD.Print($"JASS 补全 {api.Length} 条，CreateUnit 在列 {Array.Exists(api, e => e.StartsWith("CreateUnit("))}，"
                    + $"GetTriggeringTrigger 在列 {Array.Exists(api, e => e.StartsWith("GetTriggeringTrigger("))}，"
                    + $"bj_MAX_PLAYERS 在列 {Array.Exists(api, e => e == "bj_MAX_PLAYERS")}");
                GetTree().Quit();
            }
            return;
        }

        if (objectEditorTest.Length > 0)
        {
            if (frames == 8)
            {
                var window = new ObjectEditorWindow();
                root.AddChild(window);
                window.Build();
                GD.Print($"对象编辑器 {window.CategoryCount} 类，共 {window.TotalObjects()} 个对象");
                GetTree().Quit();
            }
            return;
        }

        if (newMapTest.Length > 0)
        {
            if (frames == 6)
            {
                NewMap();
            }
            else if (frames == 12)
            {
                GD.Print($"新建后：{view.Info.Width}×{view.Info.Height}，装饰物 {HiveCore.MapInfo.DoodadCount}，"
                    + $"单位 {HiveCore.MapInfo.UnitCount}，区域 {HiveCore.MapInfo.RegionCount}");
                int placed = HiveCore.AddDoodad("LTlt", 32f, 32f, 0f, 0f);
                GD.Print($"在新图上放树返回 {placed}");
                view.BuildDoodads();
                GD.Print($"放完装饰物 {HiveCore.MapInfo.DoodadCount}");
                GetTree().Quit();
            }
            return;
        }

        if (constantTest.Length > 0)
        {
            if (frames == 8)
            {
                var window = new GameplayConstantWindow();
                root.AddChild(window);
                window.Build();
                GD.Print($"游戏常量窗口 {window.RowCount} 条");
                window.SetFilter("敏捷");
                GD.Print($"筛「敏捷」剩 {window.VisibleRows()} 条");
                window.SetFilter(string.Empty);
                GD.Print($"清空筛选剩 {window.VisibleRows()} 条");
                GetTree().Quit();
            }
            return;
        }

        if (playerTest.Length > 0)
        {
            if (frames == 8)
            {
                var window = new PlayerWindow();
                root.AddChild(window);
                window.Build();
                GD.Print($"玩家窗口建好，槽位 0 控制器 {HiveCore.PlayerSlotType(0)} 名字 {HiveCore.PlayerSlotName(0)}");
                HiveCore.SetPlayerSlot(3, 1, 3, "测试电脑", 1);
                GD.Print($"加槽位 3：控制器 {HiveCore.PlayerSlotType(3)} 种族 {HiveCore.PlayerSlotRace(3)} "
                    + $"固定 {HiveCore.PlayerSlotFixed(3)} 名字 {HiveCore.PlayerSlotName(3)}");
                window.RestoreDefaults();
                window.Apply();
                GD.Print($"恢复默认并应用后：槽位 0 控制器 {HiveCore.PlayerSlotType(0)} 名字 {HiveCore.PlayerSlotName(0)}，"
                    + $"槽位 3 控制器 {HiveCore.PlayerSlotType(3)}");
                GD.Print($"w3i 里还剩 {HiveCore.PlayerCount()} 条");
                GetTree().Quit();
            }
            return;
        }

        if (pathingPaintTest.Length > 0)
        {
            if (frames == 8)
            {
                int changed = PathingBlocked();
                GD.Print($"画之前不可走格 {changed}");
                HiveCore.SetPathingTool(0x02 | 0x08, 1);
                HiveCore.BrushSize(8);
                HiveCore.BrushMove(96f, 48f);
                HiveCore.BrushBegin();
                HiveCore.BrushApply(1.0);
                HiveCore.BrushEnd();
                view.RefreshPathing();
                GD.Print($"画之后不可走格 {PathingBlocked()}");
                HiveCore.Undo();
                view.RefreshPathing();
                GD.Print($"撤销后不可走格 {PathingBlocked()}");
                GetTree().Quit();
            }
            return;
        }

        if (objectPickTest.Length > 0)
        {
            if (frames == 8)
            {
                var picker = new ObjectPicker();
                root.AddChild(picker);
                picker.Build(true, unitTypes, _ => { });
                GD.Print($"单位选择器 {picker.EntryCount} 条");
                for (int i = 0; i < 3; i++)
                {
                    GD.Print($"  {picker.Describe(i)}");
                }
                picker.SetFilter("步兵");
                GD.Print($"筛选「步兵」剩 {picker.ShownCount} 条");
                picker.SetFilter(string.Empty);
                GD.Print($"清空筛选剩 {picker.ShownCount} 条");

                var doodads = new ObjectPicker();
                root.AddChild(doodads);
                doodads.Build(false, doodadTypes, _ => { });
                GD.Print($"装饰物选择器 {doodads.EntryCount} 条");
                for (int i = 0; i < 3; i++)
                {
                    GD.Print($"  {doodads.Describe(i)}");
                }
                doodads.SetFilter("log");
                GD.Print($"筛选「log」剩 {doodads.ShownCount} 条");
                GetTree().Quit();
            }
            return;
        }

        if (startTest.Length > 0)
        {
            if (frames == 8)
            {
                DumpStartLocations("放置前");
                HiveCore.SetStartLocation(2, 40f, 30f);
                view.BuildStartLocations();
                DumpStartLocations("放置玩家 3 后");
            }
            else if (frames == 14)
            {
                HiveCore.RemoveStartLocation(2);
                view.BuildStartLocations();
                DumpStartLocations("移除后");
                GetTree().Quit();
            }
            return;
        }

        if (heightmapTest.Length > 0)
        {
            if (frames == 8)
            {
                HiveMapInfo info = HiveCore.MapInfo;
                float[] before = HiveCore.Heights();
                float minBefore = before[0];
                float maxBefore = before[0];
                foreach (float value in before)
                {
                    minBefore = MathF.Min(minBefore, value);
                    maxBefore = MathF.Max(maxBefore, value);
                }

                byte[] pixels = new byte[info.Width * info.Height * 4];
                for (int i = 0; i < pixels.Length; i += 4)
                {
                    pixels[i] = 128;
                    pixels[i + 1] = 128;
                    pixels[i + 2] = 128;
                    pixels[i + 3] = 255;
                }
                using Image image = Image.CreateFromData(info.Width, info.Height, false, Image.Format.Rgba8, pixels);
                bool ok = HiveCore.ImportHeightmap(image, out string error);
                GD.Print($"高度图导入 {info.Width}×{info.Height}：{ok}（{error}）");

                float[] after = HiveCore.Heights();
                int changed = 0;
                for (int i = 0; i < before.Length && i < after.Length; i++)
                {
                    if (MathF.Abs(before[i] - after[i]) > 0.001f)
                    {
                        changed++;
                    }
                }
                GD.Print($"导入前高度 {minBefore:F2}..{maxBefore:F2}，导入后变了 {changed} / {after.Length} 个顶点");
                GetTree().Quit();
            }
            return;
        }

        if (tilePathTest.Length > 0)
        {
            if (frames == 8)
            {
                var window = new TilePatherWindow();
                root.AddChild(window);
                window.Build();
                GD.Print($"瓦片路径窗口：{window.Describe()}");
                window.QueueFree();
                GetTree().Quit();
            }
            return;
        }

        if (tilesetTest.Length > 0)
        {
            if (frames == 8)
            {
                var window = new TileSetterWindow();
                root.AddChild(window);
                window.Build();
                GD.Print($"地形集窗口：{window.Describe()}");
                int before = HiveCore.MapInfo.TextureCount;
                window.EmitSignal(AcceptDialog.SignalName.Confirmed);
                GD.Print($"应用前后纹理槽 {before} → {HiveCore.MapInfo.TextureCount}");
                GetTree().Quit();
            }
            return;
        }

        if (visibleTest.Length > 0)
        {
            if (frames == 8)
            {
                GD.Print($"显示开关初值：水面 {view.GetNodeOrNull<Node3D>("Water")?.Visible} "
                    + $"单位 {view.GetNodeOrNull<Node3D>("Units")?.Visible} "
                    + $"装饰物 {view.GetNodeOrNull<Node3D>("Doodads")?.Visible}");
                view.SetWaterVisible(false);
                view.SetUnitsVisible(false);
                view.SetDoodadsVisible(false);
                GD.Print($"关掉后：水面 {view.GetNodeOrNull<Node3D>("Water")?.Visible} "
                    + $"单位 {view.GetNodeOrNull<Node3D>("Units")?.Visible} "
                    + $"装饰物 {view.GetNodeOrNull<Node3D>("Doodads")?.Visible}");
                view.SetWaterVisible(true);
                view.SetUnitsVisible(true);
                view.SetDoodadsVisible(true);
                Node3D? helpers = view.GetNodeOrNull<Node3D>("ClickHelpers");
                var helperMesh = helpers?.GetChildOrNull<MultiMeshInstance3D>(0);
                GD.Print($"点击盒：节点 {helpers != null}，实例 {helperMesh?.Multimesh?.InstanceCount ?? 0}");
                GetViewport().DebugDraw = Viewport.DebugDrawEnum.Wireframe;
                GD.Print($"线框模式设为 {GetViewport().DebugDraw}");
                GetViewport().DebugDraw = Viewport.DebugDrawEnum.Disabled;
                ResetCamera();
                GD.Print($"重置镜头 focus {focus} distance {distance}");
                GetTree().Quit();
            }
            return;
        }

        if (protectTest.Length > 0)
        {
            if (frames == 8)
            {
                var window = new MapProtectionWindow();
                root.AddChild(window);
                window.Build();
                HiveProtection options = window.Options();
                GD.Print($"保护窗口默认：删编辑器文件 {options.DeleteEditorFiles} 不写目录 {options.RemoveMetadata} "
                    + $"压脚本 {options.ObfuscateScript} 头偏移 {options.JunkHeaderOffset} 加密 {options.EncryptImports}");
                options.JunkHeaderOffset = 1;
                bool ok = HiveCore.ExportMap("/tmp/hivewe_ui_export.w3m", options, out string error);
                GD.Print(ok ? "保护导出成功" : $"保护导出失败：{error}");
                GetTree().Quit();
            }
            return;
        }

        if (modelTest.Length > 0)
        {
            if (frames == 8)
            {
                modelWindow = ShowModelWindow(modelTest);
                GD.Print($"模型三栏：{modelWindow.Describe()}");
            }
            else if (frames == 24)
            {
                Capture(screenshotPath);
                GD.Print("模型预览：已截图");
                GetTree().Quit();
            }
            return;
        }

        if (animTest.Length > 0)
        {
            if (frames == 6)
            {
                int placed = HiveCore.AddUnit("nmoo", 30f, 22f, 0f, 0);
                GD.Print($"动画测试：放置 nmoo 返回 {placed}");
                view.BuildUnits();
                focus = new Vector3(30f, 0f, 22f);
                distance = 14f;
                UpdateCamera();
            }
            else if (frames == 12)
            {
                Capture(screenshotPath + "_a.png");
            }
            else if (frames == 70)
            {
                Capture(screenshotPath + "_b.png");
                GD.Print($"动画测试：两组截图 {screenshotPath}_a.png / _b.png");
                GetTree().Quit();
            }
            return;
        }

        if (teamTest.Length > 0)
        {
            if (frames == 8)
            {
                int basePlayer = int.TryParse(OS.GetEnvironment("HIVE_TEAM_PLAYER"), out int parsed) ? parsed : 0;
                GD.Print($"队伍色前 {HiveCore.MapInfo.UnitCount} 个单位，起始玩家 {basePlayer}");
                for (int i = 0; i < 4; i++)
                {
                    int placed = HiveCore.AddUnit("ngns", 26f + i * 4f, 22f, 0f, basePlayer + i);
                    GD.Print($"玩家 {basePlayer + i} 放到索引 {placed}");
                }
                view.BuildUnits();
                focus = new Vector3(32f, 0f, 22f);
                distance = 22f;
                UpdateCamera();
            }
            else if (frames == 16)
            {
                Capture(screenshotPath);
                GD.Print("队伍色测试：4 个步兵，玩家 0-3");
                GetTree().Quit();
            }
            return;
        }

        if (unitTest.Length > 0)
        {
            if (frames == 8)
            {
                focus = new Vector3(31f, 0f, 21f);
                distance = 26f;
                UpdateCamera();

                string id = HiveCore.UnitTypes().Length > 0 ? HiveCore.UnitTypes()[0] : string.Empty;
                for (int i = 0; i < HiveCore.MapInfo.UnitCount; i++)
                {
                    string? existing = HiveCore.UnitId(i);
                    if (existing != null && existing != "sloc")
                    {
                        id = existing;
                        break;
                    }
                }
                unitType = id;
                PlaceUnit(new Vector3(31f, view.HeightAt(31f, 21f), 21f));
                GD.Print($"单位类型 {HiveCore.UnitTypes().Length} 个，用 {id} 放置");
            }
            else if (frames == 12)
            {
                SetSelection(true, new[] { HiveCore.MapInfo.UnitCount - 1 });
                CopySelectionObjects();
                StartPaste();
                view.UpdateClipboardGhost(34f, 24f);
                Capture(screenshotPath);
            }
            else if (frames == 18)
            {
                Vector2 screen = camera.UnprojectPosition(new Vector3(34f, view.HeightAt(34f, 24f), 24f));
                PlaceClipboardAt(screen);
                GD.Print($"单位剪贴板后 {HiveCore.MapInfo.UnitCount} 个单位");
            }
            else if (frames == 22)
            {
                Capture(unitTest);
                GetTree().Quit();
            }
            return;
        }

        if (clipTest.Length > 0)
        {
            if (frames == 8)
            {
                focus = new Vector3(33f, 0f, 26f);
                distance = 11f;
                UpdateCamera();
                SetSelection(false, new[] { 0 });
                CopySelectionObjects();
                StartPaste();
                view.UpdateClipboardGhost(35f, 30f);
            }
            else if (frames == 16)
            {
                Capture(screenshotPath);

                int before = HiveCore.MapInfo.DoodadCount;
                int count = HiveCore.ClipboardCount;
                Vector2 screen = camera.UnprojectPosition(new Vector3(33f, view.HeightAt(33f, 26f), 26f));
                PlaceClipboardAt(screen);
                int after = HiveCore.MapInfo.DoodadCount;
                GD.Print($"剪贴板测试：{count} 个放到 {before} -> {after}");
            }
            else if (frames == 20)
            {
                Capture(clipTest);
            }
            else if (frames == 24)
            {
                HiveCore.Undo();
                ReloadObjects();
                GD.Print($"撤销后 {HiveCore.MapInfo.DoodadCount}");
                GetTree().Quit();
            }
            return;
        }

        if (pickTest.Length > 0)
        {
            if (frames == 8)
            {
                SetSelection(false, new[] { 0 });
                focus = new Vector3(27f, 0f, 17.5f);
                distance = 16f;
                UpdateCamera();
            }
            else if (frames == 20)
            {
                Capture(screenshotPath);
                GD.Print($"拾取测试：选中装饰物 {HiveCore.DoodadId(selection[0])}");

                int before = HiveCore.MapInfo.DoodadCount;
                DeleteSelection();
                int after = HiveCore.MapInfo.DoodadCount;
                HiveCore.Undo();
                ReloadObjects();
                int restored = HiveCore.MapInfo.DoodadCount;
                GD.Print($"删除测试：{before} -> {after} -> 撤销 {restored}");

                HiveDoodad[] before2 = HiveCore.Doodads();
                GD.Print($"移动前 #0 在 ({before2[0].X:F1}, {before2[0].Y:F1})");
                HiveCore.BeginGroup(false, new[] { 0 }, before2[0].X, before2[0].Y);
                HiveCore.MoveGroupTo(false, 30f, 20f);
                HiveCore.EndGroup(false);
                HiveDoodad[] after2 = HiveCore.Doodads();
                GD.Print($"移动后 #0 在 ({after2[0].X:F1}, {after2[0].Y:F1})");
                HiveCore.Undo();
                HiveDoodad[] undone2 = HiveCore.Doodads();
                GD.Print($"撤销移动后 #0 在 ({undone2[0].X:F1}, {undone2[0].Y:F1})");

                HiveDoodad doodad = HiveCore.Doodads()[0];
                float a0 = doodad.Angle;
                HiveCore.BeginGroup(false, new[] { 0 }, doodad.X, doodad.Y);
                HiveCore.RotateGroupTo(false, doodad.X + MathF.Cos(1.5f), doodad.Y + MathF.Sin(1.5f));
                HiveCore.EndGroup(false);
                float a1 = HiveCore.ObjectAngle(false, 0);

                HiveUnit unit = HiveCore.Units()[0];
                float u0 = unit.Angle;
                HiveCore.BeginGroup(true, new[] { 0 }, unit.X, unit.Y);
                HiveCore.RotateGroupTo(true, unit.X + MathF.Cos(1.5f), unit.Y + MathF.Sin(1.5f));
                HiveCore.EndGroup(true);
                float u1 = HiveCore.ObjectAngle(true, 0);
                GD.Print($"旋转：装饰物 {a0:F2} -> {a1:F2}，单位 {u0:F2} -> {u1:F2}");

                HiveCore.Undo();
                HiveCore.Undo();
                ReloadObjects();
                GD.Print($"旋转后连续撤销两次，装饰物 {HiveCore.Doodads()[0].Angle:F2}，单位 {HiveCore.Units()[0].Angle:F2}");

                GetTree().Quit();
            }
            return;
        }

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

    private static int PathingBlocked()
    {
        byte[]? cells = HiveCore.Pathing(out _, out _);
        if (cells == null)
        {
            return -1;
        }

        int blocked = 0;
        foreach (byte cell in cells)
        {
            if ((cell & 0x02) != 0)
            {
                blocked++;
            }
        }
        return blocked;
    }

    private void DumpStartLocations(string label)
    {
        int count = HiveCore.StartLocationCount();
        GD.Print($"{label}：{count} 个开始点");
        for (int i = 0; i < count; i++)
        {
            if (HiveCore.StartLocation(i, out int player, out float x, out float y))
            {
                GD.Print($"  玩家 {player + 1} 在 {x:F1}, {y:F1}");
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
