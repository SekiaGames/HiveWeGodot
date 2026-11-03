using System;
using System.Collections.Generic;
using Godot;

namespace HiveWeGodot;

public partial class ObjectEditorWindow : AcceptDialog
{
    private static readonly (string Title, int Category)[] Categories =
    {
        ("单位", 0), ("物品", 1), ("装饰物", 2), ("可破坏物", 3), ("技能", 4), ("升级", 5), ("buff", 6),
    };

    private sealed class CategoryView
    {
        public int Category;
        public LineEdit Filter = null!;
        public ItemList List = null!;
        public VBoxContainer Fields = null!;
        public Label Status = null!;
        public readonly List<string> Ids = new();
        public readonly List<string> Texts = new();
    }

    private readonly List<CategoryView> views = new();

    public int TotalObjects()
    {
        int total = 0;
        foreach (CategoryView view in views)
        {
            total += view.Ids.Count;
        }
        return total;
    }

    public int CategoryCount => views.Count;

    public void Build()
    {
        Title = "对象编辑器";
        OkButtonText = "关闭";

        var tabs = new TabContainer { CustomMinimumSize = new Vector2(980, 640) };
        AddChild(tabs);

        foreach ((string title, int category) in Categories)
        {
            var page = new HSplitContainer { Name = title };
            tabs.AddChild(page);

            var left = new VBoxContainer { CustomMinimumSize = new Vector2(300, 0) };
            left.AddThemeConstantOverride("separation", 4);
            page.AddChild(left);

            var view = new CategoryView { Category = category };
            view.Filter = new LineEdit { PlaceholderText = "按名字或 id 筛选" };
            view.Filter.TextChanged += _ => Refill(view);
            left.AddChild(view.Filter);

            view.List = new ItemList { SizeFlagsVertical = Control.SizeFlags.ExpandFill };
            view.List.ItemSelected += _ => ShowFields(view);
            left.AddChild(view.List);

            view.Status = new Label { Text = string.Empty };
            left.AddChild(view.Status);

            var right = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
            right.AddThemeConstantOverride("separation", 4);
            page.AddChild(right);

            var scroll = new ScrollContainer
            {
                SizeFlagsVertical = Control.SizeFlags.ExpandFill,
                HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
            };
            right.AddChild(scroll);

            view.Fields = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
            view.Fields.AddThemeConstantOverride("separation", 2);
            scroll.AddChild(view.Fields);

            views.Add(view);
        }

        foreach (CategoryView view in views)
        {
            Load(view);
        }
    }

    private void Load(CategoryView view)
    {
        int count = HiveCore.ObjectTypeCount(view.Category);
        for (int i = 0; i < count; i++)
        {
            string id = HiveCore.ObjectTypeId(view.Category, i) ?? string.Empty;
            if (id.Length == 0)
            {
                continue;
            }
            string name = HiveCore.ObjectTypeName(view.Category, id) ?? id;
            view.Ids.Add(id);
            view.Texts.Add($"{name}（{id}）");
            view.List.AddItem(view.Texts[^1]);
        }
        view.Status.Text = $"{view.Ids.Count} 个";
    }

    private void Refill(CategoryView view)
    {
        string keyword = view.Filter.Text.Trim();
        view.List.Clear();
        var visible = new List<int>();
        for (int i = 0; i < view.Texts.Count; i++)
        {
            if (keyword.Length == 0 || view.Texts[i].Contains(keyword, StringComparison.OrdinalIgnoreCase))
            {
                view.List.AddItem(view.Texts[i]);
                visible.Add(i);
            }
        }
        view.List.SetMeta("visible", visible.ToArray());
        view.Status.Text = $"{visible.Count} / {view.Ids.Count} 个";
    }

    private string SelectedId(CategoryView view)
    {
        int[] selected = view.List.GetSelectedItems();
        if (selected.Length == 0)
        {
            return string.Empty;
        }
        var visible = view.List.GetMeta("visible").AsInt32Array();
        if (visible.Length == 0)
        {
            return view.Ids[selected[0]];
        }
        return visible[selected[0]] < view.Ids.Count ? view.Ids[visible[selected[0]]] : string.Empty;
    }

    private void ShowFields(CategoryView view)
    {
        string id = SelectedId(view);
        if (id.Length == 0)
        {
            return;
        }

        foreach (Node child in view.Fields.GetChildren())
        {
            view.Fields.RemoveChild(child);
            child.QueueFree();
        }

        view.Fields.AddChild(new Label
        {
            Text = $"{HiveCore.ObjectTypeName(view.Category, id) ?? id}（{id}）",
            Modulate = new Color(0.8f, 0.85f, 1f),
        });

        int count = HiveCore.ObjectFieldCount(view.Category, id);
        for (int i = 0; i < count; i++)
        {
            int index = i;
            string label = HiveCore.ObjectFieldLabel(view.Category, i) ?? string.Empty;
            string value = HiveCore.ObjectValue(view.Category, id, i) ?? string.Empty;
            string description = HiveCore.ObjectFieldDescription(view.Category, i) ?? string.Empty;

            var row = new HBoxContainer();
            row.AddThemeConstantOverride("separation", 8);

            var name = new Label
            {
                Text = label,
                CustomMinimumSize = new Vector2(340, 0),
                AutowrapMode = TextServer.AutowrapMode.WordSmart,
                TooltipText = description.Length > 0 ? description : label,
                Modulate = HiveCore.ObjectValueModified(view.Category, id, i)
                    ? new Color(0.85f, 0.6f, 1f)
                    : Colors.White,
            };
            row.AddChild(name);

            var edit = new LineEdit
            {
                Text = value,
                SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            };
            edit.TextSubmitted += text => Commit(view, id, index, name, text);
            edit.FocusExited += () => Commit(view, id, index, name, edit.Text);
            row.AddChild(edit);

            view.Fields.AddChild(row);
        }
    }

    private void Commit(CategoryView view, string id, int index, Label label, string value)
    {
        if (!HiveCore.SetObjectValue(view.Category, id, index, value))
        {
            view.Status.Text = $"改不了 {id}";
            return;
        }
        label.Modulate = new Color(0.85f, 0.6f, 1f);
        view.Status.Text = $"{id} 已改，Ctrl+S 存进地图";
    }
}
