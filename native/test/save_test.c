#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hive_c.h"

static float max_diff(const float* a, const float* b, int count) {
	float worst = 0.f;
	for (int i = 0; i < count; ++i) {
		const float diff = fabsf(a[i] - b[i]);
		if (diff > worst) {
			worst = diff;
		}
	}
	return worst;
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	HiveMapInfo info;
	hive_map_info(hive, &info);
	const int corners = info.width * info.height;

	float* before = calloc(corners, sizeof(float));
	float* edited = calloc(corners, sizeof(float));
	hive_copy_heights(hive, before, corners);

	hive_brush_shape(hive, 1);
	hive_brush_size(hive, 20);
	hive_set_height_tool(hive, 0);
	hive_brush_move(hive, 96.f, 48.f);
	hive_brush_begin(hive);
	for (int i = 0; i < 10; ++i) {
		hive_brush_apply(hive, 0.05);
	}
	hive_brush_end(hive);

	hive_set_texture_tool(hive, "Lrok");
	hive_brush_size(hive, 16);
	hive_brush_begin(hive);
	hive_brush_apply(hive, 0.016);
	hive_brush_end(hive);

	hive_copy_heights(hive, edited, corners);
	printf("编辑前后最大高度差 %.3f\n", max_diff(before, edited, corners));

	const char* target = "/tmp/hive_edit_out";
	if (hive_save(hive, target, error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	hive = hive_open(target, "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("重新打开失败：%s\n", error);
		return 1;
	}

	float* reloaded = calloc(corners, sizeof(float));
	hive_copy_heights(hive, reloaded, corners);
	printf("重新打开后与编辑结果最大差 %.4f\n", max_diff(edited, reloaded, corners));
	printf("保存目录含有 war3map.w3e：%s\n", hive_map_directory(hive));

	HiveTile tile;
	HiveTile* tiles = calloc(info.tiles_x * info.tiles_y, sizeof(HiveTile));
	hive_copy_tiles(hive, tiles, info.tiles_x * info.tiles_y);
	tile = tiles[(info.tiles_y / 2) * info.tiles_x + info.tiles_x / 2];
	printf("中心格子纹理索引 %u\n", tile.texture[0] & 0xFFFF);

	free(tiles);
	free(reloaded);
	free(edited);
	free(before);
	hive_close(hive);
	return 0;
}
