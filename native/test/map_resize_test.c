#include <stdio.h>
#include <stdlib.h>

#include "hive_c.h"

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	HiveMapInfo before;
	hive_map_info(hive, &before);
	printf("原始 %dx%d，装饰物 %d，单位 %d\n", before.width, before.height, before.doodad_count, before.unit_count);

	int playable_width = 0;
	int playable_height = 0;
	hive_map_playable_size(hive, &playable_width, &playable_height);
	printf("可玩 %dx%d\n", playable_width, playable_height);

	HiveDoodad* doodads = calloc(before.doodad_count, sizeof(HiveDoodad));
	hive_copy_doodads(hive, doodads, before.doodad_count);
	printf("第一个装饰物在 (%.1f, %.1f)\n", doodads[0].x, doodads[0].y);

	if (hive_resize_map(hive, 10, 10, 10, 10) != 0) {
		printf("调整尺寸失败\n");
		return 1;
	}

	HiveMapInfo after;
	hive_map_info(hive, &after);
	printf("调整后 %dx%d，装饰物 %d，单位 %d\n", after.width, after.height, after.doodad_count, after.unit_count);

	HiveDoodad* moved = calloc(after.doodad_count, sizeof(HiveDoodad));
	hive_copy_doodads(hive, moved, after.doodad_count);
	printf("第一个装饰物在 (%.1f, %.1f)，位移 (%.1f, %.1f)\n", moved[0].x, moved[0].y, moved[0].x - doodads[0].x,
		   moved[0].y - doodads[0].y);

	hive_map_playable_size(hive, &playable_width, &playable_height);
	printf("可玩 %dx%d\n", playable_width, playable_height);

	if (hive_resize_map(hive, -1000, 0, 0, 0) == 0) {
		printf("过小的尺寸应该被拒绝\n");
		return 1;
	}
	printf("过小的尺寸被拒绝\n");

	if (hive_save_map(hive, "/tmp/edited_size.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* reopened = hive_open("/tmp/edited_size.w3m", "testdata/warcraft", error, sizeof(error));
	if (reopened == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}
	HiveMapInfo reopened_info;
	hive_map_info(reopened, &reopened_info);
	printf("重开后 %dx%d，装饰物 %d\n", reopened_info.width, reopened_info.height, reopened_info.doodad_count);

	const int ok = after.width == before.width + 10 && after.height == before.height + 10
		&& after.doodad_count == before.doodad_count && moved[0].x == doodads[0].x + 10.f
		&& moved[0].y == doodads[0].y + 10.f && reopened_info.width == after.width
		&& reopened_info.height == after.height;

	free(moved);
	free(doodads);
	hive_close(reopened);
	return ok ? 0 : 1;
}
