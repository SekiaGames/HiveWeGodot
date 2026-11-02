#ifndef HIVE_C_H
#define HIVE_C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HiveMapInfo {
	int32_t width;
	int32_t height;
	int32_t tiles_x;
	int32_t tiles_y;
	int32_t texture_count;
	int32_t doodad_count;
	int32_t unit_count;
	int32_t region_count;
	int32_t tileset_id;
} HiveMapInfo;

/* 原样对应 HiveWE 的 uvec4：每项低 16 位是纹理索引，高 16 位是额外信息。
   第 0 项的高 16 位是变体号，其余项的高 16 位是该纹理出现在哪几个角的掩码，
   0xFFFF 表示这一项为空。 */
typedef struct HiveTile {
	uint32_t texture[4];
	uint32_t variation;
} HiveTile;

/* 打开地图。data_dir 指向装了客户端的目录（含 WAR3.MPQ），或从 MPQ 里解包出来的资源目录。
   失败返回 NULL 并把原因写进 error。 */
void* hive_open(const char* map_path, const char* data_dir, char* error, int32_t error_size);

void hive_close(void* handle);

/* 把当前地图写回 target_dir。成功返回 0。 */
int32_t hive_save(void* handle, const char* target_dir, char* error, int32_t error_size);

/* 把当前地图打成 w3m / w3x 写到 map_path。成功返回 0。 */
int32_t hive_save_map(void* handle, const char* map_path, char* error, int32_t error_size);

int32_t hive_map_info(void* handle, HiveMapInfo* out);

/* 拷贝地形数据，返回实际元素数。capacity 不够时只写 capacity 个。 */
int32_t hive_copy_heights(void* handle, float* out, int32_t capacity);
int32_t hive_copy_tiles(void* handle, HiveTile* out, int32_t capacity);
int32_t hive_copy_ground_exists(void* handle, uint8_t* out, int32_t capacity);
int32_t hive_copy_water_exists(void* handle, uint8_t* out, int32_t capacity);
/* 每个顶点的水面高度，不含全局水偏移。 */
int32_t hive_copy_water_heights(void* handle, float* out, int32_t capacity);
/* 实际水面高度 = 顶点水面高度 + 这个偏移。 */
float hive_water_offset(void* handle);

/* 第 index 个地面纹理的 id（四字符，如 Adrt），越界返回 NULL。index 等于纹理数时返回 "blight"。 */
const char* hive_texture_id(void* handle, int32_t index);

/* 地面纹理已经被切成 tile 网格：width = height = 单块边长，channels = 层数（16 或 32），
   每层是 width×width 的 RGBA8。变体号就是层号。成功返回 0。 */
int32_t hive_texture_info(void* handle, int32_t index, int32_t* width, int32_t* height, int32_t* channels);
int32_t hive_copy_texture(void* handle, int32_t index, uint8_t* out, int32_t capacity);

/* 小地图，尺寸同顶点网格，RGBA8，自上而下。 */
int32_t hive_minimap_info(void* handle, int32_t* width, int32_t* height);
int32_t hive_copy_minimap(void* handle, uint8_t* out, int32_t capacity);

/* 地图文件所在目录，即解包后的临时目录。 */
const char* hive_map_directory(void* handle);

/* 笔刷位置，地形 tile 坐标，笔刷中心落在 (x, y)。 */
void hive_brush_move(void* handle, float x, float y);

/* 笔刷边长，单位是 tile，取值 1 到 249。 */
void hive_brush_size(void* handle, int32_t size);

/* 笔刷形状：0 方形，1 圆形，2 菱形。 */
void hive_brush_shape(void* handle, int32_t shape);

/* 选择工具。每个工具单独设置，后设的会关掉与它不兼容的旧工具。
   返回 0 成功，负数是错误。 */
int32_t hive_set_texture_tool(void* handle, const char* tile_id);
/* deformation：0 抬升，1 降低，2 平台，3 起伏，4 平滑。 */
int32_t hive_set_height_tool(void* handle, int32_t deformation);
/* operation：0 降两级，1 降一级，2 抹平，3 升一级，4 升两级，5 深水，6 浅水，7 斜坡。 */
int32_t hive_set_cliff_tool(void* handle, int32_t operation, const char* cliff_id);
/* operation：0 加水，1 去水，2 加边界，3 去边界。 */
int32_t hive_set_cell_tool(void* handle, int32_t operation);

/* 一次绘制。begin 会快照整张地形用于撤销，apply 可以按帧连调多次，end 收尾并入撤销栈。 */
void hive_brush_begin(void* handle);
void hive_brush_apply(void* handle, double frame_delta);
void hive_brush_end(void* handle);

/* 撤销 / 重做一步，返回 1 表示动了，0 表示没有可撤销的。 */
int32_t hive_undo(void* handle);
int32_t hive_redo(void* handle);

/* 装饰物。position 是地形坐标系，y 是高度，单位与地形一致；angle 是弧度。 */
typedef struct HiveDoodad {
	float x;
	float y;
	float z;
	float angle;
	float scale_x;
	float scale_y;
	float scale_z;
	int32_t variation;
	int32_t solid;
} HiveDoodad;

int32_t hive_copy_doodads(void* handle, HiveDoodad* out, int32_t capacity);
/* 第 index 个装饰物的四字符 id，用于查模型。越界返回 NULL。 */
const char* hive_doodad_id(void* handle, int32_t index);

/* 单位。坐标单位同装饰物：位置是 tile，angle 是弧度。player 是 0..15 的玩家位。 */
typedef struct HiveUnit {
	float x;
	float y;
	float z;
	float angle;
	float scale_x;
	float scale_y;
	float scale_z;
	int32_t player;
	int32_t variation;
} HiveUnit;

int32_t hive_copy_units(void* handle, HiveUnit* out, int32_t capacity);
const char* hive_unit_id(void* handle, int32_t index);

/* 模型网格。顶点、法线、UV 按 geoset 顺序拼成一个数组，索引各自独立。
   MDX 原始坐标系是 Z 向上，调用方自己转。 */
typedef struct HiveModelInfo {
	int32_t geoset_count;
	int32_t vertex_count;
	int32_t index_count;
	float min_x;
	float min_y;
	float min_z;
	float max_x;
	float max_y;
	float max_z;
} HiveModelInfo;

typedef struct HiveGeosetInfo {
	int32_t vertex_offset;
	int32_t vertex_count;
	int32_t index_offset;
	int32_t index_count;
	int32_t two_sided;
	int32_t unshaded;
	float alpha;
} HiveGeosetInfo;

/* 打开模型，返回 >= 0 的句柄，失败返回 -1。句柄在 hive_close 前一直有效。 */
int32_t hive_model_from_doodad(void* handle, int32_t doodad_index);
int32_t hive_model_from_unit(void* handle, int32_t unit_index);
int32_t hive_model_from_path(void* handle, const char* path);
int32_t hive_model_info(void* handle, int32_t model, HiveModelInfo* out);
int32_t hive_model_geoset(void* handle, int32_t model, int32_t geoset, HiveGeosetInfo* out);
/* 该 geoset 用的 BLP 路径，没有贴图返回 NULL。 */
const char* hive_model_geoset_texture(void* handle, int32_t model, int32_t geoset);

int32_t hive_copy_model_positions(void* handle, int32_t model, float* out, int32_t capacity);
int32_t hive_copy_model_normals(void* handle, int32_t model, float* out, int32_t capacity);
int32_t hive_copy_model_uvs(void* handle, int32_t model, float* out, int32_t capacity);
int32_t hive_copy_model_indices(void* handle, int32_t model, int32_t geoset, uint32_t* out, int32_t capacity);

/* 把 BLP / TGA / DDS 解成 RGBA8，返回尺寸或零。 */
int32_t hive_image_info_path(void* handle, const char* path, int32_t* width, int32_t* height);
int32_t hive_copy_image_path(void* handle, const char* path, uint8_t* out, int32_t capacity);

#ifdef __cplusplus
}
#endif

#endif
