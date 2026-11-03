using System;
using System.Collections.Generic;
using Godot;

namespace HiveWeGodot;

public partial class TilePatherWindow : AcceptDialog
{
    private readonly List<string> tiles = new();
    private readonly Dictionary<string, int> masks = new();
    private readonly Dictionary<string, int> original = new();

    private GridContainer grid = null!;
    private Label nameLabel = null!;
    private CheckBox unwalkable = null!;
    private CheckBox unflyable = null!;
    private CheckBox unbuildable = null!;
    private ButtonGroup group = new();
    private int selectedIndex = -1;

    public event Action? Applied;

    public void Build()
    {
        Title = "瓦片路径";
        OkButtonText = "应用";
        AddCancelButton("取消");

        var column = new VBoxContainer { CustomMinimumSize = new Vector2(560, 460) };
        column.AddThemeConstantOverride("separation", 8);
        AddChild(column);

        column.AddChild(new Label
        {
            Text = "给地形纹理单独设路径。改完会重算用到这块纹理的格子。",
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
        });

        grid = new GridContainer { Columns = 8 };
        var scroll = new ScrollContainer { SizeFlagsVertical = Control.SizeFlags.ExpandFill };
        grid.SizeFlagsHorizontal = Control.SizeFlags.ExpandFill;
        scroll.AddChild(grid);
        column.AddChild(scroll);

        nameLabel = new Label { Text = "瓦片：" };
        column.AddChild(nameLabel);

        var flags = new HBoxContainer();
        flags.AddThemeConstantOverride("separation", 12);
        column.AddChild(flags);

        unwalkable = new CheckBox { Text = "不可走" };
        unflyable = new CheckBox { Text = "不可飞" };
        unbuildable = new CheckBox { Text = "不可建" };
        foreach (CheckBox box in new[] { unwalkable, unflyable, unbuildable })
        {
            box.Toggled += _ => StoreMask();
            box.Disabled = true;
            flags.AddChild(box);
        }

        Load();
        Fill();

        Confirmed += Apply;
    }

    private void Load()
    {
        int count = HiveCore.MapInfo.TextureCount - 1;
        for (int i = 0; i < count; i++)
        {
            string? id = HiveCore.TextureId(i);
            if (id == null)
            {
                continue;
            }
            int mask = HiveCore.TilePathing(i);
            tiles.Add(id);
            masks[id] = mask;
            original[id] = mask;
        }
    }

    private void Fill()
    {
        foreach (Node child in grid.GetChildren())
        {
            grid.RemoveChild(child);
            child.QueueFree();
        }

        group = new ButtonGroup();
        for (int i = 0; i < tiles.Count; i++)
        {
            int index = i;
            var button = new Button
            {
                ToggleMode = true,
                ButtonGroup = group,
                CustomMinimumSize = new Vector2(52, 52),
                ExpandIcon = true,
                TooltipText = $"{tiles[i]}（{MaskText(masks[tiles[i]])}）",
            };
            Texture2D? thumbnail = TileThumbnails.Get(tiles[i]);
            if (thumbnail != null)
            {
                button.Icon = thumbnail;
            }
            else
            {
                button.Text = tiles[i];
            }
            button.Pressed += () => Select(index);
            grid.AddChild(button);
        }
        Select(-1);
    }

    private void Select(int index)
    {
        selectedIndex = index;
        bool valid = index >= 0 && index < tiles.Count;
        unwalkable.Disabled = !valid;
        unflyable.Disabled = !valid;
        unbuildable.Disabled = !valid;
        if (!valid)
        {
            nameLabel.Text = "瓦片：";
            return;
        }

        string id = tiles[index];
        nameLabel.Text = $"瓦片：{id}　{MaskText(masks[id])}";
        unwalkable.SetPressedNoSignal((masks[id] & 0x02) != 0);
        unflyable.SetPressedNoSignal((masks[id] & 0x04) != 0);
        unbuildable.SetPressedNoSignal((masks[id] & 0x08) != 0);
    }

    private void StoreMask()
    {
        if (selectedIndex < 0 || selectedIndex >= tiles.Count)
        {
            return;
        }
        int mask = 0;
        if (unwalkable.ButtonPressed)
        {
            mask |= 0x02;
        }
        if (unflyable.ButtonPressed)
        {
            mask |= 0x04;
        }
        if (unbuildable.ButtonPressed)
        {
            mask |= 0x08;
        }
        string id = tiles[selectedIndex];
        masks[id] = mask;
        nameLabel.Text = $"瓦片：{id}　{MaskText(mask)}";
    }

    private void Apply()
    {
        int changed = 0;
        for (int i = 0; i < tiles.Count; i++)
        {
            string id = tiles[i];
            if (masks[id] == original[id])
            {
                continue;
            }
            if (HiveCore.SetTilePathing(i, masks[id]) != 0)
            {
                GD.PrintErr($"设置 {id} 的路径失败");
                QueueFree();
                return;
            }
            changed++;
        }

        GD.Print($"瓦片路径：改了 {changed} 块");
        if (changed > 0)
        {
            Applied?.Invoke();
        }
        QueueFree();
    }

    public string Describe()
    {
        int modified = 0;
        foreach (string id in tiles)
        {
            if (masks[id] != original[id])
            {
                modified++;
            }
        }
        return $"共 {tiles.Count} 块纹理，已改 {modified} 块";
    }

    private static string MaskText(int mask)
    {
        var parts = new List<string>();
        if ((mask & 0x02) != 0)
        {
            parts.Add("不可走");
        }
        if ((mask & 0x04) != 0)
        {
            parts.Add("不可飞");
        }
        if ((mask & 0x08) != 0)
        {
            parts.Add("不可建");
        }
        return parts.Count == 0 ? "可走" : string.Join("，", parts);
    }
}
