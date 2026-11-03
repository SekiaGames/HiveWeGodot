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

	int models = 0;
	int animated = 0;
	int ok = 1;

	for (int i = 0; i < 6; ++i) {
		const int model = hive_model_from_unit(hive, i);
		if (model < 0) {
			continue;
		}
		models++;

		const int sequences = hive_model_sequence_count(hive, model);
		if (sequences == 0) {
			printf("[%s] 模型 %d 没有序列\n", hive_unit_id(hive, i), model);
			continue;
		}

		int start = 0;
		int end = 0;
		hive_model_sequence_range(hive, model, 0, &start, &end);
		printf("[%s] 模型 %d 序列 %d，第一条 [%s] %d..%d\n", hive_unit_id(hive, i), model, sequences,
			   hive_model_sequence_name(hive, model, 0), start, end);

		HiveModelInfo info;
		hive_model_info(hive, model, &info);
		const int count = info.vertex_count;
		if (count <= 0) {
			ok = 0;
			continue;
		}

		float* first = malloc(sizeof(float) * 3 * (size_t)count);
		float* middle = malloc(sizeof(float) * 3 * (size_t)count);

		hive_model_apply_pose(hive, model, 0, start);
		hive_copy_model_positions(hive, model, first, count * 3);
		hive_model_apply_pose(hive, model, 0, (start + end) / 2);
		hive_copy_model_positions(hive, model, middle, count * 3);

		double moved = 0.0;
		float largest = 0.f;
		for (int k = 0; k < count; ++k) {
			for (int c = 0; c < 3; ++c) {
				const float a = first[k * 3 + c];
				const float b = middle[k * 3 + c];
				moved += fabs(a - b);
				largest = fmaxf(largest, fabsf(b));
			}
		}

		printf("  起止帧位移合计 %.1f，中点帧最大坐标 %.1f\n", moved, largest);
		if (!isfinite(largest) || largest > 100000.f) {
			printf("  坐标异常\n");
			ok = 0;
		}
		if (moved > 0.5) {
			animated++;
		}

		free(first);
		free(middle);
	}

	printf("检查模型 %d 个，%d 个在两帧之间有位移\n", models, animated);

	hive_close(hive);
	return ok && models > 0 && animated > 0 ? 0 : 1;
}
