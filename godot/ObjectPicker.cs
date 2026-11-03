using System;
using System.Collections.Generic;
using Godot;

namespace HiveWeGodot;

public partial class ObjectPicker : AcceptDialog
{
    private Tree folderTree = null!;
    private ItemList list = null!;
    private LineEdit filter = null!;
    private Label info = null!;

    private const char FolderSeparator = '\u0001';

    private readonly List<(string Id, string Name, string[] Folder)> entries = new();
    private readonly List<string> shown = new();
    private string currentFolder = "全部";

    private Action<string>? chosen;

    public int EntryCount => entries.Count;

    public int ShownCount => shown.Count;

    public string Describe(int index)
        => $"{entries[index].Name}（{entries[index].Id}）在 {string.Join("/", entries[index].Folder)}";

    public void SetFilter(string text)
    {
        filter.Text = text;
        Refill();
    }

    public void Build(bool isUnit, string[] ids, Action<string> onChosen)
    {
        chosen = onChosen;
        Title = isUnit ? "选单位" : "选装饰物";
        OkButtonText = "确定";

        var column = new VBoxContainer { CustomMinimumSize = new Vector2(720, 520) };
        column.AddThemeConstantOverride("separation", 6);
        AddChild(column);

        filter = new LineEdit { PlaceholderText = "按名字或 id 筛选" };
        filter.TextChanged += _ => Refill();
        column.AddChild(filter);

        var split = new HSplitContainer { SizeFlagsVertical = Control.SizeFlags.ExpandFill };
        column.AddChild(split);

        folderTree = new Tree
        {
            HideRoot = true,
            CustomMinimumSize = new Vector2(180, 0),
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
        };
        folderTree.ItemSelected += OnFolderSelected;
        split.AddChild(folderTree);

        list = new ItemList { SizeFlagsVertical = Control.SizeFlags.ExpandFill };
        list.ItemActivated += _ => PickAndClose();
        split.AddChild(list);

        info = new Label { Text = "..." };
        column.AddChild(info);

        Load(isUnit, ids);
        BuildFolders();
        Refill();

        Confirmed += () =>
        {
            PickSelected();
            QueueFree();
        };
        Canceled += QueueFree;
    }

    private void Load(bool isUnit, string[] ids)
    {
        foreach (string id in ids)
        {
            int category = isUnit ? 0 : HiveCore.DoodadCategory(id);
            if (category < 0)
            {
                continue;
            }

            string name = HiveCore.ObjectTypeName(category, id) ?? id;
            entries.Add((id, name, HiveCore.ObjectTypeFolders(category, id)));
        }

        GD.Print($"对象列表 {entries.Count} 个");
    }

    private void BuildFolders()
    {
        folderTree.Clear();
        TreeItem root = folderTree.CreateItem();

        var groups = new Dictionary<string, TreeItem>();
        foreach ((string _, string _, string[] folder) in entries)
        {
            TreeItem parent = root;
            string path = string.Empty;
            foreach (string part in folder)
            {
                path = path.Length == 0 ? part : path + FolderSeparator + part;
                if (!groups.TryGetValue(path, out TreeItem? item))
                {
                    item = folderTree.CreateItem(parent);
                    item.SetText(0, part);
                    item.SetMetadata(0, path);
                    groups[path] = item;
                }
                parent = item;
            }
        }

        TreeItem all = folderTree.CreateItem(root);
        all.SetText(0, "全部");
        all.SetMetadata(0, string.Empty);
        all.Select(0);

        root.SetCollapsed(false);
        foreach (TreeItem item in root.GetChildren())
        {
            item.SetCollapsed(false);
        }
        currentFolder = string.Empty;
    }

    private void OnFolderSelected()
    {
        TreeItem? item = folderTree.GetSelected();
        if (item == null)
        {
            return;
        }
        currentFolder = item.GetMetadata(0).AsString();
        Refill();
    }

    private void Refill()
    {
        list.Clear();
        shown.Clear();

        string keyword = filter.Text.Trim();
        foreach ((string id, string name, string[] folder) in entries)
        {
            if (currentFolder.Length > 0)
            {
                string path = string.Join(FolderSeparator, folder);
                if (!(path == currentFolder || path.StartsWith(currentFolder + FolderSeparator)))
                {
                    continue;
                }
            }
            if (keyword.Length > 0
                && !name.Contains(keyword, StringComparison.OrdinalIgnoreCase)
                && !id.Contains(keyword, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            list.AddItem($"{name}（{id}）");
            shown.Add(id);
        }

        info.Text = $"{shown.Count} 个";
    }

    private string SelectedId()
    {
        int[] selected = list.GetSelectedItems();
        return selected.Length == 0 ? string.Empty : shown[selected[0]];
    }

    private void PickSelected()
    {
        string id = SelectedId();
        if (id.Length > 0)
        {
            chosen?.Invoke(id);
        }
    }

    private void PickAndClose()
    {
        PickSelected();
        QueueFree();
    }
}
