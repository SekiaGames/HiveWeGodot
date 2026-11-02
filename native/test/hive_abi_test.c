#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hive_c.h"

int main(int argc, char** argv) {
	const char* map_path = argc > 1 ? argv[1] : "testdata/maps/(2)BootyBay.w3m";
	const char* data_dir = argc > 2 ? argv[2] : "testdata/warcraft";

	char error[512] = {0};
	void* hive = hive_open(map_path, data_dir, error, (int32_t)sizeof error);
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	HiveMapInfo info;
	if (hive_map_info(hive, &info) != 0) {
		printf("读地图信息失败\n");
		hive_close(hive);
		return 1;
	}

	printf("地图 %s\n", map_path);
	printf("  %d×%d 顶点，%d×%d 格，tileset 0x%02X\n",
		   info.width, info.height, info.tiles_x, info.tiles_y, info.tileset_id);
	printf("  地面纹理 %d 种，装饰物 %d，单位 %d，区域 %d\n",
		   info.texture_count, info.doodad_count, info.unit_count, info.region_count);

	printf("  纹理 id：");
	for (int32_t i = 0; i < info.texture_count; i++) {
		printf("%s ", hive_texture_id(hive, i));
	}
	printf("\n");

	const int32_t vertex_count = info.width * info.height;
	float* heights = malloc(sizeof(float) * (size_t)vertex_count);
	const int32_t heights_read = hive_copy_heights(hive, heights, vertex_count);

	float lowest = heights[0];
	float highest = heights[0];
	for (int32_t i = 0; i < heights_read; i++) {
		if (heights[i] < lowest) {
			lowest = heights[i];
		}
		if (heights[i] > highest) {
			highest = heights[i];
		}
	}
	printf("  高度 %d 个，范围 %.2f ~ %.2f，[0]=%.2f [100]=%.2f\n",
		   heights_read, lowest, highest, heights[0], heights[100 < heights_read ? 100 : 0]);

	const int32_t tile_count = info.tiles_x * info.tiles_y;
	HiveTile* tiles = malloc(sizeof(HiveTile) * (size_t)tile_count);
	const int32_t tiles_read = hive_copy_tiles(hive, tiles, tile_count);
	printf("  格子 %d 个，[0] 纹理 %u/%u/%u/%u 变体 %u\n",
		   tiles_read, tiles[0].texture[0], tiles[0].texture[1],
		   tiles[0].texture[2], tiles[0].texture[3], tiles[0].variation);

	uint8_t* water = malloc((size_t)vertex_count);
	const int32_t water_read = hive_copy_water_exists(hive, water, vertex_count);
	int32_t water_count = 0;
	for (int32_t i = 0; i < water_read; i++) {
		water_count += water[i] != 0;
	}
	printf("  有水顶点 %d / %d\n", water_count, water_read);

	free(heights);
	free(tiles);
	free(water);
	hive_close(hive);
	printf("完成\n");
	return 0;
}
