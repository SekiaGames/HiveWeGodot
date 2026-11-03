using System;
using Godot;

namespace HiveWeGodot;

public partial class RegionWindow : AcceptDialog
{
    private ItemList list = null!;
    private LineEdit nameEdit = null!;
    private Label info = null!;
    private ColorPickerButton colorButton = null!;
    private OptionButton weatherBox = null!;
    private OptionButton ambientBox = null!;
    private readonly System.Collections.Generic.List<string> weatherIds = new();
    private readonly System.Collections.Generic.List<string> ambientIds = new();

    private Func<Vector2> centerProvider = null!;

    public event Action? RegionsChanged;

    public void Build(Func<Vector2> center)
    {
        Title = "区域";
        OkButtonText = "关闭";
        centerProvider = center;

        var column = new VBoxContainer { CustomMinimumSize = new Vector2(460, 420) };
        column.AddThemeConstantOverride("separation", 6);
        AddChild(column);

        list = new ItemList { SizeFlagsVertical = Control.SizeFlags.ExpandFill };
        list.ItemSelected += _ => OnSelected();
        column.AddChild(list);

        info = new Label { Text = "选中一个区域查看范围", AutowrapMode = TextServer.AutowrapMode.WordSmart };
        column.AddChild(info);

        var nameRow = new HBoxContainer();
        nameRow.AddChild(new Label { Text = "名字" });
        nameEdit = new LineEdit { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        nameEdit.TextSubmitted += _ => ApplyName();
        nameEdit.FocusExited += ApplyName;
        nameRow.AddChild(nameEdit);
        column.AddChild(nameRow);

        var colorRow = new HBoxContainer();
        colorRow.AddChild(new Label { Text = "颜色" });
        // 区域颜色的 alpha 在 w3r 里读回来会被红色通道覆盖（上游如此），界面不暴露它
        colorButton = new ColorPickerButton { CustomMinimumSize = new Vector2(80, 0), EditAlpha = false };
        colorButton.ColorChanged += ApplyColor;
        colorRow.AddChild(colorButton);
        column.AddChild(colorRow);

        var weatherRow = new HBoxContainer();
        weatherRow.AddChild(new Label { Text = "天气" });
        weatherBox = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        weatherBox.AddItem("无");
        weatherIds.Add(string.Empty);
        for (int i = 0; i < HiveCore.WeatherCount(); i++)
        {
            weatherBox.AddItem(HiveCore.WeatherName(i) ?? HiveCore.WeatherId(i) ?? "?");
            weatherIds.Add(HiveCore.WeatherId(i) ?? string.Empty);
        }
        weatherBox.ItemSelected += choice => ApplyWeather((int)choice);
        weatherRow.AddChild(weatherBox);
        column.AddChild(weatherRow);

        var ambientRow = new HBoxContainer();
        ambientRow.AddChild(new Label { Text = "环境音" });
        ambientBox = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        ambientBox.AddItem("无");
        ambientIds.Add(string.Empty);
        for (int i = 0; i < HiveCore.SoundCount(); i++)
        {
            ambientBox.AddItem(HiveCore.SoundName(i) ?? HiveCore.SoundVariable(i) ?? "?");
            ambientIds.Add(HiveCore.SoundVariable(i) ?? string.Empty);
        }
        ambientBox.ItemSelected += choice => ApplyAmbient((int)choice);
        ambientRow.AddChild(ambientBox);
        column.AddChild(ambientRow);

        var buttons = new HBoxContainer();
        buttons.AddThemeConstantOverride("separation", 6);
        column.AddChild(buttons);

        var create = new Button { Text = "在视野中心新建" };
        create.Pressed += () =>
        {
            Vector2 center = centerProvider();
            if (HiveCore.CreateRegion(center.X - 4f, center.Y - 4f, center.X + 4f, center.Y + 4f) >= 0)
            {
                Reload();
                RegionsChanged?.Invoke();
            }
        };
        buttons.AddChild(create);

        var remove = new Button { Text = "删除选中" };
        remove.Pressed += () =>
        {
            int selected = Selectedindex();
            if (selected >= 0 && HiveCore.DeleteRegion(selected))
            {
                Reload();
                RegionsChanged?.Invoke();
            }
        };
        buttons.AddChild(remove);

        Reload();
    }

    public void Reload()
    {
        list.Clear();
        int count = HiveCore.MapInfo.RegionCount;
        for (int i = 0; i < count; i++)
        {
            string name = HiveCore.RegionName(i) ?? $"区域 {i}";
            list.AddItem($"{name}  #{i}");
        }
        info.Text = $"共 {count} 个区域";
        nameEdit.Text = string.Empty;
    }

    private int Selectedindex()
    {
        int[] selected = list.GetSelectedItems();
        return selected.Length > 0 ? selected[0] : -1;
    }

    private void OnSelected()
    {
        int index = Selectedindex();
        if (index < 0)
        {
            return;
        }
        HiveRegion[] regions = HiveCore.Regions();
        if (index >= regions.Length)
        {
            return;
        }
        HiveRegion region = regions[index];
        info.Text = $"{region.Left:F0},{region.Bottom:F0} - {region.Right:F0},{region.Top:F0}"
            + $"，颜色 {region.Red},{region.Green},{region.Blue},{region.Alpha}";
        nameEdit.Text = HiveCore.RegionName(index) ?? string.Empty;
        colorButton.Color = new Color(region.Red / 255f, region.Green / 255f, region.Blue / 255f);
        weatherBox.Select(Math.Max(0, weatherIds.IndexOf(HiveCore.RegionWeather(index) ?? string.Empty)));
        ambientBox.Select(Math.Max(0, ambientIds.IndexOf(HiveCore.RegionAmbient(index) ?? string.Empty)));
    }

    private void ApplyColor(Color color)
    {
        int index = Selectedindex();
        if (index >= 0 && HiveCore.SetRegionColor(index, color))
        {
            RegionsChanged?.Invoke();
        }
    }

    private void ApplyWeather(int choice)
    {
        int index = Selectedindex();
        if (index >= 0 && choice >= 0 && choice < weatherIds.Count && HiveCore.SetRegionWeather(index, weatherIds[choice]))
        {
            RegionsChanged?.Invoke();
        }
    }

    private void ApplyAmbient(int choice)
    {
        int index = Selectedindex();
        if (index >= 0 && choice >= 0 && choice < ambientIds.Count && HiveCore.SetRegionAmbient(index, ambientIds[choice]))
        {
            RegionsChanged?.Invoke();
        }
    }

    private void ApplyName()
    {
        int index = Selectedindex();
        if (index >= 0 && nameEdit.Text.Length > 0)
        {
            HiveCore.SetRegionName(index, nameEdit.Text);
            list.SetItemText(index, $"{nameEdit.Text}  #{index}");
            RegionsChanged?.Invoke();
        }
    }
}
