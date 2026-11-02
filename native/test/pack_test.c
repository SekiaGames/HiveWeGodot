#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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
	const int tile_count = info.tiles_x * info.tiles_y;

	float* edited = calloc(corners, sizeof(float));

	hive_brush_shape(hive, 1);
	hive_brush_size(hive, 24);
	hive_set_height_tool(hive, 0);
	hive_brush_move(hive, 96.f, 48.f);
	hive_brush_begin(hive);
	for (int i = 0; i < 20; ++i) {
		hive_brush_apply(hive, 0.05);
	}
	hive_brush_end(hive);

	hive_set_texture_tool(hive, "Lrok");
	hive_brush_begin(hive);
	hive_brush_apply(hive, 0.016);
	hive_brush_end(hive);

	hive_copy_heights(hive, edited, corners);

	const char* output = "/tmp/edited_map.w3m";
	if (hive_save_map(hive, output, error, sizeof(error)) != 0) {
		printf("打包失败：%s\n", error);
		return 1;
	}

	FILE* file = fopen(output, "rb");
	fseek(file, 0, SEEK_END);
	const long size = ftell(file);
	fclose(file);
	printf("输出 %s，%ld 字节\n", output, size);

	hive_close(hive);

	hive = hive_open(output, "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开新地图失败：%s\n", error);
		return 1;
	}

	float* reloaded = calloc(corners, sizeof(float));
	hive_copy_heights(hive, reloaded, corners);
	printf("重新打开后高度最大差 %.4f\n", max_diff(edited, reloaded, corners));

	HiveTile* tiles = calloc(tile_count, sizeof(HiveTile));
	hive_copy_tiles(hive, tiles, tile_count);
	int rock = 0;
	for (int i = 0; i < tile_count; ++i) {
		if ((tiles[i].texture[0] & 0xFFFF) == 3) {
			rock++;
		}
	}
	printf("Lrok 格子 %d 个\n", rock);

	HiveMapInfo reopened;
	hive_map_info(hive, &reopened);
	printf("尺寸 %d×%d，装饰物 %d，单位 %d\n", reopened.width, reopened.height, reopened.doodad_count, reopened.unit_count);

	free(tiles);
	free(reloaded);
	free(edited);
	hive_close(hive);
	return 0;
}
