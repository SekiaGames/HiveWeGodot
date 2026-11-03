#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "hive_c.h"

static void raise_terrain(void* hive, const float x, const float y, const int frames) {
	hive_set_height_tool(hive, 0);
	hive_brush_size(hive, 16);
	hive_brush_move(hive, x, y);
	hive_brush_begin(hive);
	for (int i = 0; i < frames; ++i) {
		hive_brush_apply(hive, 0.05);
	}
	hive_brush_end(hive);
}

static float doodad_height(void* hive, const int index) {
	HiveMapInfo info;
	hive_map_info(hive, &info);
	HiveDoodad* doodads = calloc((size_t)info.doodad_count, sizeof(HiveDoodad));
	hive_copy_doodads(hive, doodads, info.doodad_count);
	const float z = index < info.doodad_count ? doodads[index].z : -1.f;
	free(doodads);
	return z;
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	const int flags = hive_terrain_brush_flags(hive);
	printf("默认开关位 %d（0x%02x）\n", flags, flags);
	if ((flags & 0x02) == 0) {
		printf("默认应该会更新装饰物高度\n");
		return 1;
	}

	HiveMapInfo info;
	hive_map_info(hive, &info);
	HiveDoodad* doodads = calloc((size_t)info.doodad_count, sizeof(HiveDoodad));
	hive_copy_doodads(hive, doodads, info.doodad_count);

	int target = -1;
	for (int i = 0; i < info.doodad_count; ++i) {
		if (doodads[i].x > 8.f && doodads[i].y > 8.f && doodads[i].x < info.width - 8.f) {
			target = i;
			break;
		}
	}
	if (target < 0) {
		printf("找不到可用的装饰物\n");
		return 1;
	}
	const float x = doodads[target].x;
	const float y = doodads[target].y;
	const float before = doodads[target].z;
	free(doodads);
	printf("拿装饰物 %d 在 %.1f, %.1f，高度 %.2f\n", target, x, y, before);

	// 关掉「更新装饰物高度」，抬地形不应改变它
	hive_set_terrain_brush_flags(hive, flags & ~0x02);
	raise_terrain(hive, x, y, 8);
	const float without_update = doodad_height(hive, target);
	printf("关掉联动抬地形后高度 %.2f\n", without_update);

	// 打开后再抬一次，应该跟着动
	hive_set_terrain_brush_flags(hive, flags | 0x02);
	raise_terrain(hive, x, y, 8);
	const float with_update = doodad_height(hive, target);
	printf("打开联动再抬后高度 %.2f\n", with_update);

	if (hive_terrain_brush_flags(hive) != (flags | 0x02)) {
		printf("开关读回来不对：%d\n", hive_terrain_brush_flags(hive));
		return 1;
	}

	const int ok = fabsf(without_update - before) < 0.05f && with_update > before + 0.2f;
	printf("%s\n", ok ? "地形笔刷联动开关生效（装饰物高度跟着地形走）" : "联动开关没生效");

	hive_close(hive);
	return ok ? 0 : 1;
}
