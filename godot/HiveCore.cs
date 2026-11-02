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
public struct HiveDoodad
{
    public float X;
    public float Y;
    public float Z;
    public float Angle;
    public float ScaleX;
    public float ScaleY;
    public float ScaleZ;
    public int Variation;
    public int Solid;
}

[StructLayout(LayoutKind.Sequential)]
public struct HiveRegion
{
    public float Left;
    public float Right;
    public float Bottom;
    public float Top;
    public byte Red;
    public byte Green;
    public byte Blue;
}

[StructLayout(LayoutKind.Sequential)]
public struct HiveUnit
{
    public float X;
    public float Y;
    public float Z;
    public float Angle;
    public float ScaleX;
    public float ScaleY;
    public float ScaleZ;
    public int Player;
    public int Variation;
}

[StructLayout(LayoutKind.Sequential)]
public struct HiveModelInfo
{
    public int GeosetCount;
    public int VertexCount;
    public int IndexCount;
    public float MinX;
    public float MinY;
    public float MinZ;
    public float MaxX;
    public float MaxY;
    public float MaxZ;
}

[StructLayout(LayoutKind.Sequential)]
public struct HiveGeosetInfo
{
    public int VertexOffset;
    public int VertexCount;
    public int IndexOffset;
    public int IndexCount;
    public int TwoSided;
    public int Unshaded;
    public float Alpha;
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
    private static extern int hive_copy_water_heights(IntPtr hive, float[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_regions(IntPtr hive, [Out] HiveRegion[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_region_name(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_pathing_info(IntPtr hive, out int width, out int height);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_pathing(IntPtr hive, byte[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_minimap_info(IntPtr hive, out int width, out int height);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_minimap(IntPtr hive, byte[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern float hive_water_offset(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_texture_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_texture_info(IntPtr hive, int index, out int width, out int layers, out int channels);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_texture(IntPtr hive, int index, byte[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_brush_move(IntPtr hive, float x, float y);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_brush_size(IntPtr hive, int size);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_brush_shape(IntPtr hive, int shape);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_texture_tool(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string tileId);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_height_tool(IntPtr hive, int deformation);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_cliff_tool(IntPtr hive, int operation, [MarshalAs(UnmanagedType.LPUTF8Str)] string? cliffId);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_cell_tool(IntPtr hive, int operation);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_brush_begin(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_brush_apply(IntPtr hive, double frameDelta);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_brush_end(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_save_map(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string mapPath, byte[] error, int errorSize);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_doodads(IntPtr hive, [Out] HiveDoodad[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_doodad_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_pick_doodad(IntPtr hive, float[] origin, float[] direction, float maxDistance);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_pick_unit(IntPtr hive, float[] origin, float[] direction, float maxDistance);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_query_area(IntPtr hive, float x0, float y0, float x1, float y1, int isUnit, [Out] int[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_remove_selection(IntPtr hive, int isUnit, int[] indices, int count);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_begin_group(IntPtr hive, int isUnit, int[] indices, int count, float x, float y);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_move_group_to(IntPtr hive, int isUnit, float x, float y);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_rotate_group_to(IntPtr hive, int isUnit, float x, float y);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_end_group(IntPtr hive, int isUnit);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_selection_box(IntPtr hive, int isUnit, int index, float[] output);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern float hive_object_angle(IntPtr hive, int isUnit, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_doodad_type_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_doodad_type_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_add_doodad(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string id, float x, float y, float z, float angle);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_selection(IntPtr hive, int[] indices, int count);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_clipboard_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_clipboard(IntPtr hive, [Out] HiveDoodad[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_clipboard_anchor(IntPtr hive, [Out] float[] output);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_place_clipboard(IntPtr hive, float x, float y);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_clipboard_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_from_clipboard(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_units(IntPtr hive, [Out] HiveUnit[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_unit_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_from_doodad(IntPtr hive, int doodadIndex);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_from_unit(IntPtr hive, int unitIndex);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_unit_type_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_unit_type_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_add_unit(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string id, float x, float y, float angle, int player);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_unit_selection(IntPtr hive, int[] indices, int count);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_unit_clipboard_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_unit_clipboard(IntPtr hive, [Out] HiveUnit[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_unit_clipboard_anchor(IntPtr hive, [Out] float[] output);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_place_unit_clipboard(IntPtr hive, float x, float y);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_unit_clipboard_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_from_unit_clipboard(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_info(IntPtr hive, int model, out HiveModelInfo info);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_geoset(IntPtr hive, int model, int geoset, out HiveGeosetInfo info);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_model_geoset_texture(IntPtr hive, int model, int geoset);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_model_positions(IntPtr hive, int model, float[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_model_normals(IntPtr hive, int model, float[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_model_uvs(IntPtr hive, int model, float[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_model_indices(IntPtr hive, int model, int geoset, uint[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_image_info_path(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string path, out int width, out int height);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_copy_image_path(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string path, byte[] output, int capacity);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_doodad_category(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string id);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_object_field_count(IntPtr hive, int category, [MarshalAs(UnmanagedType.LPUTF8Str)] string id);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_object_field_key(IntPtr hive, int category, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_object_field_label(IntPtr hive, int category, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_object_field_type(IntPtr hive, int category, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_object_field_description(IntPtr hive, int category, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_object_value(IntPtr hive, int category, [MarshalAs(UnmanagedType.LPUTF8Str)] string id, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_object_value(IntPtr hive, int category, [MarshalAs(UnmanagedType.LPUTF8Str)] string id, int index, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_object_value_modified(IntPtr hive, int category, [MarshalAs(UnmanagedType.LPUTF8Str)] string id, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_undo(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_redo(IntPtr hive);

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

    public static float[] WaterHeights()
    {
        HiveMapInfo info = MapInfo;
        float[] data = new float[info.Width * info.Height];
        hive_copy_water_heights(handle, data, data.Length);
        return data;
    }

    public static float WaterOffset() => hive_water_offset(handle);

    public static HiveRegion[] Regions()
    {
        HiveMapInfo info = MapInfo;
        if (info.RegionCount <= 0)
        {
            return Array.Empty<HiveRegion>();
        }
        HiveRegion[] data = new HiveRegion[info.RegionCount];
        hive_copy_regions(handle, data, data.Length);
        return data;
    }

    public static string? RegionName(int index)
    {
        IntPtr pointer = hive_region_name(handle, index);
        return pointer == IntPtr.Zero ? null : Marshal.PtrToStringUTF8(pointer);
    }

    public static byte[]? Pathing(out int width, out int height)
    {
        if (hive_pathing_info(handle, out width, out height) != 0 || width <= 0 || height <= 0)
        {
            return null;
        }
        byte[] data = new byte[width * height];
        hive_copy_pathing(handle, data, data.Length);
        return data;
    }

    public static Image? Minimap()
    {
        if (hive_minimap_info(handle, out int width, out int height) != 0 || width <= 0 || height <= 0)
        {
            return null;
        }
        byte[] data = new byte[width * height * 4];
        hive_copy_minimap(handle, data, data.Length);
        Image image = Image.CreateFromData(width, height, false, Image.Format.Rgba8, data);
        image.FlipY();
        return image;
    }

    public static void CopyWaterHeights(float[] output) => hive_copy_water_heights(handle, output, output.Length);

    public static void CopyWaterExists(byte[] output) => hive_copy_water_exists(handle, output, output.Length);

    public static bool TextureInfo(int index, out int tileSize, out int layers)
    {
        int result = hive_texture_info(handle, index, out tileSize, out layers, out _);
        return result == 0;
    }

    public static byte[] TexturePixels(int index, int tileSize, int layers)
    {
        byte[] data = new byte[tileSize * tileSize * 4 * layers];
        hive_copy_texture(handle, index, data, data.Length);
        return data;
    }

    public static string? TextureId(int index)
    {
        IntPtr pointer = hive_texture_id(handle, index);
        return pointer == IntPtr.Zero ? null : Marshal.PtrToStringUTF8(pointer);
    }

    public static void BrushMove(float x, float y) => hive_brush_move(handle, x, y);

    public static void BrushSize(int size) => hive_brush_size(handle, size);

    public static void BrushShape(int shape) => hive_brush_shape(handle, shape);

    public static bool SetTextureTool(string tileId) => hive_set_texture_tool(handle, tileId) == 0;

    public static bool SetHeightTool(int deformation) => hive_set_height_tool(handle, deformation) == 0;

    public static bool SetCliffTool(int operation, string? cliffId) => hive_set_cliff_tool(handle, operation, cliffId) == 0;

    public static bool SetCellTool(int operation) => hive_set_cell_tool(handle, operation) == 0;

    public static void BrushBegin() => hive_brush_begin(handle);

    public static void BrushApply(double frameDelta) => hive_brush_apply(handle, frameDelta);

    public static void BrushEnd() => hive_brush_end(handle);

    public static bool SaveMap(string mapPath, out string error)
    {
        byte[] buffer = new byte[512];
        if (hive_save_map(handle, mapPath, buffer, buffer.Length) == 0)
        {
            error = string.Empty;
            return true;
        }
        int end = Array.IndexOf(buffer, (byte)0);
        error = System.Text.Encoding.UTF8.GetString(buffer, 0, end < 0 ? buffer.Length : end);
        return false;
    }

    public static HiveDoodad[] Doodads()
    {
        HiveMapInfo info = MapInfo;
        if (info.DoodadCount <= 0)
        {
            return Array.Empty<HiveDoodad>();
        }
        HiveDoodad[] data = new HiveDoodad[info.DoodadCount];
        hive_copy_doodads(handle, data, data.Length);
        return data;
    }

    public static string? DoodadId(int index)
    {
        IntPtr pointer = hive_doodad_id(handle, index);
        return pointer == IntPtr.Zero ? null : Marshal.PtrToStringUTF8(pointer);
    }

    public static HiveUnit[] Units()
    {
        HiveMapInfo info = MapInfo;
        if (info.UnitCount <= 0)
        {
            return Array.Empty<HiveUnit>();
        }
        HiveUnit[] data = new HiveUnit[info.UnitCount];
        hive_copy_units(handle, data, data.Length);
        return data;
    }

    public static string? UnitId(int index)
    {
        IntPtr pointer = hive_unit_id(handle, index);
        return pointer == IntPtr.Zero ? null : Marshal.PtrToStringUTF8(pointer);
    }

    public static string[] DoodadTypes()
    {
        int count = hive_doodad_type_count(handle);
        string[] names = new string[count];
        for (int i = 0; i < count; i++)
        {
            IntPtr pointer = hive_doodad_type_id(handle, i);
            names[i] = pointer == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUTF8(pointer) ?? string.Empty;
        }
        return names;
    }

    public static int PickDoodad(Vector3 origin, Vector3 direction, float maxDistance)
        => hive_pick_doodad(handle, new[] { origin.X, origin.Y, origin.Z },
                            new[] { direction.X, direction.Y, direction.Z }, maxDistance);

    public static int PickUnit(Vector3 origin, Vector3 direction, float maxDistance)
        => hive_pick_unit(handle, new[] { origin.X, origin.Y, origin.Z },
                          new[] { direction.X, direction.Y, direction.Z }, maxDistance);

    public static int[] QueryArea(float x0, float y0, float x1, float y1, bool isUnit)
    {
        int capacity = isUnit ? MapInfo.UnitCount : MapInfo.DoodadCount;
        if (capacity <= 0)
        {
            return Array.Empty<int>();
        }
        int[] data = new int[capacity];
        int count = hive_query_area(handle, x0, y0, x1, y1, isUnit ? 1 : 0, data, capacity);
        if (count <= 0)
        {
            return Array.Empty<int>();
        }
        Array.Resize(ref data, count);
        return data;
    }

    public static int RemoveSelection(bool isUnit, int[] indices)
        => indices.Length == 0 ? 0 : hive_remove_selection(handle, isUnit ? 1 : 0, indices, indices.Length);

    public static bool BeginGroup(bool isUnit, int[] indices, float x, float y)
        => indices.Length != 0 && hive_begin_group(handle, isUnit ? 1 : 0, indices, indices.Length, x, y) == 0;

    public static bool MoveGroupTo(bool isUnit, float x, float y)
        => hive_move_group_to(handle, isUnit ? 1 : 0, x, y) == 0;

    public static bool RotateGroupTo(bool isUnit, float x, float y)
        => hive_rotate_group_to(handle, isUnit ? 1 : 0, x, y) == 0;

    public static bool EndGroup(bool isUnit) => hive_end_group(handle, isUnit ? 1 : 0) == 0;

    public static bool SelectionBox(bool isUnit, int index, out Vector3 minimum, out Vector3 maximum)
    {
        float[] data = new float[6];
        minimum = Vector3.Zero;
        maximum = Vector3.Zero;
        if (hive_selection_box(handle, isUnit ? 1 : 0, index, data) != 0)
        {
            return false;
        }
        minimum = new Vector3(data[0], data[1], data[2]);
        maximum = new Vector3(data[3], data[4], data[5]);
        return true;
    }

    public static float ObjectAngle(bool isUnit, int index) => hive_object_angle(handle, isUnit ? 1 : 0, index);

    public static int AddDoodad(string id, float x, float y, float z, float angle)
        => hive_add_doodad(handle, id, x, y, z, angle);

    public static int CopySelection(int[] indices) => hive_copy_selection(handle, indices, indices.Length);

    public static int ClipboardCount => hive_clipboard_count(handle);

    public static HiveDoodad[] Clipboard()
    {
        int count = ClipboardCount;
        if (count <= 0)
        {
            return Array.Empty<HiveDoodad>();
        }
        HiveDoodad[] data = new HiveDoodad[count];
        hive_copy_clipboard(handle, data, data.Length);
        return data;
    }

    public static Vector2 ClipboardAnchor()
    {
        float[] data = new float[2];
        hive_clipboard_anchor(handle, data);
        return new Vector2(data[0], data[1]);
    }

    public static int PlaceClipboard(float x, float y) => hive_place_clipboard(handle, x, y);

    public static string? ClipboardId(int index)
    {
        IntPtr pointer = hive_clipboard_id(handle, index);
        return pointer == IntPtr.Zero ? null : Marshal.PtrToStringUTF8(pointer);
    }

    public static int ModelFromClipboard(int index) => hive_model_from_clipboard(handle, index);

    public static int ModelFromDoodad(int doodadIndex) => hive_model_from_doodad(handle, doodadIndex);

    public static int ModelFromUnit(int unitIndex) => hive_model_from_unit(handle, unitIndex);

    public static string[] UnitTypes()
    {
        int count = hive_unit_type_count(handle);
        string[] names = new string[count];
        for (int i = 0; i < count; i++)
        {
            IntPtr pointer = hive_unit_type_id(handle, i);
            names[i] = pointer == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUTF8(pointer) ?? string.Empty;
        }
        return names;
    }

    public static int AddUnit(string id, float x, float y, float angle, int player)
        => hive_add_unit(handle, id, x, y, angle, player);

    public static int CopyUnitSelection(int[] indices) => hive_copy_unit_selection(handle, indices, indices.Length);

    public static int UnitClipboardCount => hive_unit_clipboard_count(handle);

    public static HiveUnit[] UnitClipboard()
    {
        int count = UnitClipboardCount;
        if (count <= 0)
        {
            return Array.Empty<HiveUnit>();
        }
        HiveUnit[] data = new HiveUnit[count];
        hive_copy_unit_clipboard(handle, data, data.Length);
        return data;
    }

    public static Vector2 UnitClipboardAnchor()
    {
        float[] data = new float[2];
        hive_unit_clipboard_anchor(handle, data);
        return new Vector2(data[0], data[1]);
    }

    public static int PlaceUnitClipboard(float x, float y) => hive_place_unit_clipboard(handle, x, y);

    public static string? UnitClipboardId(int index)
    {
        IntPtr pointer = hive_unit_clipboard_id(handle, index);
        return pointer == IntPtr.Zero ? null : Marshal.PtrToStringUTF8(pointer);
    }

    public static int ModelFromUnitClipboard(int index) => hive_model_from_unit_clipboard(handle, index);

    public static int DoodadCategory(string id) => hive_doodad_category(handle, id);

    public static int ObjectFieldCount(int category, string id) => hive_object_field_count(handle, category, id);

    public static string? ObjectFieldKey(int category, int index) => ReadString(hive_object_field_key(handle, category, index));

    public static string? ObjectFieldLabel(int category, int index) => ReadString(hive_object_field_label(handle, category, index));

    public static string? ObjectFieldType(int category, int index) => ReadString(hive_object_field_type(handle, category, index));

    public static string? ObjectFieldDescription(int category, int index)
        => ReadString(hive_object_field_description(handle, category, index));

    public static string? ObjectValue(int category, string id, int index)
        => ReadString(hive_object_value(handle, category, id, index));

    public static bool SetObjectValue(int category, string id, int index, string value)
        => hive_set_object_value(handle, category, id, index, value) == 0;

    public static bool ObjectValueModified(int category, string id, int index)
        => hive_object_value_modified(handle, category, id, index) != 0;

    private static string? ReadString(IntPtr pointer)
        => pointer == IntPtr.Zero ? null : Marshal.PtrToStringUTF8(pointer);

    public static HiveModelInfo ModelInfo(int model)
    {
        hive_model_info(handle, model, out HiveModelInfo info);
        return info;
    }

    public static HiveGeosetInfo ModelGeoset(int model, int geoset)
    {
        hive_model_geoset(handle, model, geoset, out HiveGeosetInfo info);
        return info;
    }

    public static string? ModelGeosetTexture(int model, int geoset)
    {
        IntPtr pointer = hive_model_geoset_texture(handle, model, geoset);
        return pointer == IntPtr.Zero ? null : Marshal.PtrToStringUTF8(pointer);
    }

    public static float[] ModelPositions(int model, int vertexCount)
    {
        float[] data = new float[vertexCount * 3];
        hive_copy_model_positions(handle, model, data, data.Length);
        return data;
    }

    public static float[] ModelNormals(int model, int vertexCount)
    {
        float[] data = new float[vertexCount * 3];
        hive_copy_model_normals(handle, model, data, data.Length);
        return data;
    }

    public static float[] ModelUvs(int model, int vertexCount)
    {
        float[] data = new float[vertexCount * 2];
        hive_copy_model_uvs(handle, model, data, data.Length);
        return data;
    }

    public static uint[] ModelIndices(int model, int geoset, int indexCount)
    {
        uint[] data = new uint[indexCount];
        hive_copy_model_indices(handle, model, geoset, data, data.Length);
        return data;
    }

    public static Image? ImageAt(string path)
    {
        if (hive_image_info_path(handle, path, out int width, out int height) != 0 || width <= 0 || height <= 0)
        {
            return null;
        }
        byte[] data = new byte[width * height * 4];
        hive_copy_image_path(handle, path, data, data.Length);
        return Image.CreateFromData(width, height, false, Image.Format.Rgba8, data);
    }

    public static void Undo() => hive_undo(handle);

    public static void Redo() => hive_redo(handle);

    public static void CopyHeights(float[] output) => hive_copy_heights(handle, output, output.Length);

    public static void CopyTiles(HiveTile[] output) => hive_copy_tiles(handle, output, output.Length);
}
