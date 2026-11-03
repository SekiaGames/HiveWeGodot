#include <stdio.h>
#include <stdlib.h>

#include "hive_c.h"

static int count_unwalkable(void* hive, int width, int height) {
	uint8_t* cells = malloc((size_t)width * height);
	hive_copy_pathing(hive, cells, width * height);
	int count = 0;
	for (int i = 0; i < width * height; ++i) {
		if (cells[i] & 0x02) {
			count++;
		}
	}
	free(cells);
	return count;
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	int width = 0;
	int height = 0;
	hive_pathing_info(hive, &width, &height);

	const int pathing = hive_tile_pathing(hive, 0);
	const int base = hive_tile_base_pathing(hive, 0);
	printf("第 0 块纹理：路径位 %d，默认位 %d\n", pathing, base);
	if (pathing < 0 || base < 0) {
		printf("取不到路径位\n");
		return 1;
	}

	const int before = count_unwalkable(hive, width, height);
	const int changed = pathing ^ 0x02;
	if (hive_set_tile_pathing(hive, 0, changed) != 0) {
		printf("设置路径位失败\n");
		return 1;
	}
	const int after = count_unwalkable(hive, width, height);
	const int readback = hive_tile_pathing(hive, 0);
	printf("第 0 块改成 %d，读回 %d；不可走格子 %d → %d\n", changed, readback, before, after);

	hive_set_tile_pathing(hive, 0, pathing);
	const int restored = hive_tile_pathing(hive, 0);
	printf("改回 %d\n", restored);

	const int ok = readback == changed && restored == pathing && before != after;
	printf("%s\n", ok ? "瓦片路径覆盖生效" : "瓦片路径结果不符");

	hive_close(hive);
	return ok ? 0 : 1;
}
