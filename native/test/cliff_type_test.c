#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hive_c.h"

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	const int count = hive_cliffset_count(hive);
	printf("悬崖类型 %d 个\n", count);
	if (count < 2) {
		printf("悬崖类型太少\n");
		return 1;
	}

	char first[8] = {0};
	char second[8] = {0};
	snprintf(first, sizeof(first), "%s", hive_cliffset_id(hive, 0));
	snprintf(second, sizeof(second), "%s", hive_cliffset_id(hive, 1));
	printf("  0：%s，1：%s\n", first, second);

	HiveMapInfo info;
	hive_map_info(hive, &info);
	const int cells = info.width * info.height;
	float* before = malloc(sizeof(float) * cells);
	float* after = malloc(sizeof(float) * cells);
	hive_copy_heights(hive, before, cells);

	if (hive_set_cliff_tool(hive, 3, second) != 0) {
		printf("设定悬崖类型失败\n");
		return 1;
	}
	hive_brush_size(hive, 6);
	hive_brush_move(hive, 80.f, 50.f);
	hive_brush_begin(hive);
	hive_brush_apply(hive, 0.1);
	hive_brush_end(hive);

	hive_copy_heights(hive, after, cells);
	int changed = 0;
	for (int i = 0; i < cells; ++i) {
		if (before[i] != after[i]) {
			changed++;
		}
	}
	printf("用 %s 升一级后高度变了 %d 个点\n", second, changed);

	const int ok = changed > 0;
	printf("%s\n", ok ? "悬崖类型可选，写地形生效" : "悬崖工具没动地形");

	free(before);
	free(after);
	hive_close(hive);
	return ok ? 0 : 1;
}
