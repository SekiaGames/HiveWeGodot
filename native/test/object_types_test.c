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

	const char* names[] = {"单位", "物品", "装饰物", "可破坏物", "技能", "升级", "buff"};
	int counts[7] = {0};
	int ok = 1;
	for (int category = 0; category < 7; ++category) {
		counts[category] = hive_object_type_count(hive, category);
		if (counts[category] <= 0) {
			printf("%s 一个都没有\n", names[category]);
			ok = 0;
			continue;
		}

		const char* id = hive_object_type_id(hive, category, counts[category] - 1);
		if (id == NULL || strlen(id) != 4) {
			printf("%s 的最后一个 id 不对：%s\n", names[category], id == NULL ? "(空)" : id);
			ok = 0;
		}
	}

	printf("单位 %d、物品 %d、装饰物 %d、可破坏物 %d、技能 %d、升级 %d、buff %d\n", counts[0], counts[1],
		   counts[2], counts[3], counts[4], counts[5], counts[6]);

	const int total = counts[0] + counts[1] + counts[2] + counts[3] + counts[4] + counts[5] + counts[6];
	printf("共 %d 个对象\n", total);

	if (hive_object_type_count(hive, 7) != 0) {
		printf("越界的分类应该有 0 个，实际 %d\n", hive_object_type_count(hive, 7));
		ok = 0;
	}
	if (hive_object_type_id(hive, 0, -1) != NULL) {
		printf("负下标应该返回空\n");
		ok = 0;
	}

	// 单位那套旧接口要和通用接口说同一件事
	if (hive_unit_type_count(hive) != counts[0]) {
		printf("单位数量对不上：%d vs %d\n", hive_unit_type_count(hive), counts[0]);
		ok = 0;
	}

	printf("%s\n", ok ? "七类对象都能枚举" : "对象枚举有问题");
	hive_close(hive);
	return ok ? 0 : 1;
}
