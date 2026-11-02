using System;
using System.Text;
using Godot;

namespace HiveWeGodot;

public partial class Main : Node
{
    private Label label = null!;

    public override void _Ready()
    {
        label = new Label
        {
            Position = new Vector2(24, 24),
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            Size = new Vector2(900, 600),
        };
        AddChild(label);

        string projectRoot = ProjectSettings.GlobalizePath("res://");
        string mapPath = projectRoot + "../testdata/maps/(2)BootyBay.w3m";
        string dataDir = projectRoot + "../testdata/warcraft";

        StringBuilder report = new();

        if (!HiveCore.Open(mapPath, dataDir, out string error))
        {
            report.AppendLine("打开失败：" + error);
            GD.PrintErr(error);
            label.Text = report.ToString();
            return;
        }

        HiveMapInfo info = HiveCore.MapInfo;
        report.AppendLine($"地图 {System.IO.Path.GetFileName(mapPath)}");
        report.AppendLine($"  {info.Width}×{info.Height} 顶点，{info.TilesX}×{info.TilesY} 格，tileset 0x{info.TilesetId:X2}");
        report.AppendLine($"  地面纹理 {info.TextureCount} 种，装饰物 {info.DoodadCount}，单位 {info.UnitCount}，区域 {info.RegionCount}");

        StringBuilder ids = new();
        for (int i = 0; i < info.TextureCount; i++)
        {
            ids.Append(HiveCore.TextureId(i)).Append(' ');
        }
        report.AppendLine($"  纹理：{ids}");

        float[] heights = HiveCore.Heights();
        float lowest = heights[0];
        float highest = heights[0];
        foreach (float height in heights)
        {
            lowest = Math.Min(lowest, height);
            highest = Math.Max(highest, height);
        }
        report.AppendLine($"  高度 {heights.Length} 个，范围 {lowest:F2} ~ {highest:F2}");

        HiveTile[] tiles = HiveCore.Tiles();
        report.AppendLine($"  格子 {tiles.Length} 个，[0] 纹理 {tiles[0].Tex0}/{tiles[0].Tex1}/{tiles[0].Tex2}/{tiles[0].Tex3} 变体 {tiles[0].Variation}");

        byte[] water = HiveCore.WaterExists();
        int waterCount = 0;
        foreach (byte flag in water)
        {
            waterCount += flag != 0 ? 1 : 0;
        }
        report.AppendLine($"  有水顶点 {waterCount} / {water.Length}");

        label.Text = report.ToString();
        GD.Print(report.ToString());

        HiveCore.Close();
    }

    public override void _ExitTree()
    {
        HiveCore.Close();
    }
}
