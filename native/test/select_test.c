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

	int* hits = calloc(before.doodad_count, sizeof(int));
	const int hit_count = hive_query_area(hive, 20.f, 10.f, 60.f, 40.f, 0, hits, before.doodad_count);
	printf("框选 (20,10)-(60,40) 命中装饰物 %d 个\n", hit_count);
	for (int i = 0; i < 5 && i < hit_count; ++i) {
		printf("  #%d %s\n", hits[i], hive_doodad_id(hive, hits[i]));
	}

	int* reversed_hits = calloc(before.doodad_count, sizeof(int));
	const int reversed = hive_query_area(hive, 60.f, 40.f, 20.f, 10.f, 0, reversed_hits, before.doodad_count);
	printf("反向框选命中 %d 个（应相同）\n", reversed);

	if (hit_count <= 0) {
		printf("没选中东西\n");
		return 1;
	}

	const int removed = hive_remove_selection(hive, 0, hits, hit_count);
	HiveMapInfo after;
	hive_map_info(hive, &after);
	printf("删掉 %d 个，装饰物 %d -> %d\n", removed, before.doodad_count, after.doodad_count);

	hive_undo(hive);
	HiveMapInfo undone;
	hive_map_info(hive, &undone);
	printf("撤销后 %d 个\n", undone.doodad_count);

	int* units = calloc(before.doodad_count, sizeof(int));
	const int boxed_units = hive_query_area(hive, 20.f, 10.f, 60.f, 40.f, 1, units, before.doodad_count);
	const int all_units = hive_query_area(hive, 0.f, 0.f, 193.f, 97.f, 1, units, before.doodad_count);
	printf("同一块区域命中单位 %d 个，整张图 %d 个\n", boxed_units, all_units);

	const int ok = reversed == hit_count && removed == hit_count && undone.doodad_count == before.doodad_count;

	free(units);
	free(reversed_hits);
	free(hits);
	hive_close(hive);
	return ok ? 0 : 1;
}
