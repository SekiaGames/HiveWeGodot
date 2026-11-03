#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hive_c.h"

static int has(const char* const* list, const int count, const char* name) {
	const size_t length = strlen(name);
	for (int i = 0; i < count; ++i) {
		if (strncmp(list[i], name, length) == 0) {
			return 1;
		}
	}
	return 0;
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	const int count = hive_jass_api_count(hive);
	if (count <= 0) {
		printf("没有补全条目\n");
		return 1;
	}

	const char** list = calloc((size_t)count, sizeof(char*));
	int functions = 0;
	int with_parameters = 0;
	for (int i = 0; i < count; ++i) {
		list[i] = hive_jass_api(hive, i);
		if (list[i] == NULL) {
			printf("第 %d 条取不到\n", i);
			return 1;
		}
		if (strchr(list[i], '(') != NULL) {
			functions++;
			if (strcmp(list[i] + strlen(list[i]) - 2, "()") != 0) {
				with_parameters++;
			}
		}
	}

	printf("补全条目 %d 条，其中函数 %d 条（%d 条带参数）\n", count, functions, with_parameters);
	printf("CreateUnit：%s\n", has(list, count, "CreateUnit(") ? "有" : "没有");
	printf("GetTriggeringTrigger：%s\n", has(list, count, "GetTriggeringTrigger(") ? "有" : "没有");
	printf("unit 类型：%s\n", has(list, count, "unit") ? "有" : "没有");
	printf("bj_MAX_PLAYERS：%s\n", has(list, count, "bj_MAX_PLAYERS") ? "有" : "没有");

	const int ok = count > 1000 && functions > 500 && with_parameters > 500 && has(list, count, "CreateUnit(")
		&& has(list, count, "GetTriggeringTrigger(") && has(list, count, "unit") && has(list, count, "bj_MAX_PLAYERS");
	printf("%s\n", ok ? "JASS 补全条目齐备" : "JASS 补全条目不全");

	free(list);
	hive_close(hive);
	return ok ? 0 : 1;
}
