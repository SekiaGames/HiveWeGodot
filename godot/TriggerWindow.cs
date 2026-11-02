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
    private Tree ecaTree = null!;
    private VBoxContainer ecaDetail = null!;
    private readonly Dictionary<int, TreeItem> ecaItems = new();
    private readonly Dictionary<TreeItem, (int Parent, int Group)> groupTargets = new();
    private readonly Dictionary<TreeItem, int> sectionTypes = new();
    private int currentEcaNode = -1;
    private VBoxContainer scriptBox = null!;
    private CodeEdit globalScript = null!;
    private Label status = null!;

    private int current = -1;
    private readonly Dictionary<TreeItem, int> categoryIds = new();

    private static readonly Color Muted = new(0.65f, 0.7f, 0.78f);

    public void Build()
    {
        Title = "触发器";
        OkButtonText = "关闭";

        var tabs = new TabContainer
        {
            CustomMinimumSize = new Vector2(820, 540),
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
        };
        AddChild(tabs);

        var triggerPage = new HSplitContainer
        {
            Name = "触发器",
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
        };
        tabs.AddChild(triggerPage);

        tree = new Tree
        {
            HideRoot = true,
            CustomMinimumSize = new Vector2(280, 0),
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
        };
        tree.ItemSelected += OnTreeSelected;
        triggerPage.AddChild(tree);

        var right = new VBoxContainer
        {
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
        };
        right.AddThemeConstantOverride("separation", 6);
        triggerPage.AddChild(right);

        var buttons = new HBoxContainer();
        buttons.AddThemeConstantOverride("separation", 6);
        right.AddChild(buttons);

        var create = new Button { Text = "新建触发器" };
        create.Pressed += () =>
        {
            int parent = SelectedCategory();
            int index = HiveCore.CreateTrigger("New Trigger", parent);
            if (index >= 0)
            {
                Reload();
            }
        };
        buttons.AddChild(create);

        var remove = new Button { Text = "删除触发器" };
        remove.Pressed += () =>
        {
            if (current >= 0 && HiveCore.DeleteTrigger(current))
            {
                Reload();
            }
        };
        buttons.AddChild(remove);

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
        descriptionEdit = new TextEdit { CustomMinimumSize = new Vector2(0, 70) };
        descriptionEdit.FocusExited += () =>
        {
            if (current >= 0)
            {
                HiveCore.SetTriggerDescription(current, descriptionEdit.Text);
            }
        };
        right.AddChild(descriptionEdit);

        scriptBox = new VBoxContainer();
        scriptBox.AddThemeConstantOverride("separation", 4);
        right.AddChild(scriptBox);

        var ecaButtons = new HBoxContainer();
        ecaButtons.AddThemeConstantOverride("separation", 6);
        right.AddChild(ecaButtons);

        foreach ((string label, int type) in new[] { ("添加事件", 0), ("添加条件", 1), ("添加动作", 2) })
        {
            int kind = type;
            var addButton = new Button { Text = label };
            addButton.Pressed += () => ShowFunctionPicker(kind, label[2..]);
            ecaButtons.AddChild(addButton);
        }

        var ecaSplit = new VSplitContainer { SizeFlagsVertical = Control.SizeFlags.ExpandFill };
        right.AddChild(ecaSplit);

        ecaTree = new Tree
        {
            HideRoot = true,
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            CustomMinimumSize = new Vector2(0, 160),
        };
        ecaTree.ItemSelected += OnEcaSelected;
        ecaSplit.AddChild(ecaTree);

        var detailScroll = new ScrollContainer
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
            CustomMinimumSize = new Vector2(0, 120),
        };
        ecaSplit.AddChild(detailScroll);

        ecaDetail = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        ecaDetail.AddThemeConstantOverride("separation", 4);
        detailScroll.AddChild(ecaDetail);

        var scriptPage = new VBoxContainer
        {
            Name = "全局脚本",
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
        };
        scriptPage.AddThemeConstantOverride("separation", 6);
        tabs.AddChild(scriptPage);

        globalScript = new CodeEdit
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            GuttersDrawLineNumbers = true,
        };
        scriptPage.AddChild(globalScript);

        var scriptButtons = new HBoxContainer();
        scriptButtons.AddThemeConstantOverride("separation", 6);
        scriptPage.AddChild(scriptButtons);

        var applyScript = new Button { Text = "应用脚本" };
        applyScript.Pressed += () =>
        {
            HiveCore.SetMapGlobalScript(globalScript.Text);
            status.Text = "全局脚本已写回，Ctrl+S 存进地图";
        };
        scriptButtons.AddChild(applyScript);

        status = new Label { Text = string.Empty, Modulate = Muted };
        scriptButtons.AddChild(status);

        Reload();
    }

    private void ReloadScript()
    {
        globalScript.Text = HiveCore.MapGlobalScript ?? string.Empty;
    }

    public void Reload()
    {
        tree.Clear();
        categoryIds.Clear();
        TreeItem root = tree.CreateItem();

        var categoryItems = new Dictionary<int, TreeItem>();
        var childrenOf = new Dictionary<int, List<(int Id, string Name)>>();
        var known = new HashSet<int>();
        var roots = new List<(int Id, string Name)>();

        int categoryCount = HiveCore.TriggerCategoryCount;
        var categories = new List<(int Id, string Name, int Parent)>();
        for (int i = 0; i < categoryCount; i++)
        {
            (int id, string name, int parent) = HiveCore.TriggerCategory(i);
            categories.Add((id, name, parent));
            known.Add(id);
        }
        foreach (var entry in categories)
        {
            if (!childrenOf.TryGetValue(entry.Parent, out var list))
            {
                list = new List<(int, string)>();
                childrenOf[entry.Parent] = list;
            }
            list.Add((entry.Id, entry.Name));
        }
        foreach (var entry in categories)
        {
            if (!known.Contains(entry.Parent))
            {
                roots.Add((entry.Id, entry.Name));
            }
        }

        TreeItem CreateCategoryItem(TreeItem parentItem, int id, string name)
        {
            TreeItem item = tree.CreateItem(parentItem);
            item.SetText(0, name);
            item.SetMetadata(0, -1);
            categoryItems[id] = item;
            categoryIds[item] = id;
            if (childrenOf.TryGetValue(id, out var children))
            {
                foreach (var (childId, childName) in children)
                {
                    CreateCategoryItem(item, childId, childName);
                }
            }
            return item;
        }

        foreach (var (id, name) in roots)
        {
            CreateCategoryItem(root, id, name);
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

        ReloadScript();

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

    private int SelectedCategory()
    {
        TreeItem? item = tree.GetSelected();
        if (item == null)
        {
            return 0;
        }
        int index = (int)item.GetMetadata(0);
        if (index >= 0)
        {
            return HiveCore.Trigger(index).Parent;
        }
        return categoryIds.TryGetValue(item, out int id) ? id : 0;
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
            BuildEcaTree(-1);
            return;
        }

        (int id, string name, _) = HiveCore.Trigger(index);
        titleLabel.Text = $"触发器 #{id}";
        nameEdit.Text = name;
        descriptionEdit.Text = HiveCore.TriggerDescription(index) ?? string.Empty;
        enabledBox.ButtonPressed = HiveCore.TriggerIsEnabled(index);
        initiallyBox.ButtonPressed = HiveCore.TriggerInitiallyOn(index);

        ClearScript();
        if (HiveCore.TriggerIsScript(index) || (HiveCore.TriggerCustomText(index) ?? string.Empty).Length > 0)
        {
            scriptBox.AddChild(new Label { Text = "自定义脚本" });
            var edit = new CodeEdit
            {
                Text = HiveCore.TriggerCustomText(index) ?? string.Empty,
                CustomMinimumSize = new Vector2(0, 110),
                GuttersDrawLineNumbers = true,
            };
            edit.FocusExited += () => HiveCore.SetTriggerCustomText(index, edit.Text);
            scriptBox.AddChild(edit);
        }

        BuildEcaTree(index);
    }

    private void BuildEcaTree(int index)
    {
        ecaTree.Clear();
        ecaItems.Clear();
        groupTargets.Clear();
        sectionTypes.Clear();
        currentEcaNode = -1;
        ClearEcaDetail();

        TreeItem root = ecaTree.CreateItem();
        var sections = new TreeItem[3];
        string[] titles = { "事件", "条件", "动作" };
        for (int t = 0; t < 3; t++)
        {
            sections[t] = ecaTree.CreateItem(root);
            sections[t].SetText(0, titles[t]);
            sections[t].SetMetadata(0, -2);
            sections[t].SetCustomColor(0, Muted);
            sectionTypes[sections[t]] = t;
        }

        if (index >= 0)
        {
            int nodes = HiveCore.TriggerBuildNodes(index);
            var groupHeaders = new Dictionary<int, List<TreeItem>>();

            for (int node = 0; node < nodes; node++)
            {
                int depth = HiveCore.TriggerNodeDepth(index, node);
                int type = HiveCore.TriggerNodeType(index, node);
                int group = HiveCore.TriggerNodeGroup(index, node);

                TreeItem parentItem = depth == 0
                    ? sections[type is >= 0 and < 3 ? type : 2]
                    : groupHeaders.TryGetValue(depth, out List<TreeItem>? headers) && group >= 0 && group < headers.Count
                        ? headers[group]
                        : sections[type is >= 0 and < 3 ? type : 2];

                TreeItem item = ecaTree.CreateItem(parentItem);
                item.SetText(0, HiveCore.TriggerNodeDisplay(index, node) ?? "?");
                item.SetMetadata(0, node);
                ecaItems[node] = item;

                int kind = HiveCore.TriggerNodeGroupKind(index, node);
                if (kind == 0)
                {
                    continue;
                }

                string name = HiveCore.TriggerNodeName(index, node) ?? string.Empty;
                string[] labels = kind == 3
                    ? new[] { "If - 条件", "Then - 动作", "Else - 动作" }
                    : new[] { name.StartsWith("For") ? "循环 - 动作" : "条件" };

                var created = new List<TreeItem>();
                for (int g = 0; g < labels.Length; g++)
                {
                    TreeItem header = ecaTree.CreateItem(item);
                    header.SetText(0, labels[g]);
                    header.SetMetadata(0, -1);
                    header.SetCustomColor(0, Muted);
                    groupTargets[header] = (node, g);
                    created.Add(header);
                }
                groupHeaders[depth + 1] = created;
            }
        }

        foreach (TreeItem section in sections)
        {
            section.SetCollapsed(false);
        }
    }

    public void SelectFirstEca()
    {
        TreeItem? best = null;
        foreach (var pair in ecaItems)
        {
            if (HiveCore.TriggerNodeParameterCount(current, pair.Key) == 0)
            {
                continue;
            }
            if (best == null || pair.Key < (int)best.GetMetadata(0))
            {
                best = pair.Value;
            }
        }

        if (best == null)
        {
            return;
        }
        best.Select(0);
        ecaTree.ScrollToItem(best);
    }

    private void OnEcaSelected()
    {
        TreeItem? item = ecaTree.GetSelected();
        if (item == null)
        {
            return;
        }

        int node = (int)item.GetMetadata(0);
        currentEcaNode = node;
        ClearEcaDetail();
        if (node >= 0 && current >= 0)
        {
            ShowEcaDetail(current, node);
        }
    }

    private void ShowEcaDetail(int index, int node)
    {
        int depth = HiveCore.TriggerNodeDepth(index, node);
        ecaDetail.AddChild(new Label
        {
            Text = $"{HiveCore.TriggerNodeName(index, node)}　第 {depth + 1} 层",
            Modulate = Muted,
        });

        int parameters = HiveCore.TriggerNodeParameterCount(index, node);
        if (parameters == 0)
        {
            ecaDetail.AddChild(new Label { Text = "没有参数", Modulate = Muted });
        }

        for (int p = 0; p < parameters; p++)
        {
            int slot = p;
            var row = new HBoxContainer();
            row.AddThemeConstantOverride("separation", 6);
            row.AddChild(new Label
            {
                Text = HiveCore.TriggerNodeParameterTypeName(index, node, p) ?? $"参数 {p + 1}",
                CustomMinimumSize = new Vector2(110, 0),
                Modulate = Muted,
            });

            var edit = new LineEdit
            {
                Text = HiveCore.TriggerNodeParameterValue(index, node, p) ?? string.Empty,
                SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            };
            edit.TextSubmitted += text => CommitEcaParameter(node, slot, text);
            edit.FocusExited += () => CommitEcaParameter(node, slot, edit.Text);
            row.AddChild(edit);

            ecaDetail.AddChild(row);
        }

        var remove = new Button { Text = "删除这一条" };
        remove.Pressed += () =>
        {
            if (HiveCore.RemoveTriggerNode(index, node))
            {
                ShowTrigger(index);
            }
        };
        ecaDetail.AddChild(remove);
    }

    private void CommitEcaParameter(int node, int parameter, string value)
    {
        if (current < 0)
        {
            return;
        }
        HiveCore.SetTriggerNodeParameter(current, node, parameter, value);
        if (ecaItems.TryGetValue(node, out TreeItem? item))
        {
            item.SetText(0, HiveCore.TriggerNodeDisplay(current, node) ?? "?");
        }
    }

    private void ShowFunctionPicker(int type, string title)
    {
        if (current < 0)
        {
            status.Text = "先选一个触发器";
            return;
        }

        int parent = -1;
        int group = 0;
        TreeItem? selected = ecaTree.GetSelected();
        if (selected != null && groupTargets.TryGetValue(selected, out (int Parent, int Group) target))
        {
            parent = target.Parent;
            group = target.Group;
        }

        var dialog = new AcceptDialog
        {
            Title = $"添加{title}",
            OkButtonText = "添加",
        };

        var column = new VBoxContainer { CustomMinimumSize = new Vector2(420, 420) };
        column.AddThemeConstantOverride("separation", 6);

        var filter = new LineEdit { PlaceholderText = "按名字筛选" };
        column.AddChild(filter);

        var list = new ItemList { SizeFlagsVertical = Control.SizeFlags.ExpandFill };
        column.AddChild(list);

        int total = HiveCore.TriggerFunctionCount(type);
        var names = new string[total];
        for (int i = 0; i < total; i++)
        {
            names[i] = HiveCore.TriggerFunctionName(type, i) ?? string.Empty;
        }

        void Fill(string keyword)
        {
            list.Clear();
            foreach (string name in names)
            {
                if (keyword.Length == 0 || name.Contains(keyword, StringComparison.OrdinalIgnoreCase))
                {
                    list.AddItem(name);
                }
            }
        }

        filter.TextChanged += _ => Fill(filter.Text);
        Fill(string.Empty);

        dialog.Confirmed += () =>
        {
            int[] chosen = list.GetSelectedItems();
            if (chosen.Length > 0)
            {
                string name = list.GetItemText(chosen[0]);
                if (HiveCore.AddTriggerNode(current, type, name, parent, group))
                {
                    ShowTrigger(current);
                }
            }
            dialog.QueueFree();
        };
        dialog.Canceled += () => dialog.QueueFree();

        dialog.AddChild(column);
        AddChild(dialog);
        dialog.PopupCentered(new Vector2I(460, 520));
    }

    private void ClearEcaDetail()
    {
        foreach (Node child in ecaDetail.GetChildren())
        {
            ecaDetail.RemoveChild(child);
            child.QueueFree();
        }
    }

    private void ClearScript()
    {
        foreach (Node child in scriptBox.GetChildren())
        {
            scriptBox.RemoveChild(child);
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
