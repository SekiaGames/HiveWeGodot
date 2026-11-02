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

	const int fields = hive_object_field_count(hive, 0, "hpea");
	printf("hpea 字段 %d 个\n", fields);
	for (int i = 0; i < 5 && i < fields; ++i) {
		printf("  %s\t%s = %s\n", hive_object_field_key(hive, 0, i), hive_object_field_label(hive, 0, i),
			   hive_object_value(hive, 0, "hpea", i));
	}

	int hp_index = -1;
	for (int i = 0; i < fields; ++i) {
		const char* key = hive_object_field_key(hive, 0, i);
		if (key != NULL && strcmp(key, "uhpm") == 0) {
			hp_index = i;
			break;
		}
	}
	if (hp_index < 0) {
		printf("没找到 uhpm 字段\n");
		return 1;
	}

	printf("生命值 %s（type %s），改过 %d\n", hive_object_value(hive, 0, "hpea", hp_index),
		   hive_object_field_type(hive, 0, hp_index), hive_object_value_modified(hive, 0, "hpea", hp_index));

	if (hive_set_object_value(hive, 0, "hpea", hp_index, "1234") != 0) {
		printf("写入失败\n");
		return 1;
	}
	printf("改后 %s，改过 %d\n", hive_object_value(hive, 0, "hpea", hp_index),
		   hive_object_value_modified(hive, 0, "hpea", hp_index));

	const int doodad_category = hive_doodad_category(hive, "LTlt");
	printf("LTlt 属于分类 %d\n", doodad_category);

	const int doodad_fields = hive_object_field_count(hive, doodad_category, "LTlt");
	printf("装饰物 LTlt 字段 %d 个\n", doodad_fields);
	for (int i = 0; i < 3 && i < doodad_fields; ++i) {
		printf("  %s\t%s = %s\n", hive_object_field_key(hive, doodad_category, i),
			   hive_object_field_label(hive, doodad_category, i), hive_object_value(hive, doodad_category, "LTlt", i));
	}

	if (hive_save_map(hive, "/tmp/edited_object.w3m", error, sizeof(error)) != 0) {
		printf("保存失败：%s\n", error);
		return 1;
	}
	hive_close(hive);

	void* reopened = hive_open("/tmp/edited_object.w3m", "testdata/warcraft", error, sizeof(error));
	if (reopened == NULL) {
		printf("重开失败：%s\n", error);
		return 1;
	}

	const int reopened_fields = hive_object_field_count(reopened, 0, "hpea");
	int reopened_hp = -1;
	for (int i = 0; i < reopened_fields; ++i) {
		const char* key = hive_object_field_key(reopened, 0, i);
		if (key != NULL && strcmp(key, "uhpm") == 0) {
			reopened_hp = i;
			break;
		}
	}

	const char* value = reopened_hp >= 0 ? hive_object_value(reopened, 0, "hpea", reopened_hp) : NULL;
	printf("重开后生命值 %s，改过 %d\n", value != NULL ? value : "(无)",
		   reopened_hp >= 0 ? hive_object_value_modified(reopened, 0, "hpea", reopened_hp) : 0);

	const int ok = value != NULL && strcmp(value, "1234") == 0;
	hive_close(reopened);
	return ok ? 0 : 1;
}
