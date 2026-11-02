using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using Godot;

namespace HiveWeGodot;

[StructLayout(LayoutKind.Sequential)]
public struct HiveMapInfo
{
    public int Width;
    public int Height;
    public int TilesX;
    public int TilesY;
    public int TextureCount;
    public int DoodadCount;
    public int UnitCount;
    public int RegionCount;
    public int TilesetId;
}

[StructLayout(LayoutKind.Sequential)]
public struct HiveTile
{
    public uint Tex0;
    public uint Tex1;
    public uint Tex2;
    public uint Tex3;
    public uint Variation;

    public uint TextureIndex(int corner) => corner switch
    {
        0 => Tex0 & 0xFFFF,
        1 => Tex1 & 0xFFFF,
        2 => Tex2 & 0xFFFF,
        3 => Tex3 & 0xFFFF,
        _ => 0xFFFF,
    };
}

public static class HiveCore
{
    private const string LibraryName = "hivewe";
    private static IntPtr handle;

    static HiveCore()
    {
        NativeLibrary.SetDllImportResolver(typeof(HiveCore).Assembly, Resolve);
    }

    private static IntPtr Resolve(string name, Assembly assembly, DllImportSearchPath? searchPath)
    {
        if (name != LibraryName)
        {
            return IntPtr.Zero;
        }

        string path = Path.Combine(ProjectSettings.GlobalizePath("res://"), "lib", "libhivewe.dylib");
        return NativeLibrary.Load(path);
    }

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_open(
        [MarshalAs(UnmanagedType.LPUTF8Str)] string mapPath,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string dataDir,
        byte[] error,
        int errorSize);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_close(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_map_info(IntPtr hive, out HiveMapInfo info);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_heights(IntPtr hive, float[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_tiles(IntPtr hive, [Out] HiveTile[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_water_exists(IntPtr hive, byte[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_texture_id(IntPtr hive, int index);

    public static bool IsOpen => handle != IntPtr.Zero;

    public static bool Open(string mapPath, string dataDir, out string error)
    {
        if (handle != IntPtr.Zero)
        {
            Close();
        }

        byte[] buffer = new byte[512];
        handle = hive_open(mapPath, dataDir, buffer, buffer.Length);
        if (handle == IntPtr.Zero)
        {
            int end = Array.IndexOf(buffer, (byte)0);
            error = System.Text.Encoding.UTF8.GetString(buffer, 0, end < 0 ? buffer.Length : end);
            return false;
        }

        error = string.Empty;
        return true;
    }

    public static void Close()
    {
        if (handle != IntPtr.Zero)
        {
            hive_close(handle);
            handle = IntPtr.Zero;
        }
    }

    public static HiveMapInfo MapInfo
    {
        get
        {
            hive_map_info(handle, out HiveMapInfo info);
            return info;
        }
    }

    public static float[] Heights()
    {
        HiveMapInfo info = MapInfo;
        float[] data = new float[info.Width * info.Height];
        hive_copy_heights(handle, data, data.Length);
        return data;
    }

    public static HiveTile[] Tiles()
    {
        HiveMapInfo info = MapInfo;
        HiveTile[] data = new HiveTile[info.TilesX * info.TilesY];
        hive_copy_tiles(handle, data, data.Length);
        return data;
    }

    public static byte[] WaterExists()
    {
        HiveMapInfo info = MapInfo;
        byte[] data = new byte[info.Width * info.Height];
        hive_copy_water_exists(handle, data, data.Length);
        return data;
    }

    public static string? TextureId(int index)
    {
        IntPtr pointer = hive_texture_id(handle, index);
        return pointer == IntPtr.Zero ? null : Marshal.PtrToStringUTF8(pointer);
    }
}
