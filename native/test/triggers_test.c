#include <stdio.h>
#include <string.h>

#include "hive_c.h"

int main(int argc, char** argv) {
	const char* map_path = argc > 1 ? argv[1] : "testdata/maps/(2)BootyBay.w3m";
	char error[512] = {0};
	void* hive = hive_open(map_path, "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	const int categories = hive_trigger_category_count(hive);
	const int triggers = hive_trigger_count(hive);
	const int variables = hive_trigger_variable_count(hive);
	printf("分类 %d，触发器 %d，变量 %d\n", categories, triggers, variables);

	for (int i = 0; i < 4 && i < categories; ++i) {
		printf("  分类 %d [%s] 父 %d\n", hive_trigger_category_id(hive, i), hive_trigger_category_name(hive, i),
			   hive_trigger_category_parent(hive, i));
	}

	for (int i = 0; i < 6 && i < triggers; ++i) {
		printf("  触发器 %d [%s] 父 %d，事件 %d 条件 %d 动作 %d，注释 %d，启用 %d\n", hive_trigger_id(hive, i),
			   hive_trigger_name(hive, i), hive_trigger_parent(hive, i), hive_trigger_eca_count(hive, i, 0),
			   hive_trigger_eca_count(hive, i, 1), hive_trigger_eca_count(hive, i, 2), hive_trigger_is_comment(hive, i),
			   hive_trigger_is_enabled(hive, i));
	}

	for (int i = 0; i < 2 && i < variables; ++i) {
		printf("  变量 [%s] 类型 %s 数组 %d 初值 [%s]\n", hive_trigger_variable_name(hive, i),
			   hive_trigger_variable_type(hive, i), hive_trigger_variable_is_array(hive, i),
			   hive_trigger_variable_initial(hive, i));
	}

	const char* script = hive_map_global_script(hive);
	printf("全局脚本 %zu 字节\n", script != NULL ? strlen(script) : 0);

	const int created = hive_create_trigger(hive, "新建测试触发器", -2);
	printf("新建触发器索引 %d，总数 %d\n", created, hive_trigger_count(hive));
	if (created >= 0) {
		printf("新触发器 [%s] 父 %d\n", hive_trigger_name(hive, created), hive_trigger_parent(hive, created));
		printf("删除新触发器 %d，总数 %d\n", hive_delete_trigger(hive, created), hive_trigger_count(hive));
	}

	if (triggers > 0) {
		char original[128] = {0};
		strncpy(original, hive_trigger_name(hive, 0), sizeof(original) - 1);

		const char renamed[] = "改过的触发器";
		if (hive_set_trigger_name(hive, 0, renamed) != 0) {
			printf("改名失败\n");
			return 1;
		}
		printf("改名 [%s] -> [%s]\n", original, hive_trigger_name(hive, 0));

		if (hive_save_map(hive, "/tmp/edited_triggers.w3m", error, sizeof(error)) != 0) {
			printf("保存失败：%s\n", error);
			return 1;
		}
		hive_close(hive);

		void* reopened = hive_open("/tmp/edited_triggers.w3m", "testdata/warcraft", error, sizeof(error));
		if (reopened == NULL) {
			printf("重开失败：%s\n", error);
			return 1;
		}

		char reopened_name[128] = {0};
		strncpy(reopened_name, hive_trigger_name(reopened, 0), sizeof(reopened_name) - 1);
		printf("重开后第一个触发器 [%s]，总数 %d\n", reopened_name, hive_trigger_count(reopened));

		const int ok = strcmp(reopened_name, renamed) == 0 && hive_trigger_count(reopened) == triggers;
		hive_close(reopened);
		return ok ? 0 : 1;
	}

	hive_close(hive);
	return 0;
}
