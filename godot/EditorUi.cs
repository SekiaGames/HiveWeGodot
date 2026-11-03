using System;
using Godot;

namespace HiveWeGodot;

public partial class EditorUi : CanvasLayer
{
    public enum ToolMode
    {
        Select,
        Texture,
        HeightRaise,
        HeightLower,
        HeightPlateau,
        HeightRipple,
        HeightSmooth,
        WaterAdd,
        WaterRemove,
        CliffRaise,
        CliffLower,
        CliffLevel,
        CliffRamp,
        DoodadPlace,
        UnitPlace,
        StartLocation,
        PathingPaint,
    }

    public enum ViewLayer
    {
        Units,
        Doodads,
        Water,
        Brush,
        Lighting,
        ClickHelpers,
        Minimap,
    }

    public event Action<int>? TextureSelected;
    public event Action<ToolMode>? ToolSelected;
    public event Action<int>? BrushSizeChanged;
    public event Action<int>? BrushShapeChanged;
    public event Action? UndoRequested;
    public event Action? RedoRequested;
    public event Action? SaveRequested;
    public event Action? ExportRequested;
    public event Action? SaveAsRequested;
    public event Action? TileSetRequested;
    public event Action? TilePatherRequested;
    public event Action? ImportHeightmapRequested;
    public event Action? OpenRequested;
    public event Action? NewMapRequested;
    public event Action<Vector2>? MinimapClicked;
    public event Action<bool>? PathingToggled;
    public event Action<bool>? RegionsToggled;
    public event Action<ViewLayer, bool>? VisibilityToggled;
    public event Action? CameraResetRequested;
    public event Action? DoodadPickerRequested;
    public event Action? UnitPickerRequested;
    public event Action<int, string, int, string>? ObjectValueChanged;
    public event Action? NewObjectRequested;
    public event Action? DeleteObjectRequested;
    public event Action? MapInfoRequested;
    public event Action? PlayersRequested;
    public event Action? ConstantsRequested;
    public event Action? ObjectEditorRequested;
    public event Action? TriggerRequested;
    public event Action? RegionRequested;
    public event Action? ModelRequested;
    public event Action<string>? CliffTypeSelected;
    public event Action<int>? DoodadRandomChanged;
    public event Action<int, int>? PathingBrushRequested;
    public event Action<int>? StartLocationRequested;
    public event Action<int>? StartLocationRemoveRequested;

    private Label status = null!;
    private Label toolLabel = null!;
    private HBoxContainer textureRow = null!;
    private TextureRect minimap = null!;
    private VBoxContainer minimapPanel = null!;
    private readonly ButtonGroup toolGroup = new();
    private readonly ButtonGroup shapeGroup = new();
    private SpinBox brushSize = null!;
    private CheckBox pathingToggle = null!;
    private Button doodadButton = null!;
    private Button unitButton = null!;
    private SpinBox startPlayer = null!;
    private OptionButton cliffTypeBox = null!;

    private PanelContainer objectPanel = null!;
    private Label objectTitle = null!;
    private LineEdit objectFilter = null!;
    private VBoxContainer objectRows = null!;
    private readonly System.Collections.Generic.List<(Label Label, Control Row)> objectRowCache = new();
    private int objectCategory = -1;
    private string objectId = string.Empty;

    private static readonly Color PanelColor = new(0.11f, 0.12f, 0.14f, 0.96f);
    private static readonly Color ButtonColor = new(0.18f, 0.19f, 0.22f);

    public void Build(TerrainView view)
    {
        BuildPalette(view);
        BuildObjectPanel();
        BuildToolbar();
        BuildStatus();
    }

    private void BuildObjectPanel()
    {
        objectPanel = new PanelContainer
        {
            Visible = false,
            MouseFilter = Control.MouseFilterEnum.Stop,
        };
        var style = new StyleBoxFlat { BgColor = PanelColor };
        style.SetCornerRadiusAll(0);
        style.ContentMarginLeft = 10;
        style.ContentMarginRight = 10;
        style.ContentMarginTop = 10;
        style.ContentMarginBottom = 10;
        objectPanel.AddThemeStyleboxOverride("panel", style);
        objectPanel.SetAnchorsPreset(Control.LayoutPreset.RightWide);
        objectPanel.OffsetLeft = -380;
        objectPanel.OffsetTop = 40;
        objectPanel.OffsetBottom = -30;
        AddChild(objectPanel);

        var column = new VBoxContainer();
        column.AddThemeConstantOverride("separation", 6);
        objectPanel.AddChild(column);

        objectTitle = SectionTitle("对象属性");
        column.AddChild(objectTitle);

        var buttons = new HBoxContainer();
        buttons.AddThemeConstantOverride("separation", 4);
        column.AddChild(buttons);

        var create = new Button { Text = "复制为新对象", SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        create.Pressed += () => NewObjectRequested?.Invoke();
        buttons.AddChild(create);

        var remove = new Button { Text = "删除自定义", SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        remove.Pressed += () => DeleteObjectRequested?.Invoke();
        buttons.AddChild(remove);

        objectFilter = new LineEdit { PlaceholderText = "筛选字段" };
        objectFilter.TextChanged += _ => ApplyObjectFilter();
        column.AddChild(objectFilter);

        var scroll = new ScrollContainer
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
        };
        column.AddChild(scroll);

        objectRows = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        objectRows.AddThemeConstantOverride("separation", 2);
        scroll.AddChild(objectRows);
    }

    public void ShowObjectProperties(int category, string id, string title)
    {
        objectCategory = category;
        objectId = id;
        objectTitle.Text = title;
        objectFilter.Text = string.Empty;

        foreach (Node child in objectRows.GetChildren())
        {
            objectRows.RemoveChild(child);
            child.QueueFree();
        }
        objectRowCache.Clear();

        objectPanel.Visible = true;

        int count = HiveCore.ObjectFieldCount(category, id);
        for (int i = 0; i < count; i++)
        {
            int index = i;
            string label = HiveCore.ObjectFieldLabel(category, i) ?? string.Empty;
            string value = HiveCore.ObjectValue(category, id, i) ?? string.Empty;
            string description = HiveCore.ObjectFieldDescription(category, i) ?? string.Empty;

            var row = new HBoxContainer();
            row.AddThemeConstantOverride("separation", 8);

            var name = new Label
            {
                Text = label,
                CustomMinimumSize = new Vector2(210, 0),
                AutowrapMode = TextServer.AutowrapMode.WordSmart,
                TooltipText = description.Length > 0 ? description : label,
                Modulate = HiveCore.ObjectValueModified(category, id, i)
                    ? new Color(0.85f, 0.6f, 1f)
                    : Colors.White,
            };
            row.AddChild(name);

            var edit = new LineEdit
            {
                Text = value,
                SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            };
            edit.TextSubmitted += text => CommitObjectValue(name, index, text);
            edit.FocusExited += () => CommitObjectValue(name, index, edit.Text);
            row.AddChild(edit);

            objectRows.AddChild(row);
            objectRowCache.Add((name, row));
        }
    }

    public void ClearObjectProperties()
    {
        objectPanel.Visible = false;
        objectCategory = -1;
        objectId = string.Empty;
    }

    public (int Category, string Id)? CurrentObject
        => objectCategory >= 0 && objectId.Length > 0 ? (objectCategory, objectId) : null;

    private void CommitObjectValue(Label label, int index, string text)
    {
        if (objectCategory < 0)
        {
            return;
        }
        if (!HiveCore.SetObjectValue(objectCategory, objectId, index, text))
        {
            return;
        }
        label.Modulate = new Color(0.85f, 0.6f, 1f);
        ObjectValueChanged?.Invoke(objectCategory, objectId, index, text);
    }

    private void ApplyObjectFilter()
    {
        string keyword = objectFilter.Text.Trim();
        foreach (var entry in objectRowCache)
        {
            entry.Row.Visible = keyword.Length == 0
                || entry.Label.Text.Contains(keyword, StringComparison.OrdinalIgnoreCase);
        }
    }

    private void BuildPalette(TerrainView view)
    {
        var panel = new PanelContainer();
        var style = new StyleBoxFlat { BgColor = PanelColor };
        style.SetCornerRadiusAll(0);
        style.ContentMarginLeft = 10;
        style.ContentMarginRight = 10;
        style.ContentMarginTop = 10;
        style.ContentMarginBottom = 10;
        panel.AddThemeStyleboxOverride("panel", style);
        panel.SetAnchorsAndOffsetsPreset(Control.LayoutPreset.LeftWide);
        panel.OffsetRight = 232;
        panel.OffsetBottom = -30;
        AddChild(panel);

        var outer = new VBoxContainer();
        outer.AddThemeConstantOverride("separation", 8);
        panel.AddChild(outer);

        var scroll = new ScrollContainer
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
        };
        outer.AddChild(scroll);

        var column = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        column.AddThemeConstantOverride("separation", 8);
        scroll.AddChild(column);

        column.AddChild(SectionTitle("地面纹理"));

        var grid = new GridContainer { Columns = 4 };
        grid.AddThemeConstantOverride("h_separation", 4);
        grid.AddThemeConstantOverride("v_separation", 4);
        column.AddChild(grid);

        // 最后一个槽是 blight，单独做一个按钮
        for (int i = 0; i < view.Info.TextureCount - 1; i++)
        {
            int index = i;
            var button = new Button
            {
                ToggleMode = true,
                ButtonGroup = toolGroup,
                CustomMinimumSize = new Vector2(52, 52),
                ExpandIcon = true,
                TooltipText = HiveCore.TextureId(i) ?? $"纹理 {i}",
            };
            Texture2D? thumbnail = view.TextureThumbnail(i);
            if (thumbnail != null)
            {
                button.Icon = thumbnail;
            }
            button.Pressed += () => TextureSelected?.Invoke(index);
            grid.AddChild(button);
        }

        var blight = new Button
        {
            Text = "枯萎",
            ToggleMode = true,
            ButtonGroup = toolGroup,
            CustomMinimumSize = new Vector2(52, 52),
            TooltipText = "blight",
        };
        blight.Pressed += () => TextureSelected?.Invoke(view.Info.TextureCount);
        grid.AddChild(blight);

        column.AddChild(Spacer(6));
        column.AddChild(SectionTitle("高度"));

        var heightGrid = new GridContainer { Columns = 3 };
        heightGrid.AddThemeConstantOverride("h_separation", 4);
        heightGrid.AddThemeConstantOverride("v_separation", 4);
        column.AddChild(heightGrid);
        column.AddChild(ToolButton("选择对象", ToolMode.Select));
        column.AddChild(Spacer(6));

        heightGrid.AddChild(ToolButton("抬升", ToolMode.HeightRaise));
        heightGrid.AddChild(ToolButton("降低", ToolMode.HeightLower));
        heightGrid.AddChild(ToolButton("削平", ToolMode.HeightPlateau));
        heightGrid.AddChild(ToolButton("起伏", ToolMode.HeightRipple));
        heightGrid.AddChild(ToolButton("平滑", ToolMode.HeightSmooth));

        column.AddChild(Spacer(6));
        column.AddChild(SectionTitle("水域"));

        var waterGrid = new GridContainer { Columns = 2 };
        waterGrid.AddThemeConstantOverride("h_separation", 4);
        column.AddChild(waterGrid);
        waterGrid.AddChild(ToolButton("加水", ToolMode.WaterAdd));
        waterGrid.AddChild(ToolButton("去水", ToolMode.WaterRemove));

        column.AddChild(Spacer(6));
        column.AddChild(SectionTitle("悬崖"));

        var cliffGrid = new GridContainer { Columns = 2 };
        cliffGrid.AddThemeConstantOverride("h_separation", 4);
        cliffGrid.AddThemeConstantOverride("v_separation", 4);
        column.AddChild(cliffGrid);
        cliffGrid.AddChild(new Label { Text = "悬崖类型" });
        cliffTypeBox = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        for (int i = 0; i < HiveCore.CliffsetCount(); i++)
        {
            cliffTypeBox.AddItem(HiveCore.CliffsetId(i) ?? "?");
        }
        cliffTypeBox.ItemSelected += _ => CliffTypeSelected?.Invoke(CurrentCliffType());
        cliffGrid.AddChild(cliffTypeBox);

        cliffGrid.AddChild(ToolButton("升一级", ToolMode.CliffRaise));
        cliffGrid.AddChild(ToolButton("降一级", ToolMode.CliffLower));
        cliffGrid.AddChild(ToolButton("抹平", ToolMode.CliffLevel));
        cliffGrid.AddChild(ToolButton("斜坡", ToolMode.CliffRamp));

        column.AddChild(Spacer(10));
        column.AddChild(SectionTitle("对象"));

        doodadButton = new Button { Text = "装饰物…", SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        doodadButton.Pressed += () => DoodadPickerRequested?.Invoke();
        column.AddChild(doodadButton);

        unitButton = new Button { Text = "单位…", SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        unitButton.Pressed += () => UnitPickerRequested?.Invoke();
        column.AddChild(unitButton);

        var randomRow = new HBoxContainer();
        randomRow.AddThemeConstantOverride("separation", 4);
        column.AddChild(randomRow);

        var randomBoxes = new System.Collections.Generic.List<(CheckBox Box, int Bit)>();
        foreach ((string label, int bit) in new[] { ("随机变体", 1), ("随机朝向", 2), ("随机缩放", 4) })
        {
            var box = new CheckBox { Text = label };
            box.Toggled += _ =>
            {
                int flags = 0;
                foreach (var entry in randomBoxes)
                {
                    if (entry.Box.ButtonPressed)
                    {
                        flags |= entry.Bit;
                    }
                }
                DoodadRandomChanged?.Invoke(flags);
            };
            randomRow.AddChild(box);
            randomBoxes.Add((box, bit));
        }

        var startRow = new HBoxContainer();
        startRow.AddThemeConstantOverride("separation", 4);
        column.AddChild(startRow);

        startRow.AddChild(new Label { Text = "开始点" });
        startPlayer = new SpinBox
        {
            MinValue = 1,
            MaxValue = 24,
            Step = 1,
            Value = 1,
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
        };
        startRow.AddChild(startPlayer);

        var startPlace = new Button { Text = "放置" };
        startPlace.Pressed += () => StartLocationRequested?.Invoke((int)startPlayer.Value - 1);
        startRow.AddChild(startPlace);

        var startRemove = new Button { Text = "移除" };
        startRemove.Pressed += () => StartLocationRemoveRequested?.Invoke((int)startPlayer.Value - 1);
        startRow.AddChild(startRemove);

        column.AddChild(Spacer(8));

        pathingToggle = new CheckBox { Text = "显示路径" };
        pathingToggle.Toggled += on => PathingToggled?.Invoke(on);
        column.AddChild(pathingToggle);

        var pathingFlags = new HBoxContainer();
        pathingFlags.AddThemeConstantOverride("separation", 4);
        column.AddChild(pathingFlags);

        var walkBox = new CheckBox { Text = "不可走", ButtonPressed = true };
        var flyBox = new CheckBox { Text = "不可飞" };
        var buildBox = new CheckBox { Text = "不可建", ButtonPressed = true };
        pathingFlags.AddChild(walkBox);
        pathingFlags.AddChild(flyBox);
        pathingFlags.AddChild(buildBox);

        int Mask()
        {
            int mask = 0;
            if (walkBox.ButtonPressed)
            {
                mask |= 0x02;
            }
            if (flyBox.ButtonPressed)
            {
                mask |= 0x04;
            }
            if (buildBox.ButtonPressed)
            {
                mask |= 0x08;
            }
            return mask;
        }

        var pathingOps = new HBoxContainer();
        pathingOps.AddThemeConstantOverride("separation", 4);
        column.AddChild(pathingOps);

        foreach ((string label, int operation) in new[] { ("覆盖", 0), ("加", 1), ("减", 2) })
        {
            int op = operation;
            var button = new Button { Text = label, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
            button.Pressed += () => PathingBrushRequested?.Invoke(Mask(), op);
            pathingOps.AddChild(button);
        }

        var regionToggle = new CheckBox { Text = "显示区域", ButtonPressed = true };
        regionToggle.Toggled += on => RegionsToggled?.Invoke(on);
        column.AddChild(regionToggle);

        column.AddChild(Spacer(6));
        column.AddChild(SectionTitle("显示"));

        var visibilityRow = new GridContainer { Columns = 2 };
        visibilityRow.AddThemeConstantOverride("h_separation", 8);
        column.AddChild(visibilityRow);

        foreach ((string label, ViewLayer layer, bool on) in new[]
                 {
                     ("单位", ViewLayer.Units, true),
                     ("装饰物", ViewLayer.Doodads, true),
                     ("水面", ViewLayer.Water, true),
                     ("笔刷", ViewLayer.Brush, true),
                     ("光照", ViewLayer.Lighting, true),
                     ("点击盒", ViewLayer.ClickHelpers, true),
                     ("小地图", ViewLayer.Minimap, true),
                 })
        {
            var box = new CheckBox { Text = label, ButtonPressed = on };
            ViewLayer target = layer;
            box.Toggled += value =>
            {
                if (target == ViewLayer.Minimap)
                {
                    minimapPanel.Visible = value;
                    return;
                }
                VisibilityToggled?.Invoke(target, value);
            };
            visibilityRow.AddChild(box);
        }

        var resetCamera = new Button { Text = "重置镜头" };
        resetCamera.Pressed += () => CameraResetRequested?.Invoke();
        column.AddChild(resetCamera);

        column.AddChild(Spacer(6));
        column.AddChild(SectionTitle("笔刷联动"));

        var brushFlagBoxes = new System.Collections.Generic.List<(CheckBox Box, int Bit)>();
        var brushFlagGrid = new GridContainer { Columns = 2 };
        brushFlagGrid.AddThemeConstantOverride("h_separation", 8);
        column.AddChild(brushFlagGrid);
        int brushFlags = HiveCore.TerrainBrushFlags();
        foreach ((string label, int bit) in new[]
                 {
                     ("限水面高度", 0x01), ("更新装饰物高度", 0x02), ("相对悬崖高度", 0x04),
                     ("悬崖路径", 0x08), ("地面路径", 0x10), ("水面路径", 0x20),
                     ("改地面", 0x40), ("改水面", 0x80),
                 })
        {
            var box = new CheckBox { Text = label, ButtonPressed = (brushFlags & bit) != 0 };
            box.Toggled += _ =>
            {
                int mask = 0;
                foreach ((CheckBox other, int otherBit) in brushFlagBoxes)
                {
                    if (other.ButtonPressed)
                    {
                        mask |= otherBit;
                    }
                }
                HiveCore.SetTerrainBrushFlags(mask);
            };
            brushFlagGrid.AddChild(box);
            brushFlagBoxes.Add((box, bit));
        }

        column.AddChild(Spacer(6));
        column.AddChild(SectionTitle("笔刷"));

        var sizeRow = new HBoxContainer();
        sizeRow.AddChild(new Label { Text = "大小" });
        brushSize = new SpinBox
        {
            MinValue = 1,
            MaxValue = 48,
            Step = 1,
            Value = 8,
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
        };
        brushSize.ValueChanged += value => BrushSizeChanged?.Invoke((int)value);
        sizeRow.AddChild(brushSize);
        column.AddChild(sizeRow);

        var shapeRow = new HBoxContainer();
        shapeRow.AddThemeConstantOverride("separation", 4);
        string[] names = { "方形", "圆形", "菱形" };
        for (int i = 0; i < names.Length; i++)
        {
            int shape = i;
            var button = new Button
            {
                Text = names[i],
                ToggleMode = true,
                ButtonGroup = shapeGroup,
                SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            };
            if (i == 0)
            {
                button.ButtonPressed = true;
            }
            button.Pressed += () => BrushShapeChanged?.Invoke(shape);
            shapeRow.AddChild(button);
        }
        column.AddChild(shapeRow);

        column.AddChild(Spacer(10));

        minimap = new TextureRect
        {
            Texture = view.Minimap(),
            ExpandMode = TextureRect.ExpandModeEnum.IgnoreSize,
            StretchMode = TextureRect.StretchModeEnum.KeepAspectCentered,
            CustomMinimumSize = new Vector2(0, 100),
        };
        float aspect = (float)view.Info.Height / view.Info.Width;
        minimap.CustomMinimumSize = new Vector2(210, 210 * aspect);
        minimap.GuiInput += @event =>
        {
            if (@event is InputEventMouseButton button && button.Pressed && button.ButtonIndex == MouseButton.Left)
            {
                Vector2 size = minimap.Size;
                if (size.X <= 0 || size.Y <= 0)
                {
                    return;
                }
                Vector2 local = button.Position / size;
                MinimapClicked?.Invoke(new Vector2(
                    Mathf.Clamp(local.X, 0f, 1f) * (view.Info.Width - 1),
                    Mathf.Clamp(local.Y, 0f, 1f) * (view.Info.Height - 1)));
            }
        };
        minimapPanel = new VBoxContainer();
        minimapPanel.AddThemeConstantOverride("separation", 4);
        minimapPanel.AddChild(SectionTitle("小地图"));
        minimapPanel.AddChild(minimap);
        outer.AddChild(minimapPanel);

        textureRow = new HBoxContainer();
        column.AddChild(textureRow);
    }

    private void BuildToolbar()
    {
        var bar = new PanelContainer();
        var style = new StyleBoxFlat { BgColor = PanelColor };
        style.SetCornerRadiusAll(0);
        style.ContentMarginLeft = 12;
        style.ContentMarginRight = 12;
        style.ContentMarginTop = 6;
        style.ContentMarginBottom = 6;
        bar.AddThemeStyleboxOverride("panel", style);
        bar.SetAnchorsAndOffsetsPreset(Control.LayoutPreset.TopWide);
        bar.OffsetBottom = 40;
        bar.OffsetLeft = 232;
        AddChild(bar);

        var row = new HBoxContainer();
        row.AddThemeConstantOverride("separation", 8);
        bar.AddChild(row);

        toolLabel = new Label { Text = "纹理笔" };
        row.AddChild(toolLabel);

        row.AddChild(new VSeparator());
        row.AddChild(ActionButton("新建", () => NewMapRequested?.Invoke()));
        row.AddChild(ActionButton("打开", () => OpenRequested?.Invoke()));
        row.AddChild(ActionButton("撤销", () => UndoRequested?.Invoke()));
        row.AddChild(ActionButton("重做", () => RedoRequested?.Invoke()));
        row.AddChild(new VSeparator());
        row.AddChild(ActionButton("保存 w3m", () => SaveRequested?.Invoke()));
        row.AddChild(ActionButton("另存为", () => SaveAsRequested?.Invoke()));
        row.AddChild(ActionButton("导出", () => ExportRequested?.Invoke()));
        row.AddChild(new VSeparator());
        row.AddChild(ActionButton("地图信息", () => MapInfoRequested?.Invoke()));
        row.AddChild(ActionButton("玩家", () => PlayersRequested?.Invoke()));
        row.AddChild(ActionButton("常量", () => ConstantsRequested?.Invoke()));
        row.AddChild(ActionButton("对象", () => ObjectEditorRequested?.Invoke()));
        row.AddChild(ActionButton("触发器", () => TriggerRequested?.Invoke()));
        row.AddChild(ActionButton("区域", () => RegionRequested?.Invoke()));
        row.AddChild(ActionButton("地形集", () => TileSetRequested?.Invoke()));
        row.AddChild(ActionButton("瓦片路径", () => TilePatherRequested?.Invoke()));
        row.AddChild(ActionButton("导入高度", () => ImportHeightmapRequested?.Invoke()));
        row.AddChild(ActionButton("模型", () => ModelRequested?.Invoke()));
    }

    private void BuildStatus()
    {
        var bar = new PanelContainer();
        var style = new StyleBoxFlat { BgColor = PanelColor };
        style.SetCornerRadiusAll(0);
        style.ContentMarginLeft = 12;
        style.ContentMarginTop = 4;
        style.ContentMarginBottom = 4;
        bar.AddThemeStyleboxOverride("panel", style);
        bar.SetAnchorsAndOffsetsPreset(Control.LayoutPreset.BottomWide);
        bar.OffsetTop = -30;
        bar.OffsetLeft = 232;
        AddChild(bar);

        status = new Label { Text = "" };
        bar.AddChild(status);
    }

    public void SetStatus(string text) => status.Text = text;

    public void RefreshMinimap(TerrainView view) => minimap.Texture = view.Minimap();

    public void SetActiveTool(string name) => toolLabel.Text = name;

    public void SetBrushSize(int size) => brushSize.Value = size;

    public string CurrentCliffType()
        => cliffTypeBox.ItemCount == 0 ? string.Empty : HiveCore.CliffsetId(cliffTypeBox.Selected) ?? string.Empty;

    public void SetDoodadTypes(int count) => doodadButton.TooltipText = $"可放置 {count} 种";

    public void SetUnitTypes(int count) => unitButton.TooltipText = $"可放置 {count} 种";

    public void SetDoodadChoice(string id, string name)
    {
        doodadButton.Text = $"装饰物：{name}（{id}）";
    }

    public void SetUnitChoice(string id, string name)
    {
        unitButton.Text = $"单位：{name}（{id}）";
    }

    public void SetPathingChecked(bool on)
    {
        pathingToggle.ButtonPressed = on;
        PathingToggled?.Invoke(on);
    }

    private Button ToolButton(string text, ToolMode mode)
    {
        var button = new Button
        {
            Text = text,
            ToggleMode = true,
            ButtonGroup = toolGroup,
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
        };
        button.Pressed += () => ToolSelected?.Invoke(mode);
        return button;
    }

    private Button ActionButton(string text, Action action)
    {
        var button = new Button { Text = text };
        button.Pressed += action;
        return button;
    }

    private static Label SectionTitle(string text) => new()
    {
        Text = text,
        Modulate = new Color(0.65f, 0.7f, 0.78f),
    };

    private static Control Spacer(float height) => new() { CustomMinimumSize = new Vector2(0, height) };
}
