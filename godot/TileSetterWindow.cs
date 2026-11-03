using System;
using System.Collections.Generic;
using Godot;

namespace HiveWeGodot;

public partial class TileSetterWindow : AcceptDialog
{
    private const int MaxGround = 64;
    private const int MaxCliff = 15;

    private readonly List<string> selected = new();
    private readonly List<string> available = new();

    private OptionButton tilesetPicker = null!;
    private GridContainer selectedGrid = null!;
    private GridContainer availableGrid = null!;
    private Label selectedLabel = null!;
    private Label availableLabel = null!;
    private Label countsLabel = null!;
    private Button addButton = null!;
    private Button removeButton = null!;
    private Button shiftLeftButton = null!;
    private Button shiftRightButton = null!;

    private ButtonGroup selectedGroup = new();
    private ButtonGroup availableGroup = new();
    private int selectedIndex = -1;
    private int availableIndex = -1;
    private string tilesetKey = "L";

    public event Action? Applied;

    public void Build()
    {
        Title = "地形纹理集";
        OkButtonText = "应用";
        AddCancelButton("取消");

        var column = new VBoxContainer { CustomMinimumSize = new Vector2(620, 560) };
        column.AddThemeConstantOverride("separation", 8);
        AddChild(column);

        var top = new HBoxContainer();
        top.AddThemeConstantOverride("separation", 8);
        column.AddChild(top);
        top.AddChild(new Label { Text = "基础地形集" });

        tilesetPicker = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        for (int i = 0; i < HiveCore.TilesetCount(); i++)
        {
            string? key = HiveCore.TilesetKey(i);
            if (key == null)
            {
                continue;
            }
            tilesetPicker.AddItem($"{HiveCore.TilesetName(i)}（{key}）", i);
        }
        tilesetPicker.ItemSelected += index =>
        {
            string? key = HiveCore.TilesetKey((int)tilesetPicker.GetItemId((int)index));
            if (key != null)
            {
                tilesetKey = key;
                availableIndex = -1;
                FillAvailable();
            }
        };
        top.AddChild(tilesetPicker);

        var reset = new Button { Text = "重置" };
        reset.Pressed += ResetToDefault;
        top.AddChild(reset);

        countsLabel = new Label();
        column.AddChild(countsLabel);

        selectedGrid = MakeGrid();
        column.AddChild(Wrap(selectedGrid, 190));

        selectedLabel = new Label { Text = "瓦片：" };
        column.AddChild(selectedLabel);

        var operations = new HBoxContainer();
        operations.AddThemeConstantOverride("separation", 6);
        column.AddChild(operations);

        addButton = new Button { Text = "添加" };
        addButton.Pressed += AddSelected;
        operations.AddChild(addButton);

        removeButton = new Button { Text = "删除" };
        removeButton.Pressed += RemoveSelected;
        operations.AddChild(removeButton);

        shiftLeftButton = new Button { Text = "←" };
        shiftLeftButton.Pressed += () => Shift(-1);
        operations.AddChild(shiftLeftButton);

        shiftRightButton = new Button { Text = "→" };
        shiftRightButton.Pressed += () => Shift(1);
        operations.AddChild(shiftRightButton);

        column.AddChild(new HSeparator());
        column.AddChild(new Label { Text = "该地形集可用的瓦片" });

        availableGrid = MakeGrid();
        column.AddChild(Wrap(availableGrid, 190));

        availableLabel = new Label { Text = "瓦片：" };
        column.AddChild(availableLabel);

        tilesetKey = ((char)HiveCore.MapInfo.TilesetId).ToString();
        SelectCurrentTileset();
        LoadCurrent();
        FillAvailable();
        UpdateLabels();

        Confirmed += Apply;
    }

    public string Describe()
    {
        int cliffs = 0;
        foreach (string id in selected)
        {
            if (IsCliff(id))
            {
                cliffs++;
            }
        }
        return $"地形集 {tilesetKey}，已选 {selected.Count} 块（其中悬崖 {cliffs} 块），可用 {available.Count} 块";
    }

    private void SelectCurrentTileset()
    {
        for (int i = 0; i < tilesetPicker.ItemCount; i++)
        {
            string? key = HiveCore.TilesetKey((int)tilesetPicker.GetItemId(i));
            if (key == tilesetKey)
            {
                tilesetPicker.Select(i);
                return;
            }
        }
    }

    private void LoadCurrent()
    {
        selected.Clear();
        // 最后一个槽是 blight，不在地形纹理列表里
        int count = HiveCore.MapInfo.TextureCount - 1;
        for (int i = 0; i < count; i++)
        {
            string? id = HiveCore.TextureId(i);
            if (id != null)
            {
                selected.Add(id);
            }
        }
    }

    private void ClearGrid(GridContainer grid)
    {
        foreach (Node child in grid.GetChildren())
        {
            grid.RemoveChild(child);
            child.QueueFree();
        }
    }

    private void FillSelected()
    {
        ClearGrid(selectedGrid);
        selectedGroup = new ButtonGroup();
        for (int i = 0; i < selected.Count; i++)
        {
            int index = i;
            Button button = TileButton(selected[i], selectedGroup);
            button.Pressed += () =>
            {
                selectedIndex = index;
                UpdateLabels();
            };
            selectedGrid.AddChild(button);
        }
        selectedIndex = -1;
        UpdateLabels();
    }

    private void FillAvailable()
    {
        available.Clear();
        int count = HiveCore.TilesetTextureCount(tilesetKey);
        for (int i = 0; i < count; i++)
        {
            string? id = HiveCore.TilesetTextureId(tilesetKey, i);
            if (id != null)
            {
                available.Add(id);
            }
        }

        ClearGrid(availableGrid);
        availableGroup = new ButtonGroup();
        for (int i = 0; i < available.Count; i++)
        {
            int index = i;
            Button button = TileButton(available[i], availableGroup);
            button.Pressed += () =>
            {
                availableIndex = index;
                UpdateLabels();
            };
            availableGrid.AddChild(button);
        }
        availableIndex = -1;
        UpdateLabels();
    }

    private void UpdateLabels()
    {
        int cliffs = 0;
        foreach (string id in selected)
        {
            if (IsCliff(id))
            {
                cliffs++;
            }
        }

        countsLabel.Text = $"地形纹理 {selected.Count} / {MaxGround}　悬崖纹理 {cliffs} / {MaxCliff}";

        selectedLabel.Text = selectedIndex >= 0 && selectedIndex < selected.Count
            ? $"瓦片：{selected[selectedIndex]}"
            : "瓦片：";
        availableLabel.Text = availableIndex >= 0 && availableIndex < available.Count
            ? $"瓦片：{available[availableIndex]}"
            : "瓦片：";

        shiftLeftButton.Disabled = selectedIndex <= 0;
        shiftRightButton.Disabled = selectedIndex < 0 || selectedIndex >= selected.Count - 1;
        removeButton.Disabled = selectedIndex < 0 || (IsCliff(selected[selectedIndex]) && cliffs <= 1);

        bool already = availableIndex >= 0 && selected.Contains(available[availableIndex]);
        bool cliffLimit = availableIndex >= 0 && IsCliff(available[availableIndex]) && cliffs >= MaxCliff;
        addButton.Disabled = availableIndex < 0 || already || cliffLimit || selected.Count >= MaxGround;
    }

    private bool IsCliff(string id)
    {
        int index = available.IndexOf(id);
        if (index < 0)
        {
            for (int i = 0; i < HiveCore.TilesetTextureCount(tilesetKey); i++)
            {
                if (HiveCore.TilesetTextureId(tilesetKey, i) == id)
                {
                    return HiveCore.TilesetTextureCliff(tilesetKey, i);
                }
            }
            return false;
        }
        return HiveCore.TilesetTextureCliff(tilesetKey, index);
    }

    private void AddSelected()
    {
        if (availableIndex < 0 || availableIndex >= available.Count)
        {
            return;
        }
        selected.Add(available[availableIndex]);
        FillSelected();
        UpdateLabels();
    }

    private void RemoveSelected()
    {
        if (selectedIndex < 0 || selectedIndex >= selected.Count)
        {
            return;
        }
        selected.RemoveAt(selectedIndex);
        FillSelected();
        UpdateLabels();
    }

    private void Shift(int delta)
    {
        int target = selectedIndex + delta;
        if (selectedIndex < 0 || target < 0 || target >= selected.Count)
        {
            return;
        }
        (selected[selectedIndex], selected[target]) = (selected[target], selected[selectedIndex]);
        int moved = target;
        FillSelected();
        selectedIndex = moved;
        UpdateLabels();
    }

    private void ResetToDefault()
    {
        selected.Clear();
        int count = HiveCore.TilesetTextureCount(tilesetKey);
        for (int i = 0; i < count; i++)
        {
            string? id = HiveCore.TilesetTextureId(tilesetKey, i);
            if (id != null)
            {
                selected.Add(id);
            }
        }
        FillSelected();
        UpdateLabels();
    }

    private void Apply()
    {
        if (selected.Count == 0)
        {
            GD.PrintErr("地形纹理不能为空");
            QueueFree();
            return;
        }

        var oldIds = new List<string>();
        int oldCount = HiveCore.MapInfo.TextureCount - 1;
        for (int i = 0; i < oldCount; i++)
        {
            oldIds.Add(HiveCore.TextureId(i) ?? string.Empty);
        }

        var mapping = new int[oldIds.Count];
        var missing = new List<int>();
        for (int i = 0; i < oldIds.Count; i++)
        {
            int index = selected.IndexOf(oldIds[i]);
            if (index >= 0)
            {
                mapping[i] = index;
            }
            else
            {
                mapping[i] = 0;
                missing.Add(i);
            }
        }

        if (missing.Count > 0)
        {
            AskReplacements(oldIds, mapping, missing);
            return;
        }

        Commit(mapping);
    }

    private void AskReplacements(List<string> oldIds, int[] mapping, List<int> missing)
    {
        var dialog = new AcceptDialog
        {
            Title = "选择替换的瓦片",
            OkButtonText = "替换",
        };
        dialog.AddCancelButton("取消");

        var column = new VBoxContainer { CustomMinimumSize = new Vector2(360, 0) };
        column.AddThemeConstantOverride("separation", 8);
        dialog.AddChild(column);
        column.AddChild(new Label
        {
            Text = "这些瓦片在新的列表里没有了，地形上用到它们的格子要换成哪一块：",
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
        });

        var pickers = new List<(int Index, OptionButton Picker)>();
        foreach (int index in missing)
        {
            var row = new HBoxContainer();
            row.AddThemeConstantOverride("separation", 8);
            row.AddChild(new Label { Text = oldIds[index], CustomMinimumSize = new Vector2(60, 0) });

            var picker = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
            foreach (string id in selected)
            {
                picker.AddItem(id);
            }
            picker.Select(0);
            row.AddChild(picker);
            column.AddChild(row);
            pickers.Add((index, picker));
        }

        dialog.Confirmed += () =>
        {
            foreach ((int index, OptionButton picker) in pickers)
            {
                mapping[index] = (int)picker.Selected;
            }
            dialog.QueueFree();
            Commit(mapping);
        };
        dialog.Canceled += () =>
        {
            dialog.QueueFree();
            QueueFree();
        };

        GetParent().AddChild(dialog);
        dialog.PopupCentered();
    }

    private void Commit(int[] mapping)
    {
        string[] ids = selected.ToArray();
        if (!HiveCore.ChangeTileset(tilesetKey, ids, mapping, out string error))
        {
            GD.PrintErr($"换地形集失败：{error}");
            QueueFree();
            return;
        }
        GD.Print($"地形集换成 {tilesetKey}，{ids.Length} 块纹理");
        Applied?.Invoke();
        QueueFree();
    }

    private Button TileButton(string id, ButtonGroup group)
    {
        var button = new Button
        {
            ToggleMode = true,
            ButtonGroup = group,
            CustomMinimumSize = new Vector2(52, 52),
            ExpandIcon = true,
            TooltipText = id,
        };
        Texture2D? thumbnail = TileThumbnails.Get(id);
        if (thumbnail != null)
        {
            button.Icon = thumbnail;
        }
        else
        {
            button.Text = id;
        }
        return button;
    }

    private static GridContainer MakeGrid() => new() { Columns = 8 };

    private static ScrollContainer Wrap(Control content, int height)
    {
        var scroll = new ScrollContainer { CustomMinimumSize = new Vector2(0, height) };
        content.SizeFlagsHorizontal = Control.SizeFlags.ExpandFill;
        scroll.AddChild(content);
        return scroll;
    }
}
