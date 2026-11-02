#include <stdio.h>
#include <stdlib.h>

#include "hive_c.h"

static int count_distinct(const HiveDoodad* doodads, int start, int count, int* variations) {
	int distinct = 0;
	for (int i = 0; i < count; ++i) {
		int seen = 0;
		for (int j = 0; j < i && !seen; ++j) {
			const HiveDoodad* a = &doodads[start + i];
			const HiveDoodad* b = &doodads[start + j];
			if (a->variation == b->variation && a->angle == b->angle && a->scale_x == b->scale_x) {
				seen = 1;
			}
		}
		if (!seen) {
			distinct++;
			variations[i] = 1;
		}
	}
	return distinct;
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	HiveMapInfo before;
	hive_map_info(hive, &before);

	for (int i = 0; i < 20; ++i) {
		if (hive_add_doodad(hive, "LTlt", 20.f + i * 2.f, 20.f, 2.f, 0.f) < 0) {
			printf("放置失败\n");
			return 1;
		}
	}

	HiveMapInfo plain;
	hive_map_info(hive, &plain);
	HiveDoodad* doodads = calloc(plain.doodad_count, sizeof(HiveDoodad));
	hive_copy_doodads(hive, doodads, plain.doodad_count);

	int* marks = calloc(20, sizeof(int));
	const int plain_distinct = count_distinct(doodads, before.doodad_count, 20, marks);
	printf("不开随机：20 个里有 %d 种\n", plain_distinct);

	hive_set_doodad_random(hive, 7);
	for (int i = 0; i < 20; ++i) {
		if (hive_add_doodad(hive, "LTlt", 20.f + i * 2.f, 40.f, 2.f, 0.f) < 0) {
			printf("放置失败\n");
			return 1;
		}
	}

	HiveMapInfo random_info;
	hive_map_info(hive, &random_info);
	doodads = realloc(doodads, random_info.doodad_count * sizeof(HiveDoodad));
	hive_copy_doodads(hive, doodads, random_info.doodad_count);

	const int random_distinct = count_distinct(doodads, plain.doodad_count, 20, marks);
	printf("开随机：20 个里有 %d 种\n", random_distinct);

	for (int i = 0; i < 3; ++i) {
		const HiveDoodad* doodad = &doodads[plain.doodad_count + i];
		printf("  变体 %d，角度 %.2f，缩放 %.2f\n", doodad->variation, doodad->angle, doodad->scale_x);
	}

	hive_set_doodad_random(hive, 0);
	const int ok = plain_distinct == 1 && random_distinct > 1;

	free(marks);
	free(doodads);
	hive_close(hive);
	return ok ? 0 : 1;
}
