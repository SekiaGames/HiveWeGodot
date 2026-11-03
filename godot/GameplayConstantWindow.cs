using System;
using Godot;

namespace HiveWeGodot;

public partial class GameplayConstantWindow : AcceptDialog
{
    private const int KeyColumnWidth = 340;

    private LineEdit filter = null!;
    private VBoxContainer rows = null!;
    private Label status = null!;

    private int count;
    private string[] keys = Array.Empty<string>();
    private string[] labels = Array.Empty<string>();
    private readonly System.Collections.Generic.List<(HBoxContainer Row, string Text)> rowCache = new();

    public void Build()
    {
        Title = "游戏常量";
        OkButtonText = "关闭";

        var column = new VBoxContainer { CustomMinimumSize = new Vector2(760, 620) };
        column.AddThemeConstantOverride("separation", 6);
        AddChild(column);

        filter = new LineEdit { PlaceholderText = "按名字或字段筛选" };
        filter.TextChanged += _ => Refill();
        column.AddChild(filter);

        var scroll = new ScrollContainer
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
        };
        column.AddChild(scroll);

        rows = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        rows.AddThemeConstantOverride("separation", 2);
        scroll.AddChild(rows);

        status = new Label { Text = string.Empty };
        column.AddChild(status);

        Load();
    }

    private void Load()
    {
        count = HiveCore.GameplayConstantCount();
        keys = new string[count];
        labels = new string[count];
        rowCache.Clear();

        for (int i = 0; i < count; i++)
        {
            keys[i] = HiveCore.GameplayConstantKey(i) ?? string.Empty;
            labels[i] = HiveCore.GameplayConstantLabel(i) ?? keys[i];

            int index = i;
            var row = new HBoxContainer();
            row.AddThemeConstantOverride("separation", 8);

            bool modified = HiveCore.GameplayConstantModified(index) != 0;
            var label = new Label
            {
                Text = labels[i],
                CustomMinimumSize = new Vector2(KeyColumnWidth, 0),
                AutowrapMode = TextServer.AutowrapMode.WordSmart,
                TooltipText = keys[i],
                Modulate = modified ? new Color(0.85f, 0.6f, 1f) : Colors.White,
            };
            row.AddChild(label);

            var edit = new LineEdit
            {
                Text = HiveCore.GameplayConstantValue(index) ?? string.Empty,
                SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            };
            edit.TextSubmitted += text => Commit(index, label, text);
            edit.FocusExited += () => Commit(index, label, edit.Text);
            row.AddChild(edit);

            rows.AddChild(row);
            rowCache.Add((row, $"{labels[i]} {keys[i]}".ToLowerInvariant()));
        }

        status.Text = $"{count} 条";
    }

    private void Commit(int index, Label label, string value)
    {
        if (HiveCore.GameplayConstantValue(index) == value)
        {
            return;
        }

        if (!HiveCore.SetGameplayConstant(index, value))
        {
            status.Text = $"改不了 {keys[index]}";
            return;
        }

        label.Modulate = HiveCore.GameplayConstantModified(index) != 0
            ? new Color(0.85f, 0.6f, 1f)
            : Colors.White;
        status.Text = $"{keys[index]} 已改，Ctrl+S 存进地图";
    }

    private void Refill()
    {
        string keyword = filter.Text.Trim().ToLowerInvariant();
        foreach ((HBoxContainer row, string text) in rowCache)
        {
            row.Visible = keyword.Length == 0 || text.Contains(keyword, StringComparison.Ordinal);
        }

        int visible = 0;
        foreach ((HBoxContainer row, string _) in rowCache)
        {
            if (row.Visible)
            {
                visible++;
            }
        }
        status.Text = $"{visible} / {count} 条";
    }

    public int RowCount => rowCache.Count;

    public int VisibleRows()
    {
        int visible = 0;
        foreach ((HBoxContainer row, string _) in rowCache)
        {
            if (row.Visible)
            {
                visible++;
            }
        }
        return visible;
    }

    public void SetFilter(string text)
    {
        filter.Text = text;
        Refill();
    }
}
