using Godot;

namespace HiveWeGodot;

public partial class MapProtectionWindow : AcceptDialog
{
    private CheckBox editorFiles = null!;
    private CheckBox metadata = null!;
    private CheckBox minify = null!;
    private CheckBox junkHeader = null!;
    private CheckBox encryptImports = null!;

    public void Build()
    {
        Title = "地图保护";
        OkButtonText = "导出";
        AddCancelButton("取消");

        var column = new VBoxContainer { CustomMinimumSize = new Vector2(520, 340) };
        column.AddThemeConstantOverride("separation", 10);
        AddChild(column);

        var notice = new Label
        {
            Text = "保护只是让别人更难打开你的地图，无法做到绝对打不开。\n"
                + "也可以考虑不保护直接发布，这样你不在之后别人还能帮你修 bug。",
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
        };
        column.AddChild(notice);

        column.AddChild(Group("有效", "这些选项会真的去掉信息，导出后无法还原。", out VBoxContainer effective));
        editorFiles = Check("去掉只有编辑器才需要的文件", "地图放进游戏里用不到的编辑器文件不再写进去。", effective);
        editorFiles.ButtonPressed = true;
        metadata = Check("不写 MPQ 的目录信息（listfile、attributes）",
            "别人没法直接列出包里有哪些文件。", effective);
        metadata.ButtonPressed = true;
        minify = Check("压缩地图脚本（war3map.j / war3map.lua）",
            "去掉注释行和行首缩进，不改标识符。", effective);

        column.AddChild(Group("效果有限", "只能挡住过时的工具，现代解包器照样能开。", out VBoxContainer weak));
        junkHeader = Check("文件头前加垃圾数据", "地图文件开头塞一段垃圾，MPQ 头不再在偏移 0。游戏能开，"
            + "默认从 0 开始读的工具会失败。", weak);
        encryptImports = Check("加密导入的文件", "没有文件名就解不出来。V1 的 MPQ 加密早被破解，挡不住多少人。", weak);

        Canceled += QueueFree;
    }

    public HiveProtection Options() => new()
    {
        DeleteEditorFiles = editorFiles.ButtonPressed ? 1 : 0,
        RemoveMetadata = metadata.ButtonPressed ? 1 : 0,
        ObfuscateScript = minify.ButtonPressed ? 1 : 0,
        JunkHeaderOffset = junkHeader.ButtonPressed ? 1 : 0,
        EncryptImports = encryptImports.ButtonPressed ? 1 : 0,
    };

    private static VBoxContainer Group(string title, string hint, out VBoxContainer body)
    {
        var box = new VBoxContainer();
        box.AddThemeConstantOverride("separation", 4);
        box.AddChild(new Label { Text = title });
        box.AddChild(new Label
        {
            Text = hint,
            Modulate = new Color(0.62f, 0.67f, 0.74f),
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
        });
        body = new VBoxContainer();
        body.AddThemeConstantOverride("separation", 2);
        box.AddChild(body);
        return box;
    }

    private static CheckBox Check(string text, string hint, VBoxContainer parent)
    {
        var box = new CheckBox { Text = text, TooltipText = hint };
        parent.AddChild(box);
        return box;
    }
}
