#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "hive_c.h"

static int read_starts(void* hive, int* players, float* xs, float* ys, int capacity) {
	const int count = hive_start_location_count(hive);
	for (int i = 0; i < count && i < capacity; ++i) {
		if (hive_start_location(hive, i, &players[i], &xs[i], &ys[i]) != 0) {
			return -1;
		}
	}
	return count;
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	int players[32] = {0};
	float xs[32] = {0.f};
	float ys[32] = {0.f};
	const int before = read_starts(hive, players, xs, ys, 32);
	if (before < 0) {
		printf("读开始点失败\n");
		return 1;
	}
	printf("原有 %d 个开始点\n", before);
	for (int i = 0; i < before; ++i) {
		printf("  玩家 %d 在 %.1f, %.1f\n", players[i], xs[i], ys[i]);
	}

	if (before < 2) {
		printf("这张图的开始点不够测\n");
		return 1;
	}

	// 没动过的那个开始点，用它反推地形偏移（w3i 存的是原始坐标）
	float keep_x = 0.f;
	float keep_y = 0.f;
	hive_player_start(hive, players[1], &keep_x, &keep_y);
	const float offset_x = keep_x - xs[1] * 128.f;
	const float offset_y = keep_y - ys[1] * 128.f;

	const int moved = players[0];
	if (hive_set_start_location(hive, moved, 40.f, 30.f) != 0) {
		printf("移动开始点失败\n");
		return 1;
	}

	if (hive_start_location_count(hive) != before) {
		printf("移动后数量变了：%d\n", hive_start_location_count(hive));
		return 1;
	}

	if (hive_set_start_location(hive, 5, 120.f, 40.f) != 0) {
		printf("新建开始点失败\n");
		return 1;
	}
	const int added = hive_start_location_count(hive);
	printf("新建玩家 5 的开始点后共 %d 个\n", added);

	int found = -1;
	float found_x = 0.f;
	float found_y = 0.f;
	for (int i = 0; i < added; ++i) {
		int player = 0;
		float x = 0.f;
		float y = 0.f;
		hive_start_location(hive, i, &player, &x, &y);
		if (player == moved) {
			found = i;
			found_x = x;
			found_y = y;
		}
	}
	printf("玩家 %d 现在在 %.1f, %.1f\n", moved, found_x, found_y);

	if (hive_save_map(hive, "/tmp/starts.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* second = hive_open("/tmp/starts.w3m", "testdata/warcraft", error, sizeof(error));
	if (second == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}
	int reopened_players[32] = {0};
	float reopened_xs[32] = {0.f};
	float reopened_ys[32] = {0.f};
	const int reopened = read_starts(second, reopened_players, reopened_xs, reopened_ys, 32);
	printf("重开后 %d 个开始点\n", reopened);

	int moved_survived = 0;
	int added_survived = 0;
	for (int i = 0; i < reopened; ++i) {
		if (reopened_players[i] == moved && fabsf(reopened_xs[i] - 40.f) < 0.5f && fabsf(reopened_ys[i] - 30.f) < 0.5f) {
			moved_survived = 1;
		}
		if (reopened_players[i] == 5 && fabsf(reopened_xs[i] - 120.f) < 0.5f) {
			added_survived = 1;
		}
	}

	float w3i_x = 0.f;
	float w3i_y = 0.f;
	const int w3i_ok = hive_player_start(second, moved, &w3i_x, &w3i_y) == 0;
	printf("玩家 %d 的 w3i 出生点 %.1f, %.1f\n", moved, w3i_x, w3i_y);

	const int removed = hive_remove_start_location(second, 5);
	printf("删掉玩家 5 的开始点返回 %d，剩 %d 个\n", removed, hive_start_location_count(second));

	const int w3i_close = w3i_ok && fabsf(w3i_x - (40.f * 128.f + offset_x)) < 1.f
		&& fabsf(w3i_y - (30.f * 128.f + offset_y)) < 1.f;

	const int ok = found >= 0 && fabsf(found_x - 40.f) < 0.5f && added == before + 1 && moved_survived && added_survived
		&& w3i_close && removed == 1 && hive_start_location_count(second) == before;
	printf("%s\n", ok ? "开始点可移动、新增、删除并随地图存盘" : "开始点有问题");

	hive_close(second);
	return ok ? 0 : 1;
}
