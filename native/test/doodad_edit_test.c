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

	const int type_count = hive_doodad_type_count(hive);
	printf("可放置类型 %d 个\n", type_count);
	for (int i = 0; i < 5 && i < type_count; ++i) {
		printf("  %s\n", hive_doodad_type_id(hive, i));
	}

	HiveMapInfo before;
	hive_map_info(hive, &before);

	const int added = hive_add_doodad(hive, "LTlt", 96.f, 48.f, 3.f, 0.f);
	if (added < 0) {
		printf("放置失败\n");
		return 1;
	}
	printf("放在索引 %d\n", added);

	HiveMapInfo after;
	hive_map_info(hive, &after);
	printf("装饰物 %d -> %d\n", before.doodad_count, after.doodad_count);

	HiveDoodad* doodads = calloc(after.doodad_count, sizeof(HiveDoodad));
	hive_copy_doodads(hive, doodads, after.doodad_count);
	printf("新装饰物 %s 在 (%.1f, %.1f, %.1f)\n",
		   hive_doodad_id(hive, added), doodads[added].x, doodads[added].y, doodads[added].z);

	if (hive_save_map(hive, "/tmp/edited_map.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}

	free(doodads);

	// 重新打开看还在不在
	void* second = hive_open("/tmp/edited_map.w3m", "testdata/warcraft", error, sizeof(error));
	if (second == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}
	HiveMapInfo reopened;
	hive_map_info(second, &reopened);
	printf("重开后装饰物 %d 个\n", reopened.doodad_count);

	if (hive_remove_doodad(second, 0) == 0) {
		HiveMapInfo removed;
		hive_map_info(second, &removed);
		printf("删掉一个后 %d 个\n", removed.doodad_count);
	}

	hive_close(second);
	hive_close(hive);
	return 0;
}
