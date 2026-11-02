#include <stdio.h>
#include <string.h>

#include "hive_c.h"

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	char name[128] = {0};
	char author[128] = {0};
	char description[512] = {0};
	char suggested[64] = {0};
	strncpy(name, hive_map_name(hive), sizeof(name) - 1);
	strncpy(author, hive_map_author(hive), sizeof(author) - 1);
	strncpy(description, hive_map_description(hive), sizeof(description) - 1);
	strncpy(suggested, hive_map_suggested_players(hive), sizeof(suggested) - 1);

	printf("名称 [%s]\n作者 [%s]\n建议玩家 [%s]\n", name, author, suggested);
	printf("描述 %zu 个字符：[%.60s]\n", strlen(description), description);

	int versions[6] = {0};
	hive_map_versions(hive, versions);
	int playable_width = 0;
	int playable_height = 0;
	hive_map_playable_size(hive, &playable_width, &playable_height);
	printf("地图版本 %d，编辑器 %d，游戏 %d.%d.%d.%d，可玩 %dx%d\n", versions[0], versions[1], versions[2], versions[3],
		   versions[4], versions[5], playable_width, playable_height);

	if (hive_set_map_name(hive, "测试地图名字") != 0 || hive_set_map_author(hive, "测试作者") != 0) {
		printf("写入失败\n");
		return 1;
	}

	if (hive_save_map(hive, "/tmp/edited_info.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* reopened = hive_open("/tmp/edited_info.w3m", "testdata/warcraft", error, sizeof(error));
	if (reopened == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}

	char new_name[128] = {0};
	char new_author[128] = {0};
	strncpy(new_name, hive_map_name(reopened), sizeof(new_name) - 1);
	strncpy(new_author, hive_map_author(reopened), sizeof(new_author) - 1);
	printf("重开后名称 [%s] 作者 [%s]\n", new_name, new_author);

	const int ok = strcmp(new_name, "测试地图名字") == 0 && strcmp(new_author, "测试作者") == 0;
	hive_close(reopened);
	return ok ? 0 : 1;
}
