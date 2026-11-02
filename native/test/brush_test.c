#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hive_c.h"

static HiveMapInfo info;
static HiveTile* baseline;
static HiveTile* current;
static float* heights;

static int differing_tiles(void) {
	int count = 0;
	for (int i = 0; i < info.tiles_x * info.tiles_y; ++i) {
		if (memcmp(&baseline[i], &current[i], sizeof(HiveTile)) != 0) {
			count++;
		}
	}
	return count;
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}
	if (hive_map_info(hive, &info) != 0) {
		printf("读地图信息失败\n");
		return 1;
	}

	const int tile_count = info.tiles_x * info.tiles_y;
	const int corner_count = info.width * info.height;
	baseline = calloc(tile_count, sizeof(HiveTile));
	current = calloc(tile_count, sizeof(HiveTile));
	heights = calloc(corner_count, sizeof(float));

	hive_copy_tiles(hive, baseline, tile_count);
	hive_copy_heights(hive, heights, corner_count);
	const float height_before = heights[(info.height / 2) * info.width + info.width / 2];

	hive_brush_shape(hive, 0);
	hive_brush_size(hive, 8);
	hive_brush_move(hive, 96.f, 48.f);

	if (hive_set_texture_tool(hive, "Lgrs") != 0) {
		printf("选纹理工具失败\n");
		return 1;
	}
	hive_brush_begin(hive);
	hive_brush_apply(hive, 0.016);
	hive_brush_end(hive);

	hive_copy_tiles(hive, current, tile_count);
	const int painted = differing_tiles();
	printf("画草地后改变 %d 个格子\n", painted);

	hive_undo(hive);
	hive_copy_tiles(hive, current, tile_count);
	printf("撤销后剩余 %d 个不同\n", differing_tiles());

	hive_redo(hive);
	hive_copy_tiles(hive, current, tile_count);
	printf("重做后恢复 %d 个不同\n", differing_tiles());

	hive_undo(hive);

	if (hive_set_height_tool(hive, 0) != 0) {
		printf("选高度工具失败\n");
		return 1;
	}
	hive_brush_size(hive, 12);
	hive_brush_begin(hive);
	for (int i = 0; i < 6; ++i) {
		hive_brush_apply(hive, 0.05);
	}
	hive_brush_end(hive);

	hive_copy_heights(hive, heights, corner_count);
	const float height_after = heights[(info.height / 2) * info.width + info.width / 2];
	printf("抬升前后高度 %.3f -> %.3f\n", height_before, height_after);

	hive_undo(hive);
	hive_copy_heights(hive, heights, corner_count);
	printf("撤销后高度 %.3f\n", heights[(info.height / 2) * info.width + info.width / 2]);

	hive_set_cell_tool(hive, 0);
	hive_brush_size(hive, 6);
	hive_brush_begin(hive);
	hive_brush_apply(hive, 0.016);
	hive_brush_end(hive);

	unsigned char* water = calloc(corner_count, 1);
	hive_copy_water_exists(hive, water, corner_count);
	int wet = 0;
	for (int i = 0; i < corner_count; ++i) {
		wet += water[i];
	}
	printf("加水后有水顶点 %d\n", wet);

	hive_undo(hive);
	hive_copy_water_exists(hive, water, corner_count);
	wet = 0;
	for (int i = 0; i < corner_count; ++i) {
		wet += water[i];
	}
	printf("撤销后有水顶点 %d\n", wet);

	free(water);
	free(heights);
	free(current);
	free(baseline);
	hive_close(hive);
	return 0;
}
