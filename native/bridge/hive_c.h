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

#ifdef __cplusplus
}
#endif

#endif
