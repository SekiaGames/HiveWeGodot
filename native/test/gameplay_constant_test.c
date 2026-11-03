#include <stdio.h>
#include <string.h>

#include "hive_c.h"

static int find_key(void* hive, const char* target) {
	const int count = hive_gameplay_constant_count(hive);
	for (int i = 0; i < count; ++i) {
		const char* key = hive_gameplay_constant_key(hive, i);
		if (key != NULL && strcmp(key, target) == 0) {
			return i;
		}
	}
	return -1;
}

static void read_value(void* hive, const int index, char* out, const int size) {
	snprintf(out, (size_t)size, "%s", hive_gameplay_constant_value(hive, index));
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	const int count = hive_gameplay_constant_count(hive);
	printf("游戏常量 %d 个\n", count);
	if (count < 50) {
		printf("常量太少\n");
		return 1;
	}

	const int index = find_key(hive, "agiattackspeedbonus");
	if (index < 0) {
		printf("找不到 agiattackspeedbonus\n");
		return 1;
	}

	char key[64] = {0};
	char label[128] = {0};
	char before[64] = {0};
	snprintf(key, sizeof(key), "%s", hive_gameplay_constant_key(hive, index));
	snprintf(label, sizeof(label), "%s", hive_gameplay_constant_label(hive, index));
	read_value(hive, index, before, sizeof(before));
	printf("第 %d 个：%s 名字 [%s] 原值 %s，改过吗 %d\n", index, key, label, before,
		   hive_gameplay_constant_modified(hive, index));

	if (label[0] == 0) {
		printf("名字空\n");
		return 1;
	}

	if (hive_set_gameplay_constant(hive, index, "0.05") != 0) {
		printf("设置失败\n");
		return 1;
	}

	char after[64] = {0};
	read_value(hive, index, after, sizeof(after));
	const int modified = hive_gameplay_constant_modified(hive, index);
	printf("改完 %s，改过吗 %d\n", after, modified);

	if (hive_save_map(hive, "/tmp/constants.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* second = hive_open("/tmp/constants.w3m", "testdata/warcraft", error, sizeof(error));
	if (second == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}

	const int reopened_index = find_key(second, "agiattackspeedbonus");
	char reopened[64] = {0};
	if (reopened_index >= 0) {
		read_value(second, reopened_index, reopened, sizeof(reopened));
	}
	printf("重开后 %s，改过吗 %d\n", reopened, hive_gameplay_constant_modified(second, reopened_index));

	if (hive_set_gameplay_constant(second, reopened_index, before) != 0) {
		printf("改回原值失败\n");
		return 1;
	}
	const int restored_modified = hive_gameplay_constant_modified(second, reopened_index);
	printf("改回原值后改过吗 %d\n", restored_modified);

	const int ok = strcmp(after, "0.05") == 0 && modified == 1 && reopened_index >= 0
		&& strcmp(reopened, "0.05") == 0 && hive_gameplay_constant_modified(second, reopened_index) == 0;
	printf("%s\n", ok ? "游戏常量可改并随地图存盘" : "游戏常量有问题");

	hive_close(second);
	return ok ? 0 : 1;
}
