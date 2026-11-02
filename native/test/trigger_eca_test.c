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

	printf("事件函数 %d 个，条件 %d 个，动作 %d 个\n", hive_trigger_function_count(hive, 0),
		   hive_trigger_function_count(hive, 1), hive_trigger_function_count(hive, 2));
	for (int i = 0; i < 4; ++i) {
		printf("  动作 %s\n", hive_trigger_function_name(hive, 2, i));
	}

	printf("unitcode 预设 %d 个，前两个：%s / %s\n", hive_trigger_preset_count(hive, "unitcode"),
		   hive_trigger_preset_display(hive, "unitcode", 0), hive_trigger_preset_display(hive, "unitcode", 1));
	printf("脚本代码类型预设 %d 个\n", hive_trigger_preset_count(hive, "scriptcode"));

	if (hive_trigger_count(hive) == 0) {
		printf("没有触发器\n");
		return 1;
	}

	const int before = hive_trigger_eca_count(hive, 0, 2);
	const int added = hive_trigger_add_eca(hive, 0, 2, "CommentString");
	printf("加动作后 %d -> %d，新动作序号 %d，名字 %s，参数 %d 个\n", before, hive_trigger_eca_count(hive, 0, 2), added,
		   added >= 0 ? hive_trigger_eca_name(hive, 0, 2, added) : "(无)",
		   added >= 0 ? hive_trigger_eca_parameter_count(hive, 0, 2, added) : 0);

	if (added >= 0) {
		printf("参数类型 %s\n", hive_trigger_eca_parameter_type_name(hive, 0, 2, added, 0));

		if (hive_set_trigger_eca_parameter_value(hive, 0, 2, added, 0, "测试注释") != 0) {
			printf("写参数失败\n");
			return 1;
		}
		printf("参数值 %s（类型 %d）\n", hive_trigger_eca_parameter_value(hive, 0, 2, added, 0),
			   hive_trigger_eca_parameter_type(hive, 0, 2, added, 0));

		printf("删除动作 %d，剩余 %d 个\n", hive_trigger_remove_eca(hive, 0, 2, added),
			   hive_trigger_eca_count(hive, 0, 2));
	}

	const int ok = before == 8 && added >= 0 && hive_trigger_eca_count(hive, 0, 2) == before;
	hive_close(hive);
	return ok ? 0 : 1;
}
