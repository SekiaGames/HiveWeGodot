#include <stdio.h>

#include "hive_c.h"

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	int checked = 0;
	int hidden_geosets = 0;
	int posed_models = 0;

	for (int i = 0; i < 4; ++i) {
		const int model = hive_model_from_unit(hive, i);
		if (model < 0) {
			continue;
		}

		HiveModelInfo info;
		hive_model_info(hive, model, &info);
		checked++;

		int hidden = 0;
		for (int g = 0; g < info.geoset_count; ++g) {
			HiveGeosetInfo geoset;
			hive_model_geoset(hive, model, g, &geoset);
			if (geoset.alpha <= 0.001f) {
				hidden++;
			}
		}
		hidden_geosets += hidden;

		printf("[%s] 模型 %d 层 %d，隐藏 %d，范围 x %.1f..%.1f y %.1f..%.1f\n", hive_unit_id(hive, i), model,
			   info.geoset_count, hidden, info.min_x, info.max_x, info.min_y, info.max_y);
		posed_models += info.max_x > info.min_x ? 1 : 0;
	}

	printf("检查模型 %d 个，其中 %d 个有隐藏层（共 %d 层），%d 个有有效包围盒\n", checked, hidden_geosets > 0 ? 1 : 0,
		   hidden_geosets, posed_models);

	hive_close(hive);
	return checked > 0 && hidden_geosets > 0 && posed_models == checked ? 0 : 1;
}
