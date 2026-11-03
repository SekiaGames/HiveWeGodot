#include <stdio.h>
#include <string.h>

#include "hive_c.h"

static void copy(const char* value, char* out, const int size) {
	snprintf(out, (size_t)size, "%s", value == NULL ? "" : value);
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_new_map("testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("新建失败：%s\n", error);
		return 1;
	}

	HiveMapInfo info;
	hive_map_info(hive, &info);
	int playable_width = 0;
	int playable_height = 0;
	hive_map_playable_size(hive, &playable_width, &playable_height);
	int pathing_width = 0;
	int pathing_height = 0;
	hive_pathing_info(hive, &pathing_width, &pathing_height);

	char name[128] = {0};
	char author[128] = {0};
	char description[128] = {0};
	copy(hive_map_name(hive), name, sizeof(name));
	copy(hive_map_author(hive), author, sizeof(author));
	copy(hive_map_description(hive), description, sizeof(description));
	printf("新图 %dx%d，可玩 %dx%d，路径图 %dx%d，装饰物 %d 单位 %d 区域 %d\n", info.width, info.height,
		   playable_width, playable_height, pathing_width, pathing_height, info.doodad_count, info.unit_count,
		   info.region_count);
	printf("名字 [%s] 作者 [%s] 描述 [%s]\n", name, author, description);

	const int place_doodad = hive_add_doodad(hive, "LTlt", 32.f, 32.f, 0.f, 0.f);
	const int place_unit = hive_add_unit(hive, "ngns", 30.f, 30.f, 0.f, 0);
	const int region = hive_create_region(hive, 10.f, 10.f, 20.f, 20.f);
	printf("放树 %d，放兵 %d，建区域 %d\n", place_doodad, place_unit, region);

	if (hive_save_map(hive, "/tmp/blank.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* second = hive_open("/tmp/blank.w3m", "testdata/warcraft", error, sizeof(error));
	if (second == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}

	HiveMapInfo reopened;
	hive_map_info(second, &reopened);
	char reopened_name[128] = {0};
	copy(hive_map_name(second), reopened_name, sizeof(reopened_name));
	printf("重开后 %dx%d，装饰物 %d 单位 %d 区域 %d，名字 [%s]\n", reopened.width, reopened.height,
		   reopened.doodad_count, reopened.unit_count, reopened.region_count, reopened_name);

	const int ok = info.width == 65 && info.height == 65 && playable_width == 64 && playable_height == 64
		&& info.doodad_count == 0 && info.unit_count == 0 && info.region_count == 0
		&& strcmp(name, "Just another Warcraft III map") == 0 && strcmp(author, "Unknown") == 0
		&& strcmp(description, "Nondescript") == 0 && place_doodad >= 0 && place_unit >= 0 && region >= 0
		&& reopened.doodad_count == 1 && reopened.unit_count == 1 && reopened.region_count == 1
		&& strcmp(reopened_name, name) == 0;
	printf("%s\n", ok ? "空白地图可新建、可编辑、可存盘" : "新建地图有问题");

	hive_close(second);
	return ok ? 0 : 1;
}
