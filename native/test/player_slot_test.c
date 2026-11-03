#include <stdio.h>
#include <string.h>

#include "hive_c.h"

static int slot_count(void* hive, const int slot, int* type, int* race, int* fixed, char* name, const int size) {
	*type = hive_player_slot_type(hive, slot);
	if (*type < 0) {
		return 0;
	}
	*race = hive_player_slot_race(hive, slot);
	*fixed = hive_player_slot_fixed(hive, slot);
	const char* value = hive_player_slot_name(hive, slot);
	snprintf(name, (size_t)size, "%s", value == NULL ? "" : value);
	return 1;
}

static void dump(void* hive, const char* label) {
	printf("%s：\n", label);
	int used = 0;
	for (int slot = 0; slot < 24; ++slot) {
		int type = 0;
		int race = 0;
		int fixed = 0;
		char name[128] = {0};
		if (slot_count(hive, slot, &type, &race, &fixed, name, sizeof(name))) {
			printf("  槽位 %d：控制器 %d 种族 %d 固定出生点 %d 名字 %s\n", slot, type, race, fixed, name);
			used++;
		}
	}
	printf("  共 %d 个槽位有人（w3i 里 %d 条）\n", used, hive_player_count(hive));
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	dump(hive, "原来");

	int first_type = hive_player_slot_type(hive, 0);
	int first_race = hive_player_slot_race(hive, 0);
	char first_name[128] = {0};
	const char* name = hive_player_slot_name(hive, 0);
	snprintf(first_name, sizeof(first_name), "%s", name == NULL ? "" : name);
	printf("槽位 0：控制器 %d 种族 %d 名字 [%s]\n", first_type, first_race, first_name);

	if (hive_set_player_slot(hive, 1, 1, 2, "电脑兽族", 1) != 0) {
		printf("改槽位 1 失败\n");
		return 1;
	}

	if (hive_set_player_slot(hive, 5, 2, 4, "中立暗夜", 0) != 0) {
		printf("加槽位 5 失败\n");
		return 1;
	}

	if (hive_set_player_slot(hive, 7, -1, 0, NULL, 0) != 0) {
		printf("清空不存在的槽位应该成功\n");
		return 1;
	}

	dump(hive, "改完");

	if (hive_save_map(hive, "/tmp/players.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* second = hive_open("/tmp/players.w3m", "testdata/warcraft", error, sizeof(error));
	if (second == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}
	dump(second, "重开后");

	int type = 0;
	int race = 0;
	int fixed = 0;
	char reopened_name[128] = {0};
	const int has_first = slot_count(second, 1, &type, &race, &fixed, reopened_name, sizeof(reopened_name));
	const int first_ok = has_first && type == 1 && race == 2 && fixed == 1 && strcmp(reopened_name, "电脑兽族") == 0;

	slot_count(second, 5, &type, &race, &fixed, reopened_name, sizeof(reopened_name));
	const int fifth_ok = type == 2 && race == 4 && strcmp(reopened_name, "中立暗夜") == 0;

	slot_count(second, 0, &type, &race, &fixed, reopened_name, sizeof(reopened_name));
	const int untouched_ok = strcmp(reopened_name, first_name) == 0 && race == first_race;

	if (hive_set_player_slot(second, 1, -1, 0, NULL, 0) != 0 || hive_player_slot_type(second, 1) != -1) {
		printf("清空槽位 1 失败\n");
		return 1;
	}
	printf("清空槽位 1 后 w3i 还剩 %d 条\n", hive_player_count(second));

	const int ok = first_ok && fifth_ok && untouched_ok;
	printf("%s\n", ok ? "玩家槽位可改控制器、种族、名字、固定出生点并随地图存盘" : "玩家槽位有问题");

	hive_close(second);
	return ok ? 0 : 1;
}
