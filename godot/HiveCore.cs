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
    public byte Alpha;
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
public struct HiveMaterialInfo
{
    public int PriorityPlane;
    public uint Flags;
    public int LayerCount;
}

[StructLayout(LayoutKind.Sequential)]
public struct HiveLayerInfo
{
    public uint BlendMode;
    public uint Shader;
    public uint ShadingFlags;
    public uint CoordId;
    public int TextureAnimationId;
    public int TextureCount;
    public float Alpha;
    public float EmissiveGain;
}

[StructLayout(LayoutKind.Sequential)]
public struct HiveTextureInfo
{
    public uint ReplaceableId;
    public uint Flags;
    public int Width;
    public int Height;
}

[StructLayout(LayoutKind.Sequential)]
public struct HiveNodeInfo
{
    public int Id;
    public int ParentId;
    public int Flags;
    public int TranslationKeys;
    public int RotationKeys;
    public int ScalingKeys;
    public int Interpolation;
    public int GlobalSequence;
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
    public int TeamColor;
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
    private static extern IntPtr hive_new_map([MarshalAs(UnmanagedType.LPUTF8Str)] string dataDir, byte[] error,
        int errorSize);

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
    private static extern int hive_weather_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_weather_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_weather_name(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_sound_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_sound_name(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_sound_variable(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_ambient_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_ambient_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_ambient_name(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_region_weather(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_region_ambient(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_region_weather(IntPtr hive, int index,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_region_ambient(IntPtr hive, int index,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_region_color(IntPtr hive, int index, int red, int green, int blue, int alpha);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_create_region(IntPtr hive, float left, float bottom, float right, float top);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_delete_region(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_region_rect(IntPtr hive, int index, float left, float bottom, float right, float top);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_region_name(IntPtr hive, int index, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

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
    private static extern int hive_cliffset_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_cliffset_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_terrain_brush_flags(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_set_terrain_brush_flags(IntPtr hive, int mask);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_pathing_tool(IntPtr hive, int mask, int operation);

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
    private static extern void hive_set_doodad_random(IntPtr hive, int flags);

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
    private static extern int hive_player_color(int player, float[] rgb);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_from_path(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string path);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_material_count(IntPtr hive, int model);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_texture_count(IntPtr hive, int model);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_node_count(IntPtr hive, int model);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_sequence_count(IntPtr hive, int model);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_model_sequence_name(IntPtr hive, int model, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_sequence_range(IntPtr hive, int model, int index, out int start, out int end);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_apply_pose(IntPtr hive, int model, int sequence, int frame);

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
    private static extern int hive_model_material(IntPtr hive, int model, int material, out HiveMaterialInfo info);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_layer(IntPtr hive, int model, int material, int layer, out HiveLayerInfo info);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_layer_texture(IntPtr hive, int model, int material, int layer, int index,
        out int slot, out int texture);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_model_texture_name(IntPtr hive, int model, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_texture(IntPtr hive, int model, int index, out HiveTextureInfo info);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_model_node_name(IntPtr hive, int model, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_model_node(IntPtr hive, int model, int index, out HiveNodeInfo info);

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
    private static extern int hive_object_type_count(IntPtr hive, int category);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_object_type_id(IntPtr hive, int category, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_object_type_name(IntPtr hive, int category, [MarshalAs(UnmanagedType.LPUTF8Str)] string id);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_object_type_folder(IntPtr hive, int category, [MarshalAs(UnmanagedType.LPUTF8Str)] string id, int level);

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
    private static extern int hive_create_object(IntPtr hive, int category, [MarshalAs(UnmanagedType.LPUTF8Str)] string sourceId, [MarshalAs(UnmanagedType.LPUTF8Str)] string newId);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_delete_object(IntPtr hive, int category, [MarshalAs(UnmanagedType.LPUTF8Str)] string id);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_object_is_custom(IntPtr hive, int category, [MarshalAs(UnmanagedType.LPUTF8Str)] string id);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_unique_object_id(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string sourceId);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_map_name(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_name(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_map_author(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_author(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_map_description(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_description(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_map_suggested_players(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_suggested_players(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_map_versions(IntPtr hive, [Out] int[] output);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_map_playable_size(IntPtr hive, out int width, out int height);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_gameplay_constant_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_gameplay_constant_key(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_gameplay_constant_label(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_gameplay_constant_value(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_gameplay_constant(IntPtr hive, int index,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_gameplay_constant_modified(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_loading_screen_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_loading_screen_name(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_map_loading_screen(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_map_loading_screen_model(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_map_loading_screen_title(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_map_loading_screen_subtitle(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_map_loading_screen_text(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_loading_screen(IntPtr hive, int number);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_loading_screen_model(IntPtr hive,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_loading_screen_title(IntPtr hive,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_loading_screen_subtitle(IntPtr hive,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_loading_screen_text(IntPtr hive,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_imported_model_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_imported_model(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_map_option_flags(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_set_map_option_flags(IntPtr hive, int mask);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_map_fog(IntPtr hive, out int style, out float start, out float end,
        out float density, out int red, out int green, out int blue);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_fog(IntPtr hive, int style, float start, float end, float density,
        int red, int green, int blue);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_map_weather_id(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_weather_id(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_map_sound_environment(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_sound_environment(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_map_custom_light_tileset(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_custom_light_tileset(IntPtr hive, int key);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_map_custom_ambience_tileset(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_custom_ambience_tileset(IntPtr hive, int key);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_map_water_color(IntPtr hive, out int red, out int green, out int blue, out int alpha);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_water_color(IntPtr hive, int red, int green, int blue, int alpha);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_map_game_data_set(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_game_data_set(IntPtr hive, int value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_tileset_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_tileset_key(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_tileset_name(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void hive_map_playable_insets(IntPtr hive, out int left, out int right, out int top, out int bottom);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_player_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_player_slot_type(IntPtr hive, int slot);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_player_slot_race(IntPtr hive, int slot);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_player_slot_fixed(IntPtr hive, int slot);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_player_slot_name(IntPtr hive, int slot);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_player_slot(IntPtr hive, int slot, int type, int race,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string name, int fixedStart);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_start_location_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_start_location(IntPtr hive, int index, out int player, out float x, out float y);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_start_location(IntPtr hive, int player, float x, float y);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_remove_start_location(IntPtr hive, int player);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_resize_map(IntPtr hive, int deltaLeft, int deltaRight, int deltaTop, int deltaBottom);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_playable_area(IntPtr hive, int left, int right, int top, int bottom);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_category_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_category_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_category_name(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_category_parent(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_id(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_name(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_description(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_parent(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_is_comment(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_is_enabled(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_initially_on(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_is_script(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_custom_text(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_trigger_custom_text(IntPtr hive, int index, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_build_nodes(IntPtr hive, int trigger);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_node_depth(IntPtr hive, int trigger, int node);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_node_type(IntPtr hive, int trigger, int node);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_node_group(IntPtr hive, int trigger, int node);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_node_group_kind(IntPtr hive, int trigger, int node);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_node_name(IntPtr hive, int trigger, int node);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_node_display(IntPtr hive, int trigger, int node);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_node_parameter_count(IntPtr hive, int trigger, int node);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_node_parameter_type(IntPtr hive, int trigger, int node, int parameter);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_node_parameter_value(IntPtr hive, int trigger, int node, int parameter);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_node_parameter_type_name(IntPtr hive, int trigger, int node, int parameter);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_trigger_node_parameter(IntPtr hive, int trigger, int node, int parameter, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_add_node(IntPtr hive, int trigger, int type, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, int parent, int group);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_remove_node(IntPtr hive, int trigger, int node);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_preset_count(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string typeName);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_preset_code(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string typeName, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_preset_display(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string typeName, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_function_count(IntPtr hive, int type);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_function_name(IntPtr hive, int type, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_create_trigger(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string name, int parentId);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_delete_trigger(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_trigger_name(IntPtr hive, int index, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_trigger_description(IntPtr hive, int index, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_trigger_enabled(IntPtr hive, int index, int enabled);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_trigger_initially_on(IntPtr hive, int index, int on);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_variable_count(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_variable_name(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_variable_type(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_trigger_variable_is_array(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_trigger_variable_initial(IntPtr hive, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr hive_map_global_script(IntPtr hive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int hive_set_map_global_script(IntPtr hive, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);


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

    public static bool NewMap(string dataDir, out string error)
    {
        if (handle != IntPtr.Zero)
        {
            Close();
        }

        byte[] buffer = new byte[512];
        handle = hive_new_map(dataDir, buffer, buffer.Length);
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

    public static int CreateRegion(float left, float bottom, float right, float top)
        => hive_create_region(handle, left, bottom, right, top);

    public static bool DeleteRegion(int index) => hive_delete_region(handle, index) == 0;

    public static void SetRegionRect(int index, float left, float bottom, float right, float top)
        => hive_set_region_rect(handle, index, left, bottom, right, top);

    public static void SetRegionName(int index, string value) => hive_set_region_name(handle, index, value);

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

    public static int CliffsetCount() => hive_cliffset_count(handle);

    public static string? CliffsetId(int index) => ReadString(hive_cliffset_id(handle, index));

    public static int TerrainBrushFlags() => hive_terrain_brush_flags(handle);

    public static void SetTerrainBrushFlags(int mask) => hive_set_terrain_brush_flags(handle, mask);

    public static int SetPathingTool(int mask, int operation) => hive_set_pathing_tool(handle, mask, operation);

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

    public static void SetDoodadRandom(int flags) => hive_set_doodad_random(handle, flags);

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

    public static int ModelFromPath(string path) => hive_model_from_path(handle, path);

    public static int ModelMaterialCount(int model) => hive_model_material_count(handle, model);

    public static int ModelTextureCount(int model) => hive_model_texture_count(handle, model);

    public static int ModelNodeCount(int model) => hive_model_node_count(handle, model);

    public static int ModelSequenceCount(int model) => hive_model_sequence_count(handle, model);

    public static string? ModelSequenceName(int model, int index)
        => ReadString(hive_model_sequence_name(handle, model, index));

    public static (int Start, int End) ModelSequenceRange(int model, int index)
    {
        hive_model_sequence_range(handle, model, index, out int start, out int end);
        return (start, end);
    }

    public static bool ModelPose(int model, int sequence, int frame)
        => hive_model_apply_pose(handle, model, sequence, frame) == 0;

    public static Vector3 PlayerColor(int player)
    {
        float[] rgb = new float[3];
        if (hive_player_color(player, rgb) != 0)
        {
            return Vector3.One;
        }
        return new Vector3(rgb[0], rgb[1], rgb[2]);
    }

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

    public static int ObjectTypeCount(int category) => hive_object_type_count(handle, category);

    public static string? ObjectTypeId(int category, int index) => ReadString(hive_object_type_id(handle, category, index));

    public static string? ObjectTypeName(int category, string id)
        => ReadString(hive_object_type_name(handle, category, id));

    public static string? ObjectTypeFolder(int category, string id, int level)
        => ReadString(hive_object_type_folder(handle, category, id, level));

    public static string[] ObjectTypeFolders(int category, string id)
    {
        var parts = new System.Collections.Generic.List<string>();
        for (int level = 0; level < 4; level++)
        {
            string? part = ObjectTypeFolder(category, id, level);
            if (string.IsNullOrEmpty(part))
            {
                break;
            }
            parts.Add(part);
        }
        return parts.Count == 0 ? new[] { "其它" } : parts.ToArray();
    }

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

    public static bool CreateObject(int category, string sourceId, string newId)
        => hive_create_object(handle, category, sourceId, newId) == 0;

    public static bool DeleteObject(int category, string id) => hive_delete_object(handle, category, id) == 0;

    public static bool IsCustomObject(int category, string id) => hive_object_is_custom(handle, category, id) != 0;

    public static string? UniqueObjectId(string sourceId) => ReadString(hive_unique_object_id(handle, sourceId));

    public static string? MapName => ReadString(hive_map_name(handle));

    public static bool SetMapName(string value) => hive_set_map_name(handle, value) == 0;

    public static string? MapAuthor => ReadString(hive_map_author(handle));

    public static bool SetMapAuthor(string value) => hive_set_map_author(handle, value) == 0;

    public static string? MapDescription => ReadString(hive_map_description(handle));

    public static bool SetMapDescription(string value) => hive_set_map_description(handle, value) == 0;

    public static string? MapSuggestedPlayers => ReadString(hive_map_suggested_players(handle));

    public static bool SetMapSuggestedPlayers(string value) => hive_set_map_suggested_players(handle, value) == 0;

    public static int[] MapVersions()
    {
        int[] data = new int[6];
        hive_map_versions(handle, data);
        return data;
    }

    public static (int Width, int Height) MapPlayableSize()
    {
        hive_map_playable_size(handle, out int width, out int height);
        return (width, height);
    }

    public static int GameplayConstantCount() => hive_gameplay_constant_count(handle);

    public static string? GameplayConstantKey(int index) => ReadString(hive_gameplay_constant_key(handle, index));

    public static string? GameplayConstantLabel(int index) => ReadString(hive_gameplay_constant_label(handle, index));

    public static string? GameplayConstantValue(int index) => ReadString(hive_gameplay_constant_value(handle, index));

    public static bool SetGameplayConstant(int index, string value)
        => hive_set_gameplay_constant(handle, index, value) == 0;

    public static int GameplayConstantModified(int index) => hive_gameplay_constant_modified(handle, index);

    public static int LoadingScreenCount() => hive_loading_screen_count(handle);

    public static string? LoadingScreenName(int index) => ReadString(hive_loading_screen_name(handle, index));

    public static int MapLoadingScreen() => hive_map_loading_screen(handle);

    public static string? MapLoadingScreenModel() => ReadString(hive_map_loading_screen_model(handle));

    public static string? MapLoadingScreenTitle() => ReadString(hive_map_loading_screen_title(handle));

    public static string? MapLoadingScreenSubtitle() => ReadString(hive_map_loading_screen_subtitle(handle));

    public static string? MapLoadingScreenText() => ReadString(hive_map_loading_screen_text(handle));

    public static bool SetMapLoadingScreen(int number) => hive_set_map_loading_screen(handle, number) == 0;

    public static bool SetMapLoadingScreenModel(string value) => hive_set_map_loading_screen_model(handle, value) == 0;

    public static bool SetMapLoadingScreenTitle(string value) => hive_set_map_loading_screen_title(handle, value) == 0;

    public static bool SetMapLoadingScreenSubtitle(string value)
        => hive_set_map_loading_screen_subtitle(handle, value) == 0;

    public static bool SetMapLoadingScreenText(string value) => hive_set_map_loading_screen_text(handle, value) == 0;

    public static int ImportedModelCount() => hive_imported_model_count(handle);

    public static string? ImportedModel(int index) => ReadString(hive_imported_model(handle, index));

    public static int MapOptionFlags() => hive_map_option_flags(handle);

    public static void SetMapOptionFlags(int mask) => hive_set_map_option_flags(handle, mask);

    public static (int Style, float Start, float End, float Density, int Red, int Green, int Blue) MapFog()
    {
        hive_map_fog(handle, out int style, out float start, out float end, out float density, out int red,
            out int green, out int blue);
        return (style, start, end, density, red, green, blue);
    }

    public static void SetMapFog(int style, float start, float end, float density, int red, int green, int blue)
        => hive_set_map_fog(handle, style, start, end, density, red, green, blue);

    public static string? MapWeatherId() => ReadString(hive_map_weather_id(handle));

    public static bool SetMapWeatherId(string value) => hive_set_map_weather_id(handle, value) == 0;

    public static string? MapSoundEnvironment() => ReadString(hive_map_sound_environment(handle));

    public static bool SetMapSoundEnvironment(string value) => hive_set_map_sound_environment(handle, value) == 0;

    public static int MapCustomLightTileset() => hive_map_custom_light_tileset(handle);

    public static bool SetMapCustomLightTileset(int key) => hive_set_map_custom_light_tileset(handle, key) == 0;

    public static int MapCustomAmbienceTileset() => hive_map_custom_ambience_tileset(handle);

    public static bool SetMapCustomAmbienceTileset(int key) => hive_set_map_custom_ambience_tileset(handle, key) == 0;

    public static (int Red, int Green, int Blue, int Alpha) MapWaterColor()
    {
        hive_map_water_color(handle, out int red, out int green, out int blue, out int alpha);
        return (red, green, blue, alpha);
    }

    public static void SetMapWaterColor(int red, int green, int blue, int alpha)
        => hive_set_map_water_color(handle, red, green, blue, alpha);

    public static int MapGameDataSet() => hive_map_game_data_set(handle);

    public static void SetMapGameDataSet(int value) => hive_set_map_game_data_set(handle, value);

    public static int TilesetCount() => hive_tileset_count(handle);

    public static string? TilesetKey(int index) => ReadString(hive_tileset_key(handle, index));

    public static string? TilesetName(int index) => ReadString(hive_tileset_name(handle, index));

    public static int PlayerCount() => hive_player_count(handle);

    public static int PlayerSlotType(int slot) => hive_player_slot_type(handle, slot);

    public static int PlayerSlotRace(int slot) => hive_player_slot_race(handle, slot);

    public static int PlayerSlotFixed(int slot) => hive_player_slot_fixed(handle, slot);

    public static string? PlayerSlotName(int slot) => ReadString(hive_player_slot_name(handle, slot));

    public static bool SetPlayerSlot(int slot, int type, int race, string name, int fixedStart)
        => hive_set_player_slot(handle, slot, type, race, name, fixedStart) == 0;

    public static int WeatherCount() => hive_weather_count(handle);

    public static string? WeatherId(int index) => ReadString(hive_weather_id(handle, index));

    public static string? WeatherName(int index) => ReadString(hive_weather_name(handle, index));

    public static int SoundCount() => hive_sound_count(handle);

    public static string? SoundName(int index) => ReadString(hive_sound_name(handle, index));

    public static string? SoundVariable(int index) => ReadString(hive_sound_variable(handle, index));

    public static int AmbientCount() => hive_ambient_count(handle);

    public static string? AmbientId(int index) => ReadString(hive_ambient_id(handle, index));

    public static string? AmbientName(int index) => ReadString(hive_ambient_name(handle, index));

    public static string? RegionWeather(int index) => ReadString(hive_region_weather(handle, index));

    public static string? RegionAmbient(int index) => ReadString(hive_region_ambient(handle, index));

    public static bool SetRegionWeather(int index, string value) => hive_set_region_weather(handle, index, value) == 0;

    public static bool SetRegionAmbient(int index, string value) => hive_set_region_ambient(handle, index, value) == 0;

    public static bool SetRegionColor(int index, Color color)
        => hive_set_region_color(handle, index, (int)(color.R * 255f), (int)(color.G * 255f), (int)(color.B * 255f),
            (int)(color.A * 255f)) == 0;

    public static int StartLocationCount() => hive_start_location_count(handle);

    public static bool StartLocation(int index, out int player, out float x, out float y)
        => hive_start_location(handle, index, out player, out x, out y) == 0;

    public static bool SetStartLocation(int player, float x, float y)
        => hive_set_start_location(handle, player, x, y) == 0;

    public static bool RemoveStartLocation(int player) => hive_remove_start_location(handle, player) > 0;

    public static (int Left, int Right, int Top, int Bottom) MapPlayableInsets()
    {
        hive_map_playable_insets(handle, out int left, out int right, out int top, out int bottom);
        return (left, right, top, bottom);
    }

    public static int ResizeMap(int deltaLeft, int deltaRight, int deltaTop, int deltaBottom)
        => hive_resize_map(handle, deltaLeft, deltaRight, deltaTop, deltaBottom);

    public static int SetPlayableArea(int left, int right, int top, int bottom)
        => hive_set_playable_area(handle, left, right, top, bottom);

    public static int TriggerCategoryCount => hive_trigger_category_count(handle);

    public static (int Id, string Name, int Parent) TriggerCategory(int index)
        => (hive_trigger_category_id(handle, index),
            ReadString(hive_trigger_category_name(handle, index)) ?? string.Empty,
            hive_trigger_category_parent(handle, index));

    public static int TriggerCount => hive_trigger_count(handle);

    public static (int Id, string Name, int Parent) Trigger(int index)
        => (hive_trigger_id(handle, index),
            ReadString(hive_trigger_name(handle, index)) ?? string.Empty,
            hive_trigger_parent(handle, index));

    public static string? TriggerDescription(int index) => ReadString(hive_trigger_description(handle, index));

    public static bool TriggerIsComment(int index) => hive_trigger_is_comment(handle, index) != 0;

    public static bool TriggerIsEnabled(int index) => hive_trigger_is_enabled(handle, index) != 0;

    public static bool TriggerInitiallyOn(int index) => hive_trigger_initially_on(handle, index) != 0;

    public static bool TriggerIsScript(int index) => hive_trigger_is_script(handle, index) != 0;

    public static string? TriggerCustomText(int index) => ReadString(hive_trigger_custom_text(handle, index));

    public static void SetTriggerCustomText(int index, string value) => hive_set_trigger_custom_text(handle, index, value);

    public static int TriggerBuildNodes(int index) => hive_trigger_build_nodes(handle, index);

    public static int TriggerNodeType(int trigger, int node) => hive_trigger_node_type(handle, trigger, node);

    public static int TriggerNodeDepth(int trigger, int node) => hive_trigger_node_depth(handle, trigger, node);

    public static int TriggerNodeGroup(int trigger, int node) => hive_trigger_node_group(handle, trigger, node);

    public static int TriggerNodeGroupKind(int trigger, int node) => hive_trigger_node_group_kind(handle, trigger, node);

    public static string? TriggerNodeName(int trigger, int node) => ReadString(hive_trigger_node_name(handle, trigger, node));

    public static string? TriggerNodeDisplay(int trigger, int node)
        => ReadString(hive_trigger_node_display(handle, trigger, node));

    public static int TriggerNodeParameterCount(int trigger, int node)
        => hive_trigger_node_parameter_count(handle, trigger, node);

    public static int TriggerNodeParameterType(int trigger, int node, int parameter)
        => hive_trigger_node_parameter_type(handle, trigger, node, parameter);

    public static string? TriggerNodeParameterValue(int trigger, int node, int parameter)
        => ReadString(hive_trigger_node_parameter_value(handle, trigger, node, parameter));

    public static string? TriggerNodeParameterTypeName(int trigger, int node, int parameter)
        => ReadString(hive_trigger_node_parameter_type_name(handle, trigger, node, parameter));

    public static bool SetTriggerNodeParameter(int trigger, int node, int parameter, string value)
        => hive_set_trigger_node_parameter(handle, trigger, node, parameter, value) == 0;

    public static bool AddTriggerNode(int trigger, int type, string name, int parent, int group)
        => hive_trigger_add_node(handle, trigger, type, name, parent, group) == 0;

    public static bool RemoveTriggerNode(int trigger, int node) => hive_trigger_remove_node(handle, trigger, node) == 0;

    public static int TriggerPresetCount(string typeName) => hive_trigger_preset_count(handle, typeName);

    public static string? TriggerPresetCode(string typeName, int index)
        => ReadString(hive_trigger_preset_code(handle, typeName, index));

    public static string? TriggerPresetDisplay(string typeName, int index)
        => ReadString(hive_trigger_preset_display(handle, typeName, index));

    public static int TriggerFunctionCount(int type) => hive_trigger_function_count(handle, type);

    public static string? TriggerFunctionName(int type, int index)
        => ReadString(hive_trigger_function_name(handle, type, index));

    public static int CreateTrigger(string name, int parentId) => hive_create_trigger(handle, name, parentId);

    public static bool DeleteTrigger(int index) => hive_delete_trigger(handle, index) == 0;

    public static bool SetTriggerName(int index, string value) => hive_set_trigger_name(handle, index, value) == 0;

    public static bool SetTriggerDescription(int index, string value)
        => hive_set_trigger_description(handle, index, value) == 0;

    public static void SetTriggerEnabled(int index, bool enabled) => hive_set_trigger_enabled(handle, index, enabled ? 1 : 0);

    public static void SetTriggerInitiallyOn(int index, bool on) => hive_set_trigger_initially_on(handle, index, on ? 1 : 0);

    public static int TriggerVariableCount => hive_trigger_variable_count(handle);

    public static (string Name, string Type, bool IsArray, string Initial) TriggerVariable(int index)
        => (ReadString(hive_trigger_variable_name(handle, index)) ?? string.Empty,
            ReadString(hive_trigger_variable_type(handle, index)) ?? string.Empty,
            hive_trigger_variable_is_array(handle, index) != 0,
            ReadString(hive_trigger_variable_initial(handle, index)) ?? string.Empty);

    public static string? MapGlobalScript => ReadString(hive_map_global_script(handle));

    public static bool SetMapGlobalScript(string value) => hive_set_map_global_script(handle, value) == 0;

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

    public static HiveMaterialInfo? ModelMaterial(int model, int material)
    {
        if (hive_model_material(handle, model, material, out HiveMaterialInfo info) != 0)
        {
            return null;
        }
        return info;
    }

    public static HiveLayerInfo? ModelLayer(int model, int material, int layer)
    {
        if (hive_model_layer(handle, model, material, layer, out HiveLayerInfo info) != 0)
        {
            return null;
        }
        return info;
    }

    public static (int Slot, int Texture) ModelLayerTexture(int model, int material, int layer, int index)
    {
        if (hive_model_layer_texture(handle, model, material, layer, index, out int slot, out int texture) != 0)
        {
            return (-1, -1);
        }
        return (slot, texture);
    }

    public static string? ModelTextureName(int model, int index)
        => ReadString(hive_model_texture_name(handle, model, index));

    public static HiveTextureInfo ModelTexture(int model, int index)
    {
        hive_model_texture(handle, model, index, out HiveTextureInfo info);
        return info;
    }

    public static string? ModelNodeName(int model, int index)
        => ReadString(hive_model_node_name(handle, model, index));

    public static HiveNodeInfo? ModelNode(int model, int index)
    {
        if (hive_model_node(handle, model, index, out HiveNodeInfo info) != 0)
        {
            return null;
        }
        return info;
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
