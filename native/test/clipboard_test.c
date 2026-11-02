#include <math.h>
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

	HiveMapInfo info;
	hive_map_info(hive, &info);
	printf("装饰物 %d 个\n", info.doodad_count);

	int indices[3] = {0, 1, 2};
	const int copied = hive_copy_selection(hive, indices, 3);
	printf("复制 %d 个，剪贴板 %d 个\n", copied, hive_clipboard_count(hive));

	float anchor[2] = {0.f, 0.f};
	hive_clipboard_anchor(hive, anchor);
	printf("中心点 (%.2f, %.2f)\n", anchor[0], anchor[1]);

	HiveDoodad source[3];
	hive_copy_clipboard(hive, source, 3);
	for (int i = 0; i < 3; ++i) {
		printf("  %s 在 (%.2f, %.2f)，角度 %.2f，缩放 %.2f\n",
			   hive_clipboard_id(hive, i), source[i].x, source[i].y, source[i].angle, source[i].scale_x);

		const int model = hive_model_from_clipboard(hive, i);
		HiveModelInfo model_info;
		if (model < 0 || hive_model_info(hive, model, &model_info) != 0) {
			printf("模型读取失败\n");
			return 1;
		}
	}

	const float target_x = 100.f;
	const float target_y = 50.f;
	const int first = hive_place_clipboard(hive, target_x, target_y);
	if (first < 0) {
		printf("放置失败\n");
		return 1;
	}

	HiveMapInfo placed;
	hive_map_info(hive, &placed);
	printf("放到索引 %d，装饰物 %d -> %d\n", first, info.doodad_count, placed.doodad_count);

	HiveDoodad* all = calloc(placed.doodad_count, sizeof(HiveDoodad));
	hive_copy_doodads(hive, all, placed.doodad_count);

	int relative_ok = 1;
	for (int i = 0; i < 3; ++i) {
		const HiveDoodad* doodad = &all[first + i];
		const float expected_x = target_x + source[i].x - anchor[0];
		const float expected_y = target_y + source[i].y - anchor[1];
		printf("  新 #%d 在 (%.2f, %.2f)，期望 (%.2f, %.2f)，高度 %.2f\n",
			   i, doodad->x, doodad->y, expected_x, expected_y, doodad->z);
		if (fabsf(doodad->x - expected_x) > 0.51f || fabsf(doodad->y - expected_y) > 0.51f) {
			relative_ok = 0;
		}
	}

	hive_undo(hive);
	HiveMapInfo undone;
	hive_map_info(hive, &undone);
	printf("撤销后 %d 个\n", undone.doodad_count);

	hive_redo(hive);
	HiveMapInfo redone;
	hive_map_info(hive, &redone);
	printf("重做后 %d 个\n", redone.doodad_count);

	const int ok = relative_ok && undone.doodad_count == info.doodad_count && redone.doodad_count == placed.doodad_count;
	printf("相对位置 %s\n", relative_ok ? "保持" : "错位");

	free(all);
	hive_close(hive);
	return ok ? 0 : 1;
}
