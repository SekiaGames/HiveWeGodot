#include <stdio.h>
#include <string.h>

#include "hive_c.h"

static void read_string(const char* value, char* out, const int size) {
	snprintf(out, (size_t)size, "%s", value == NULL ? "" : value);
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	const int screens = hive_loading_screen_count(hive);
	printf("阵营载入画面 %d 张，第一张 %s\n", screens, hive_loading_screen_name(hive, 0));
	if (screens < 5) {
		printf("载入画面列表太少\n");
		return 1;
	}

	char title[128] = {0};
	char subtitle[128] = {0};
	char text[256] = {0};
	read_string(hive_map_loading_screen_title(hive), title, sizeof(title));
	read_string(hive_map_loading_screen_subtitle(hive), subtitle, sizeof(subtitle));
	read_string(hive_map_loading_screen_text(hive), text, sizeof(text));
	printf("原来：画面序号 %d，标题 [%s] 副标题 [%s] 文本 [%s]\n", hive_map_loading_screen(hive), title, subtitle,
		   text);
	printf("导入模型 %d 个\n", hive_imported_model_count(hive));

	if (hive_set_map_loading_screen(hive, 4) != 0 || hive_set_map_loading_screen_model(hive, "") != 0) {
		printf("设置载入画面失败\n");
		return 1;
	}
	if (hive_set_map_loading_screen_title(hive, "测试标题") != 0
		|| hive_set_map_loading_screen_subtitle(hive, "测试副标题") != 0
		|| hive_set_map_loading_screen_text(hive, "测试提示文本") != 0) {
		printf("设置载入文本失败\n");
		return 1;
	}

	read_string(hive_map_loading_screen_title(hive), title, sizeof(title));
	const int set_number = hive_map_loading_screen(hive);
	printf("改完：画面序号 %d，标题 [%s]\n", set_number, title);

	if (hive_save_map(hive, "/tmp/loading.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* second = hive_open("/tmp/loading.w3m", "testdata/warcraft", error, sizeof(error));
	if (second == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}

	char reopened_title[128] = {0};
	char reopened_subtitle[128] = {0};
	char reopened_text[256] = {0};
	read_string(hive_map_loading_screen_title(second), reopened_title, sizeof(reopened_title));
	read_string(hive_map_loading_screen_subtitle(second), reopened_subtitle, sizeof(reopened_subtitle));
	read_string(hive_map_loading_screen_text(second), reopened_text, sizeof(reopened_text));
	printf("重开后：画面序号 %d，标题 [%s] 副标题 [%s] 文本 [%s]\n", hive_map_loading_screen(second),
		   reopened_title, reopened_subtitle, reopened_text);

	if (hive_set_map_loading_screen(second, -1) != 0 || hive_map_loading_screen(second) != -1) {
		printf("清掉载入画面失败\n");
		return 1;
	}

	const int ok = set_number == 4;
	const int strings_ok = strcmp(reopened_title, "测试标题") == 0 && strcmp(reopened_subtitle, "测试副标题") == 0
		&& strcmp(reopened_text, "测试提示文本") == 0;
	const int screen_ok = hive_map_loading_screen(second) == -1;
	const int all = ok && strings_ok && screen_ok;
	printf("%s\n", all ? "载入画面可改并随地图存盘" : "载入画面有问题");

	hive_close(second);
	return all ? 0 : 1;
}
