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

	HiveDoodad* doodads = calloc(info.doodad_count, sizeof(HiveDoodad));
	hive_copy_doodads(hive, doodads, info.doodad_count);

	const HiveDoodad first = doodads[0];
	printf("第 0 个装饰物 %s 在 (%.2f, %.2f, %.2f)\n", hive_doodad_id(hive, 0), first.x, first.y, first.z);

	// 从正上方往下打
	const float origin[3] = {first.x, first.z + 60.f, first.y};
	const float direction[3] = {0.f, -1.f, 0.f};
	const int hit = hive_pick_doodad(hive, origin, direction, 200.f);
	printf("正上方垂直下打 -> %d（期望 0）\n", hit);

	// 从旁边很远的地方打，应该打不中
	const float far_origin[3] = {first.x + 30.f, first.z + 60.f, first.y};
	const int miss = hive_pick_doodad(hive, far_origin, direction, 200.f);
	printf("偏 30 格打 -> %d（期望 -1 或别的）\n", miss);

	// 往上打，什么都碰不到
	const float up[3] = {0.f, 1.f, 0.f};
	printf("往上打 -> %d（期望 -1）\n", hive_pick_doodad(hive, origin, up, 200.f));

	// 单位
	HiveUnit* units = calloc(info.unit_count, sizeof(HiveUnit));
	hive_copy_units(hive, units, info.unit_count);
	if (info.unit_count > 0) {
		const HiveUnit u = units[0];
		const float unit_origin[3] = {u.x, u.z + 40.f, u.y};
		printf("单位 0 %s 在 (%.1f, %.1f, %.1f) -> 拾取 %d\n",
			   hive_unit_id(hive, 0), u.x, u.y, u.z, hive_pick_unit(hive, unit_origin, direction, 120.f));
	}

	free(units);
	free(doodads);
	hive_close(hive);
	return 0;
}
