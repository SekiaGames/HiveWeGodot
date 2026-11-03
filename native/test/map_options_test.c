#include <math.h>
#include <stdio.h>
#include <string.h>

#include "hive_c.h"

static void dump(void* hive, const char* label) {
	int style = 0;
	float start = 0.f;
	float end = 0.f;
	float density = 0.f;
	int red = 0;
	int green = 0;
	int blue = 0;
	hive_map_fog(hive, &style, &start, &end, &density, &red, &green, &blue);

	int water_red = 0;
	int water_green = 0;
	int water_blue = 0;
	int water_alpha = 0;
	hive_map_water_color(hive, &water_red, &water_green, &water_blue, &water_alpha);

	char weather[8] = {0};
	const char* value = hive_map_weather_id(hive);
	snprintf(weather, sizeof(weather), "%s", value == NULL ? "" : value);

	char environment[64] = {0};
	value = hive_map_sound_environment(hive);
	snprintf(environment, sizeof(environment), "%s", value == NULL ? "" : value);

	printf("%s：选项位 %d，雾 %d（%.0f..%.0f 密度 %.2f，颜色 %d,%d,%d），天气 [%s]，"
		   "环境音 [%s]，光照地形集 %d 氛围地形集 %d，水色 %d,%d,%d,%d，数据版本 %d\n",
		   label, hive_map_option_flags(hive), style, start, end, density, red, green, blue, weather, environment,
		   hive_map_custom_light_tileset(hive), hive_map_custom_ambience_tileset(hive), water_red, water_green,
		   water_blue, water_alpha, hive_map_game_data_set(hive));
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	dump(hive, "原来");

	const int tilesets = hive_tileset_count(hive);
	printf("地形集 %d 种，第一个 %s（%s）\n", tilesets, hive_tileset_name(hive, 0), hive_tileset_key(hive, 0));
	if (tilesets < 5) {
		printf("地形集列表太少\n");
		return 1;
	}

	const int original_flags = hive_map_option_flags(hive);
	hive_set_map_option_flags(hive, original_flags ^ 0x01);
	if (hive_map_option_flags(hive) == original_flags) {
		printf("近战图开关没生效\n");
		return 1;
	}
	hive_set_map_option_flags(hive, 0x01 | 0x20);

	if (hive_set_map_fog(hive, 2, 1200.f, 3400.f, 0.6f, 12, 34, 56) != 0) {
		printf("设置雾失败\n");
		return 1;
	}

	char weather[8] = {0};
	snprintf(weather, sizeof(weather), "%s", hive_weather_id(hive, 3));
	if (hive_set_map_weather_id(hive, weather) != 0) {
		printf("设置天气失败\n");
		return 1;
	}

	if (hive_set_map_sound_environment(hive, "lake") != 0) {
		printf("设置环境音失败\n");
		return 1;
	}

	const char light_key = hive_tileset_key(hive, 2)[0];
	if (hive_set_map_custom_light_tileset(hive, light_key) != 0
		|| hive_set_map_custom_ambience_tileset(hive, light_key) != 0) {
		printf("设置自定义光照失败\n");
		return 1;
	}

	if (hive_set_map_water_color(hive, 20, 40, 60, 128) != 0) {
		printf("设置水色失败\n");
		return 1;
	}

	const int data_set = hive_map_game_data_set(hive);
	if (hive_set_map_game_data_set(hive, data_set + 1) != 0) {
		printf("设置数据版本失败\n");
		return 1;
	}

	dump(hive, "改完");

	if (hive_save_map(hive, "/tmp/options.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* second = hive_open("/tmp/options.w3m", "testdata/warcraft", error, sizeof(error));
	if (second == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}
	dump(second, "重开后");

	int style = 0;
	float start = 0.f;
	float end = 0.f;
	float density = 0.f;
	int red = 0;
	int green = 0;
	int blue = 0;
	hive_map_fog(second, &style, &start, &end, &density, &red, &green, &blue);
	int water_red = 0;
	int water_green = 0;
	int water_blue = 0;
	int water_alpha = 0;
	hive_map_water_color(second, &water_red, &water_green, &water_blue, &water_alpha);
	char reopened_weather[8] = {0};
	snprintf(reopened_weather, sizeof(reopened_weather), "%s", hive_map_weather_id(second));
	char reopened_environment[64] = {0};
	snprintf(reopened_environment, sizeof(reopened_environment), "%s", hive_map_sound_environment(second));

	const int fog_ok = style == 2 && fabsf(start - 1200.f) < 1.f && fabsf(end - 3400.f) < 1.f
		&& fabsf(density - 0.6f) < 0.01f && red == 12 && green == 34 && blue == 56;
	const int weather_ok = strcmp(reopened_weather, weather) == 0;
	const int water_ok = water_red == 20 && water_green == 40 && water_blue == 60 && water_alpha == 128;
	const int flags_ok = (hive_map_option_flags(second) & 0x21) == 0x21;
	const int tileset_ok = hive_map_custom_light_tileset(second) == (unsigned char)light_key;
	const int environment_ok = strcmp(reopened_environment, "lake") == 0;

	const int ok = fog_ok && weather_ok && water_ok && flags_ok && tileset_ok && environment_ok;
	printf("%s\n", ok ? "地图选项可改并随地图存盘" : "地图选项有问题");

	hive_close(second);
	return ok ? 0 : 1;
}
