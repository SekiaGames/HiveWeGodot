using System;
using Godot;

namespace HiveWeGodot;

public partial class PlayerWindow : AcceptDialog
{
    private const int SlotCount = 24;

    private static readonly string[] Controllers = { "无", "用户", "电脑", "中立", "可营救" };
    private static readonly string[] Races = { "可选", "人族", "兽族", "不死族", "暗夜精灵族" };

    private readonly OptionButton[] controllerBoxes = new OptionButton[SlotCount];
    private readonly LineEdit[] nameEdits = new LineEdit[SlotCount];
    private readonly OptionButton[] raceBoxes = new OptionButton[SlotCount];
    private readonly CheckBox[] fixedBoxes = new CheckBox[SlotCount];

    private Label status = null!;

    public void Build()
    {
        Title = "玩家";
        OkButtonText = "关闭";

        var column = new VBoxContainer { CustomMinimumSize = new Vector2(720, 560) };
        column.AddThemeConstantOverride("separation", 6);
        AddChild(column);

        var header = new GridContainer { Columns = 6 };
        header.AddThemeConstantOverride("h_separation", 8);
        column.AddChild(header);
        foreach (string text in new[] { "序号", "控制器", "名字", "颜色", "种族", "固定出生点" })
        {
            header.AddChild(new Label { Text = text });
        }

        var scroll = new ScrollContainer
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
        };
        column.AddChild(scroll);

        var grid = new GridContainer { Columns = 6, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        grid.AddThemeConstantOverride("h_separation", 8);
        grid.AddThemeConstantOverride("v_separation", 2);
        scroll.AddChild(grid);

        for (int slot = 0; slot < SlotCount; slot++)
        {
            int index = slot;
            grid.AddChild(new Label { Text = (slot + 1).ToString(), CustomMinimumSize = new Vector2(36, 0) });

            controllerBoxes[slot] = new OptionButton { CustomMinimumSize = new Vector2(110, 0) };
            foreach (string text in Controllers)
            {
                controllerBoxes[slot].AddItem(text);
            }
            controllerBoxes[slot].ItemSelected += choice => OnControllerChanged(index, (int)choice);
            grid.AddChild(controllerBoxes[slot]);

            nameEdits[slot] = new LineEdit { CustomMinimumSize = new Vector2(150, 0) };
            grid.AddChild(nameEdits[slot]);

            Vector3 rgb = HiveCore.PlayerColor(slot);
            grid.AddChild(new ColorRect
            {
                Color = new Color(rgb.X, rgb.Y, rgb.Z),
                CustomMinimumSize = new Vector2(42, 20),
            });

            raceBoxes[slot] = new OptionButton { CustomMinimumSize = new Vector2(120, 0) };
            foreach (string text in Races)
            {
                raceBoxes[slot].AddItem(text);
            }
            grid.AddChild(raceBoxes[slot]);

            fixedBoxes[slot] = new CheckBox();
            grid.AddChild(fixedBoxes[slot]);
        }

        status = new Label { Text = string.Empty };
        column.AddChild(status);

        var buttons = new HBoxContainer();
        buttons.AddThemeConstantOverride("separation", 6);
        column.AddChild(buttons);

        var apply = new Button { Text = "应用" };
        apply.Pressed += Apply;
        buttons.AddChild(apply);

        var defaults = new Button { Text = "恢复默认" };
        defaults.Pressed += RestoreDefaults;
        buttons.AddChild(defaults);

        Load();
    }

    private void Load()
    {
        int used = 0;
        for (int slot = 0; slot < SlotCount; slot++)
        {
            int type = HiveCore.PlayerSlotType(slot);
            controllerBoxes[slot].Select(Math.Max(0, type + 1));
            if (type >= 0)
            {
                used++;
                nameEdits[slot].Text = HiveCore.PlayerSlotName(slot) ?? $"Player {slot + 1}";
                raceBoxes[slot].Select(Math.Max(0, HiveCore.PlayerSlotRace(slot)));
                fixedBoxes[slot].ButtonPressed = HiveCore.PlayerSlotFixed(slot) != 0;
            }
            else
            {
                nameEdits[slot].Text = $"Player {slot + 1}";
                raceBoxes[slot].Select(slot % 4 + 1);
                fixedBoxes[slot].ButtonPressed = false;
            }
            SetRowEnabled(slot, type >= 0);
        }
        status.Text = $"{used} 个槽位有人";
    }

    private void SetRowEnabled(int slot, bool enabled)
    {
        nameEdits[slot].Editable = enabled;
        raceBoxes[slot].Disabled = !enabled;
        fixedBoxes[slot].Disabled = !enabled;
    }

    private void OnControllerChanged(int slot, int choice)
    {
        if (choice == 0)
        {
            SetRowEnabled(slot, false);
            return;
        }

        if (choice != 1 && CountUsers() == 0)
        {
            controllerBoxes[slot].Select(1);
            status.Text = "至少留一个用户控制的玩家";
            return;
        }

        SetRowEnabled(slot, true);
    }

    private int CountUsers()
    {
        int users = 0;
        for (int slot = 0; slot < SlotCount; slot++)
        {
            if (controllerBoxes[slot].Selected == 1)
            {
                users++;
            }
        }
        return users;
    }

    public void Apply()
    {
        for (int slot = 0; slot < SlotCount; slot++)
        {
            int choice = controllerBoxes[slot].Selected;
            int type = choice == 0 ? -1 : choice - 1;
            HiveCore.SetPlayerSlot(slot, type, raceBoxes[slot].Selected, nameEdits[slot].Text,
                fixedBoxes[slot].ButtonPressed ? 1 : 0);
        }

        int used = 0;
        for (int slot = 0; slot < SlotCount; slot++)
        {
            if (HiveCore.PlayerSlotType(slot) >= 0)
            {
                used++;
            }
        }
        status.Text = $"已应用，{used} 个槽位有人，Ctrl+S 存进地图";
    }

    public void RestoreDefaults()
    {
        for (int slot = 0; slot < SlotCount; slot++)
        {
            controllerBoxes[slot].Select(slot == 0 ? 1 : 0);
            nameEdits[slot].Text = $"Player {slot + 1}";
            raceBoxes[slot].Select(slot == 0 ? 0 : slot % 4 + 1);
            fixedBoxes[slot].ButtonPressed = false;
            SetRowEnabled(slot, slot == 0);
        }
        status.Text = "已恢复默认，点「应用」写入";
    }
}
