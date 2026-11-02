#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "hive_c.h"

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	float red[3] = {0};
	float blue[3] = {0};
	float purple[3] = {0};
	const int red_ok = hive_player_color(0, red);
	const int blue_ok = hive_player_color(1, blue);
	const int purple_ok = hive_player_color(3, purple);
	printf("玩家色 0 %.3f,%.3f,%.3f　1 %.3f,%.3f,%.3f　3 %.3f,%.3f,%.3f\n", red[0], red[1], red[2], blue[0], blue[1],
		   blue[2], purple[0], purple[1], purple[2]);
	printf("越界玩家返回 %d\n", hive_player_color(99, red));

	HiveMapInfo info;
	hive_map_info(hive, &info);
	HiveUnit* units = calloc((size_t)info.unit_count, sizeof(HiveUnit));
	hive_copy_units(hive, units, info.unit_count);

	int with_team = 0;
	int team_geosets = 0;
	for (int i = 0; i < info.unit_count; ++i) {
		const int model = hive_model_from_unit(hive, i);
		if (model < 0) {
			continue;
		}
		HiveModelInfo model_info;
		hive_model_info(hive, model, &model_info);
		int layers = 0;
		for (int g = 0; g < model_info.geoset_count; ++g) {
			HiveGeosetInfo geoset;
			hive_model_geoset(hive, model, g, &geoset);
			if (geoset.team_color != 0) {
				layers++;
			}
		}
		if (layers > 0) {
			with_team++;
			team_geosets += layers;
			if (with_team <= 3) {
				printf("  [%s] 玩家 %d 有 %d 层队伍色\n", hive_unit_id(hive, i), units[i].player, layers);
			}
		}
	}

	const int ok = red_ok == 0 && blue_ok == 0 && purple_ok == 0 && fabsf(red[0] - 1.f) < 0.01f
		&& fabsf(blue[2] - 1.f) < 0.01f && with_team > 0;
	printf("带队伍色的单位 %d 个，共 %d 层\n", with_team, team_geosets);

	free(units);
	hive_close(hive);
	return ok ? 0 : 1;
}
