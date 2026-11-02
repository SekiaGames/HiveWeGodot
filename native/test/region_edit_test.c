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

	HiveMapInfo before;
	hive_map_info(hive, &before);
	printf("原始区域 %d 个\n", before.region_count);

	const int index = hive_create_region(hive, 20.f, 10.f, 40.f, 30.f);
	if (index < 0) {
		printf("新建区域失败\n");
		return 1;
	}

	HiveMapInfo after;
	hive_map_info(hive, &after);
	HiveRegion* regions = calloc((size_t)after.region_count, sizeof(HiveRegion));
	hive_copy_regions(hive, regions, after.region_count);
	printf("新建后 %d 个，新区域 [%s] 范围 %.0f,%.0f - %.0f,%.0f 颜色 %d,%d,%d\n", after.region_count,
		   hive_region_name(hive, index), regions[index].left, regions[index].bottom, regions[index].right,
		   regions[index].top, regions[index].red, regions[index].green, regions[index].blue);

	hive_set_region_name(hive, index, "测试区域");
	hive_set_region_rect(hive, index, 15.f, 25.f, 5.f, 5.f);
	hive_copy_regions(hive, regions, after.region_count);
	printf("改名后 [%s]，范围 %.0f,%.0f - %.0f,%.0f\n", hive_region_name(hive, index), regions[index].left,
		   regions[index].bottom, regions[index].right, regions[index].top);

	if (hive_save_map(hive, "/tmp/edited_region.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* reopened = hive_open("/tmp/edited_region.w3m", "testdata/warcraft", error, sizeof(error));
	if (reopened == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}

	HiveMapInfo info;
	hive_map_info(reopened, &info);
	const char* last = hive_region_name(reopened, info.region_count - 1);
	char last_name[64] = {0};
	if (last != NULL) {
		strncpy(last_name, last, sizeof(last_name) - 1);
	}
	printf("重开后区域 %d 个，最后一个是 [%s]\n", info.region_count, last_name);

	const int deleted = hive_delete_region(reopened, 0);
	hive_map_info(reopened, &info);
	printf("删除第一个区域返回 %d，剩 %d 个\n", deleted, info.region_count);

	const int ok = index == before.region_count && after.region_count == before.region_count + 1
		&& strcmp(last_name, "测试区域") == 0 && deleted == 0
		&& info.region_count == after.region_count - 1;

	free(regions);
	hive_close(reopened);
	return ok ? 0 : 1;
}
