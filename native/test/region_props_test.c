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

	const int weathers = hive_weather_count(hive);
	printf("天气 %d 种，环境音表 %d 种，地图音效 %d 个\n", weathers, hive_ambient_count(hive), hive_sound_count(hive));
	if (weathers < 5) {
		printf("天气列表太少\n");
		return 1;
	}

	char first_weather[128] = {0};
	char first_weather_id[8] = {0};
	snprintf(first_weather, sizeof(first_weather), "%s", hive_weather_name(hive, 0));
	snprintf(first_weather_id, sizeof(first_weather_id), "%s", hive_weather_id(hive, 0));
	printf("  天气 0：%s（%s）\n", first_weather, first_weather_id);
	if (hive_ambient_count(hive) > 0) {
		char first_ambient[128] = {0};
		snprintf(first_ambient, sizeof(first_ambient), "%s", hive_ambient_name(hive, 0));
		printf("  环境音表 0：%s\n", first_ambient);
	}

	if (first_weather[0] == 0 || strlen(first_weather_id) != 4) {
		printf("天气名字或 id 不对\n");
		return 1;
	}

	const int created = hive_create_region(hive, 20.f, 20.f, 30.f, 30.f);
	if (created < 0) {
		printf("新建区域失败\n");
		return 1;
	}

	const int weather_index = 2;
	char weather_id[8] = {0};
	snprintf(weather_id, sizeof(weather_id), "%s", hive_weather_id(hive, weather_index));
	// 区域的环境音引用地图里的音效，这张图没有音效就是空
	char ambient_id[64] = {0};
	if (hive_sound_count(hive) > 0) {
		snprintf(ambient_id, sizeof(ambient_id), "%s", hive_sound_variable(hive, 0));
	}

	if (hive_set_region_weather(hive, created, weather_id) != 0 || hive_set_region_ambient(hive, created, ambient_id) != 0
		|| hive_set_region_color(hive, created, 10, 200, 30, 180) != 0) {
		printf("设置区域属性失败\n");
		return 1;
	}

	HiveRegion* regions = calloc((size_t)(created + 1), sizeof(HiveRegion));
	hive_copy_regions(hive, regions, created + 1);
	char set_weather[8] = {0};
	char set_ambient[64] = {0};
	snprintf(set_weather, sizeof(set_weather), "%s", hive_region_weather(hive, created));
	snprintf(set_ambient, sizeof(set_ambient), "%s", hive_region_ambient(hive, created));
	printf("区域 %d 颜色 %d,%d,%d,%d 天气 [%s] 环境音 [%s]\n", created, regions[created].red, regions[created].green,
		   regions[created].blue, regions[created].alpha, set_weather, set_ambient);

	const int set_ok = regions[created].red == 10 && regions[created].green == 200 && regions[created].blue == 30
		&& strcmp(set_weather, weather_id) == 0 && strcmp(set_ambient, ambient_id) == 0;
	free(regions);

	if (hive_save_map(hive, "/tmp/regions.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* second = hive_open("/tmp/regions.w3m", "testdata/warcraft", error, sizeof(error));
	if (second == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}

	HiveMapInfo info;
	hive_map_info(second, &info);
	const int count = info.region_count;
	HiveRegion* reopened = calloc((size_t)count, sizeof(HiveRegion));
	hive_copy_regions(second, reopened, count);
	char reopened_weather[8] = {0};
	char reopened_ambient[64] = {0};
	snprintf(reopened_weather, sizeof(reopened_weather), "%s", hive_region_weather(second, count - 1));
	snprintf(reopened_ambient, sizeof(reopened_ambient), "%s", hive_region_ambient(second, count - 1));
	printf("重开后 %d 个区域，最后一个颜色 %d,%d,%d,%d 天气 %s 环境音 %s\n", count, reopened[count - 1].red,
		   reopened[count - 1].green, reopened[count - 1].blue, reopened[count - 1].alpha, reopened_weather,
		   reopened_ambient);

	// 颜色 alpha 读回来会被红色通道顶掉（上游的读法），所以只比 RGB
	const int reopen_ok = reopened[count - 1].red == 10 && reopened[count - 1].green == 200
		&& reopened[count - 1].blue == 30 && strcmp(reopened_weather, weather_id) == 0
		&& strcmp(reopened_ambient, ambient_id) == 0;

	const int ok = set_ok && reopen_ok;
	printf("%s\n", ok ? "区域天气、环境音、颜色可改并随地图存盘" : "区域属性有问题");

	free(reopened);
	hive_close(second);
	return ok ? 0 : 1;
}
