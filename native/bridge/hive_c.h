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

int32_t hive_map_info(void* handle, HiveMapInfo* out);

/* 拷贝地形数据，返回实际元素数。capacity 不够时只写 capacity 个。 */
int32_t hive_copy_heights(void* handle, float* out, int32_t capacity);
int32_t hive_copy_tiles(void* handle, HiveTile* out, int32_t capacity);
int32_t hive_copy_ground_exists(void* handle, uint8_t* out, int32_t capacity);
int32_t hive_copy_water_exists(void* handle, uint8_t* out, int32_t capacity);

/* 第 index 个地面纹理的 id（四字符，如 Adrt），越界返回 NULL。 */
const char* hive_texture_id(void* handle, int32_t index);

#ifdef __cplusplus
}
#endif

#endif
