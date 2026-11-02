#include <stdio.h>
#include <string.h>

#include "hive_c.h"

static int field_index(void* hive, int category, const char* id, const char* key) {
	const int count = hive_object_field_count(hive, category, id);
	for (int i = 0; i < count; ++i) {
		const char* name = hive_object_field_key(hive, category, i);
		if (name != NULL && strcmp(name, key) == 0) {
			return i;
		}
	}
	return -1;
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	const char* generated = hive_unique_object_id(hive, "hpea");
	char new_id[8] = {0};
	if (generated == NULL) {
		printf("生成 id 失败\n");
		return 1;
	}
	strncpy(new_id, generated, sizeof(new_id) - 1);
	printf("生成的自定义 id %s\n", new_id);

	if (hive_create_object(hive, 0, "hpea", new_id) != 0) {
		printf("复制对象失败\n");
		return 1;
	}

	const int source_fields = hive_object_field_count(hive, 0, "hpea");
	const int custom_fields = hive_object_field_count(hive, 0, new_id);
	printf("hpea 字段 %d，%s 字段 %d，是否自定义 %d\n", source_fields, new_id, custom_fields,
		   hive_object_is_custom(hive, 0, new_id));

	const int custom_hp = field_index(hive, 0, new_id, "uhpm");
	const int source_hp = field_index(hive, 0, "hpea", "uhpm");
	if (custom_hp < 0 || source_hp < 0) {
		printf("没找到 uhpm\n");
		return 1;
	}

	const int set_result = hive_set_object_value(hive, 0, new_id, custom_hp, "999");
	const int custom_modified = hive_object_value_modified(hive, 0, new_id, custom_hp);

	char custom_text[32] = {0};
	char source_text[32] = {0};
	strncpy(custom_text, hive_object_value(hive, 0, new_id, custom_hp), sizeof(custom_text) - 1);
	strncpy(source_text, hive_object_value(hive, 0, "hpea", source_hp), sizeof(source_text) - 1);
	printf("写入返回 %d，改过 %d，自定义生命 %s，原型生命 %s\n", set_result, custom_modified, custom_text,
		   source_text);

	if (hive_save_map(hive, "/tmp/edited_custom.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* reopened = hive_open("/tmp/edited_custom.w3m", "testdata/warcraft", error, sizeof(error));
	if (reopened == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}

	const int reopened_fields = hive_object_field_count(reopened, 0, new_id);
	const int reopened_hp = field_index(reopened, 0, new_id, "uhpm");
	const int base_hp = field_index(reopened, 0, "hpea", "uhpm");

	char reopened_custom[32] = {0};
	char reopened_base[32] = {0};
	if (reopened_hp >= 0) {
		strncpy(reopened_custom, hive_object_value(reopened, 0, new_id, reopened_hp), sizeof(reopened_custom) - 1);
	}
	if (base_hp >= 0) {
		strncpy(reopened_base, hive_object_value(reopened, 0, "hpea", base_hp), sizeof(reopened_base) - 1);
	}

	const int custom_flag = hive_object_is_custom(reopened, 0, new_id);
	printf("重开后 %s 字段 %d，生命 %s，原型生命 %s，是否自定义 %d\n", new_id, reopened_fields, reopened_custom,
		   reopened_base, custom_flag);

	const int deleted = hive_delete_object(reopened, 0, new_id);
	printf("删除自定义对象 %s，之后是否还在 %d\n", deleted == 0 ? "成功" : "失败",
		   hive_object_field_count(reopened, 0, new_id));

	const int ok = custom_fields == source_fields && strcmp(reopened_custom, "999") == 0
		&& strcmp(reopened_base, "220") == 0 && custom_flag == 1 && deleted == 0;

	hive_close(reopened);
	return ok ? 0 : 1;
}
