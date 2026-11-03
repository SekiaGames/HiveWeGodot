#include <stdio.h>
#include <stdlib.h>

#include "hive_c.h"

static unsigned char* snapshot(void* hive, int* width, int* height) {
	if (hive_pathing_info(hive, width, height) != 0) {
		return NULL;
	}
	const int count = *width * *height;
	unsigned char* cells = calloc((size_t)count, 1);
	hive_copy_pathing(hive, cells, count);
	return cells;
}

static int differing_cells(const unsigned char* a, const unsigned char* b, int count) {
	int diff = 0;
	for (int i = 0; i < count; ++i) {
		if (a[i] != b[i]) {
			diff++;
		}
	}
	return diff;
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
	unsigned char* before = snapshot(hive, &width, &height);
	if (before == NULL) {
		printf("读不到路径图\n");
		return 1;
	}
	const int count = width * height;
	printf("路径图 %d×%d\n", width, height);

	HiveMapInfo info;
	hive_map_info(hive, &info);
	HiveDoodad* doodads = calloc((size_t)info.doodad_count, sizeof(HiveDoodad));
	hive_copy_doodads(hive, doodads, info.doodad_count);

	const char* id = hive_doodad_id(hive, 0);
	const float x = doodads[0].x + 6.f;
	const float y = doodads[0].y + 6.f;
	printf("拿第一个装饰物 [%s]，在 %.1f, %.1f 旁边放一棵树\n", id, x, y);

	const int added = hive_add_doodad(hive, "LTlt", x, y, 0.f, 0.f);
	unsigned char* after_add = snapshot(hive, &width, &height);
	const int changed_by_place = differing_cells(before, after_add, count);
	printf("放置后路径图变了 %d 格（新索引 %d）\n", changed_by_place, added);

	hive_map_info(hive, &info);
	const int last = info.doodad_count - 1;
	int indices[1] = {last};
	const int removed = hive_remove_selection(hive, 0, indices, 1);
	unsigned char* after_remove = snapshot(hive, &width, &height);
	const int changed_by_delete = differing_cells(before, after_remove, count);
	printf("删除后与原图差 %d 格（删除返回 %d）\n", changed_by_delete, removed);

	const int ok = added >= 0 && removed == 1 && changed_by_place > 0 && changed_by_delete == 0;
	printf("%s\n", ok ? "路径图随增删更新" : "路径图没跟着更新");

	free(doodads);
	free(before);
	free(after_add);
	free(after_remove);
	hive_close(hive);
	return ok ? 0 : 1;
}
