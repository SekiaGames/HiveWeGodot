#include <stdio.h>
#include <string.h>

#include "hive_c.h"

static int check(void* hive, const int category, const char* id, const char* name, const char* folder) {
	char got_name[128] = {0};
	char got_folder[128] = {0};

	const char* value = hive_object_type_name(hive, category, id);
	if (value != NULL) {
		snprintf(got_name, sizeof(got_name), "%s", value);
	}

	for (int level = 0; level < 4; ++level) {
		value = hive_object_type_folder(hive, category, id, level);
		if (value == NULL) {
			break;
		}
		char part[128] = {0};
		snprintf(part, sizeof(part), "%s", value);
		if (level > 0) {
			strncat(got_folder, "/", sizeof(got_folder) - strlen(got_folder) - 1);
		}
		strncat(got_folder, part, sizeof(got_folder) - strlen(got_folder) - 1);
	}

	const int ok = strcmp(got_name, name) == 0 && strcmp(got_folder, folder) == 0;
	printf("  %s [%s] 名字 %s 分类 %s%s\n", id, ok ? "对" : "错", got_name, got_folder, ok ? "" : " <- 期望不符");
	if (!ok) {
		printf("    期望 %s / %s\n", name, folder);
	}
	return ok;
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	int ok = 1;
	printf("单位：\n");
	ok &= check(hive, 0, "hfoo", "步兵", "人族/单位");
	ok &= check(hive, 0, "hgtw", "防御塔", "人族/建筑");
	ok &= check(hive, 0, "Hmkg", "山丘之王", "人族/英雄");
	ok &= check(hive, 0, "ogru", "兽族步兵", "兽族/单位");

	printf("装饰物与可破坏物：\n");
	ok &= check(hive, 2, "AOhs", "空树桩", "环境");
	ok &= check(hive, 3, "LTlt", "夏树墙", "树/可破坏");

	printf("未知 id 退回自身：\n");
	ok &= check(hive, 0, "zzzz", "zzzz", "其它/单位");

	printf("%s\n", ok ? "对象名字与分类正确" : "对象名字或分类不对");
	hive_close(hive);
	return ok ? 0 : 1;
}
