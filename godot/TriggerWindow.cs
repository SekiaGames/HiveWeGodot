using System;
using System.Collections.Generic;
using Godot;

namespace HiveWeGodot;

public partial class TriggerWindow : AcceptDialog
{
    private Tree tree = null!;
    private Label titleLabel = null!;
    private LineEdit nameEdit = null!;
    private TextEdit descriptionEdit = null!;
    private CheckBox enabledBox = null!;
    private CheckBox initiallyBox = null!;
    private VBoxContainer ecaColumn = null!;

    private int current = -1;

    private static readonly Color Muted = new(0.65f, 0.7f, 0.78f);

    public void Build()
    {
        Title = "触发器";
        OkButtonText = "关闭";

        var split = new HSplitContainer
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            CustomMinimumSize = new Vector2(820, 540),
        };
        AddChild(split);

        tree = new Tree
        {
            HideRoot = true,
            CustomMinimumSize = new Vector2(280, 0),
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
        };
        tree.ItemSelected += OnTreeSelected;
        split.AddChild(tree);

        var right = new VBoxContainer
        {
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
        };
        right.AddThemeConstantOverride("separation", 6);
        split.AddChild(right);

        titleLabel = new Label { Text = "未选择" };
        right.AddChild(titleLabel);

        var nameRow = new HBoxContainer();
        nameRow.AddChild(new Label { Text = "名称" });
        nameEdit = new LineEdit { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        nameEdit.TextSubmitted += _ => CommitName();
        nameEdit.FocusExited += CommitName;
        nameRow.AddChild(nameEdit);
        right.AddChild(nameRow);

        var flagsRow = new HBoxContainer();
        flagsRow.AddThemeConstantOverride("separation", 12);
        enabledBox = new CheckBox { Text = "启用" };
        enabledBox.Toggled += on =>
        {
            if (current >= 0)
            {
                HiveCore.SetTriggerEnabled(current, on);
            }
        };
        flagsRow.AddChild(enabledBox);

        initiallyBox = new CheckBox { Text = "初始开启" };
        initiallyBox.Toggled += on =>
        {
            if (current >= 0)
            {
                HiveCore.SetTriggerInitiallyOn(current, on);
            }
        };
        flagsRow.AddChild(initiallyBox);
        right.AddChild(flagsRow);

        right.AddChild(new Label { Text = "描述" });
        descriptionEdit = new TextEdit { CustomMinimumSize = new Vector2(0, 80) };
        descriptionEdit.FocusExited += () =>
        {
            if (current >= 0)
            {
                HiveCore.SetTriggerDescription(current, descriptionEdit.Text);
            }
        };
        right.AddChild(descriptionEdit);

        var scroll = new ScrollContainer
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
        };
        right.AddChild(scroll);

        ecaColumn = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        ecaColumn.AddThemeConstantOverride("separation", 4);
        scroll.AddChild(ecaColumn);

        Reload();
    }

    public void Reload()
    {
        tree.Clear();
        TreeItem root = tree.CreateItem();

        var categoryItems = new Dictionary<int, TreeItem>();
        var categoryParents = new Dictionary<TreeItem, int>();

        int categoryCount = HiveCore.TriggerCategoryCount;
        for (int i = 0; i < categoryCount; i++)
        {
            (int id, string name, int parent) = HiveCore.TriggerCategory(i);
            TreeItem item = tree.CreateItem(root);
            item.SetText(0, name);
            item.SetMetadata(0, -1);
            categoryItems[id] = item;
            categoryParents[item] = parent;
        }

        foreach (var pair in categoryParents)
        {
            if (categoryItems.TryGetValue(pair.Value, out TreeItem? parentItem))
            {
                parentItem.AddChild(pair.Key);
            }
        }

        int triggerCount = HiveCore.TriggerCount;
        TreeItem? firstTrigger = null;
        for (int i = 0; i < triggerCount; i++)
        {
            (_, string name, int parent) = HiveCore.Trigger(i);
            TreeItem anchor = categoryItems.TryGetValue(parent, out TreeItem? parentItem) ? parentItem : root;
            TreeItem item = tree.CreateItem(anchor);
            item.SetText(0, HiveCore.TriggerIsComment(i) ? $"// {name}" : name);
            item.SetMetadata(0, i);
            if (!HiveCore.TriggerIsEnabled(i))
            {
                item.SetCustomColor(0, Muted);
            }
            firstTrigger ??= item;
        }

        int variableCount = HiveCore.TriggerVariableCount;
        if (variableCount > 0 && categoryItems.TryGetValue(1, out TreeItem? variableRoot))
        {
            for (int i = 0; i < variableCount; i++)
            {
                (string name, string type, bool isArray, _) = HiveCore.TriggerVariable(i);
                TreeItem item = tree.CreateItem(variableRoot);
                item.SetText(0, $"{name} : {type}{(isArray ? " 数组" : string.Empty)}");
                item.SetMetadata(0, -1);
                item.SetCustomColor(0, Muted);
            }
        }

        tree.SetColumnTitle(0, $"分类 {categoryCount}，触发器 {triggerCount}，变量 {variableCount}");

        if (firstTrigger != null)
        {
            firstTrigger.Select(0);
            tree.ScrollToItem(firstTrigger);
            ShowTrigger((int)firstTrigger.GetMetadata(0));
        }
        else
        {
            ShowTrigger(-1);
        }
    }

    private void OnTreeSelected()
    {
        TreeItem? item = tree.GetSelected();
        if (item == null)
        {
            return;
        }
        int index = (int)item.GetMetadata(0);
        if (index != current)
        {
            ShowTrigger(index);
        }
    }

    private void ShowTrigger(int index)
    {
        current = index;

        if (index < 0)
        {
            titleLabel.Text = "未选择触发器";
            nameEdit.Text = string.Empty;
            descriptionEdit.Text = string.Empty;
            enabledBox.ButtonPressed = false;
            initiallyBox.ButtonPressed = false;
            ClearEca();
            return;
        }

        (int id, string name, _) = HiveCore.Trigger(index);
        titleLabel.Text = $"触发器 #{id}";
        nameEdit.Text = name;
        descriptionEdit.Text = HiveCore.TriggerDescription(index) ?? string.Empty;
        enabledBox.ButtonPressed = HiveCore.TriggerIsEnabled(index);
        initiallyBox.ButtonPressed = HiveCore.TriggerInitiallyOn(index);

        ClearEca();
        ListEca(index, 0, "事件");
        ListEca(index, 1, "条件");
        ListEca(index, 2, "动作");
    }

    private void ListEca(int index, int type, string title)
    {
        int count = HiveCore.TriggerEcaCount(index, type);
        ecaColumn.AddChild(new Label
        {
            Text = $"{title}（{count}）",
            Modulate = Muted,
        });
        for (int i = 0; i < count; i++)
        {
            string name = HiveCore.TriggerEcaName(index, type, i) ?? "?";
            int parameters = HiveCore.TriggerEcaParameterCount(index, type, i);

            var values = new List<string>();
            for (int p = 0; p < parameters; p++)
            {
                int kind = HiveCore.TriggerEcaParameterType(index, type, i, p);
                string value = HiveCore.TriggerEcaParameterValue(index, type, i, p) ?? string.Empty;
                values.Add(kind switch
                {
                    0 => value,
                    1 => value,
                    2 => $"{value}()",
                    3 => $"\"{value}\"",
                    _ => value,
                });
            }

            string text = values.Count == 0
                ? $"  {name}"
                : $"  {name}（{string.Join("，", values)}）";
            ecaColumn.AddChild(new Label { Text = text, AutowrapMode = TextServer.AutowrapMode.WordSmart });
        }
    }

    private void ClearEca()
    {
        foreach (Node child in ecaColumn.GetChildren())
        {
            ecaColumn.RemoveChild(child);
            child.QueueFree();
        }
    }

    private void CommitName()
    {
        if (current < 0)
        {
            return;
        }
        HiveCore.SetTriggerName(current, nameEdit.Text);
    }
}
