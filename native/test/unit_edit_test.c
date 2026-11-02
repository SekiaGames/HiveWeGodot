#include <math.h>
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

	const int type_count = hive_unit_type_count(hive);
	printf("可放置单位类型 %d 个\n", type_count);
	for (int i = 0; i < 5 && i < type_count; ++i) {
		printf("  %s\n", hive_unit_type_id(hive, i));
	}

	HiveMapInfo before;
	hive_map_info(hive, &before);

	char placeable[8] = {0};
	for (int i = 0; i < before.unit_count && placeable[0] == 0; ++i) {
		const char* id = hive_unit_id(hive, i);
		if (id != NULL && id[0] != 0 && strcmp(id, "sloc") != 0) {
			strncpy(placeable, id, sizeof(placeable) - 1);
		}
	}
	if (placeable[0] == 0) {
		printf("地图里没有可复用的单位 id\n");
		return 1;
	}
	printf("用 %s 试放置\n", placeable);

	const int added = hive_add_unit(hive, placeable, 50.f, 50.f, 1.f, 0);
	if (added < 0) {
		printf("放置单位失败\n");
		return 1;
	}

	HiveMapInfo after;
	hive_map_info(hive, &after);
	printf("放在索引 %d，单位 %d -> %d\n", added, before.unit_count, after.unit_count);

	HiveUnit* units = calloc(after.unit_count, sizeof(HiveUnit));
	hive_copy_units(hive, units, after.unit_count);
	printf("新单位 %s 在 (%.1f, %.1f, %.1f)，角度 %.2f，玩家 %d\n",
		   hive_unit_id(hive, added), units[added].x, units[added].y, units[added].z,
		   units[added].angle, units[added].player);

	hive_undo(hive);
	HiveMapInfo undone;
	hive_map_info(hive, &undone);
	printf("撤销后 %d 个\n", undone.unit_count);

	const int indices[2] = {0, 1};
	const int copied = hive_copy_unit_selection(hive, indices, 2);
	float anchor[2] = {0.f, 0.f};
	hive_unit_clipboard_anchor(hive, anchor);
	printf("复制 %d 个单位，剪贴板 %d 个，中心 (%.2f, %.2f)\n", copied, hive_unit_clipboard_count(hive), anchor[0], anchor[1]);

	HiveUnit source[2];
	hive_copy_unit_clipboard(hive, source, 2);

	const int first = hive_place_unit_clipboard(hive, 80.f, 60.f);
	if (first < 0) {
		printf("粘贴失败\n");
		return 1;
	}

	HiveMapInfo pasted;
	hive_map_info(hive, &pasted);
	printf("粘贴到索引 %d，单位 %d -> %d\n", first, undone.unit_count, pasted.unit_count);

	HiveUnit* all = calloc(pasted.unit_count, sizeof(HiveUnit));
	hive_copy_units(hive, all, pasted.unit_count);

	int relative_ok = 1;
	for (int i = 0; i < 2; ++i) {
		const float expected_x = 80.f + source[i].x - anchor[0];
		const float expected_y = 60.f + source[i].y - anchor[1];
		printf("  新 #%d 在 (%.2f, %.2f)，期望 (%.2f, %.2f)\n",
			   i, all[first + i].x, all[first + i].y, expected_x, expected_y);
		if (fabsf(all[first + i].x - expected_x) > 0.01f || fabsf(all[first + i].y - expected_y) > 0.01f) {
			relative_ok = 0;
		}
	}

	hive_undo(hive);
	HiveMapInfo back;
	hive_map_info(hive, &back);
	printf("撤销粘贴后 %d 个，相对位置 %s\n", back.unit_count, relative_ok ? "保持" : "错位");

	const int ok = relative_ok && undone.unit_count == before.unit_count && back.unit_count == undone.unit_count;

	free(all);
	free(units);
	hive_close(hive);
	return ok ? 0 : 1;
}
