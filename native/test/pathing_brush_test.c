#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int differing_cells(const unsigned char* a, const unsigned char* b, const int count) {
	int diff = 0;
	for (int i = 0; i < count; ++i) {
		if (a[i] != b[i]) {
			diff++;
		}
	}
	return diff;
}

static void paint(void* hive, const float x, const float y) {
	hive_brush_move(hive, x, y);
	hive_brush_begin(hive);
	hive_brush_apply(hive, 1.0);
	hive_brush_end(hive);
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

	hive_brush_size(hive, 8);
	if (hive_set_pathing_tool(hive, 0x02 | 0x08, 1) != 0) {
		printf("切路径笔刷失败\n");
		return 1;
	}

	paint(hive, 96.f, 48.f);
	unsigned char* painted = snapshot(hive, &width, &height);
	const int added = differing_cells(before, painted, count);
	printf("加不可走+不可建后变了 %d 格\n", added);

	int blocked = 0;
	for (int i = 0; i < count; ++i) {
		if ((painted[i] & 0x02) != 0) {
			blocked++;
		}
	}

	if (hive_set_pathing_tool(hive, 0x02 | 0x08, 2) != 0) {
		printf("切擦除失败\n");
		return 1;
	}
	paint(hive, 96.f, 48.f);
	unsigned char* erased = snapshot(hive, &width, &height);
	const int removed = differing_cells(painted, erased, count);
	printf("减掉后变了 %d 格\n", removed);

	hive_set_pathing_tool(hive, 0x02 | 0x04 | 0x08, 0);
	paint(hive, 40.f, 40.f);
	unsigned char* replaced = snapshot(hive, &width, &height);
	const int replaced_count = differing_cells(erased, replaced, count);
	printf("覆盖后变了 %d 格\n", replaced_count);

	hive_undo(hive);
	unsigned char* undone = snapshot(hive, &width, &height);
	const int undo_diff = differing_cells(erased, undone, count);
	printf("撤销后与覆盖前差 %d 格\n", undo_diff);

	hive_redo(hive);
	unsigned char* redone = snapshot(hive, &width, &height);
	const int redo_diff = differing_cells(replaced, redone, count);
	printf("重做后与覆盖后差 %d 格\n", redo_diff);

	hive_undo(hive);
	hive_undo(hive);
	hive_undo(hive);
	unsigned char* restored = snapshot(hive, &width, &height);
	const int restored_diff = differing_cells(before, restored, count);
	printf("全部撤销后与原图差 %d 格\n", restored_diff);

	const int ok = added > 0 && removed > 0 && replaced_count > 0 && undo_diff == 0 && redo_diff == 0
		&& restored_diff == 0;
	printf("%s\n", ok ? "路径笔刷可覆盖、加、减，带撤销重做" : "路径笔刷有问题");
	if (blocked == 0) {
		printf("注意：加完没有不可走格子\n");
	}

	free(before);
	free(painted);
	free(erased);
	free(replaced);
	free(undone);
	free(redone);
	free(restored);
	hive_close(hive);
	return ok ? 0 : 1;
}
