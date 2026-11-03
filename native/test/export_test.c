#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hive_c.h"

static long file_size(const char* path) {
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		return -1;
	}
	fseek(file, 0, SEEK_END);
	const long size = ftell(file);
	fclose(file);
	return size;
}

static int has_mpq_magic(const char* path, long offset) {
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		return 0;
	}
	char magic[4] = {0};
	fseek(file, offset, SEEK_SET);
	const size_t read = fread(magic, 1, 4, file);
	fclose(file);
	return read == 4 && memcmp(magic, "MPQ\x1a", 4) == 0;
}

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	const char* plain_path = "/tmp/hivewe_export_plain.w3m";
	const char* minified_path = "/tmp/hivewe_export_minified.w3m";
	const char* guarded_path = "/tmp/hivewe_export_guarded.w3c";

	if (hive_export_map(hive, plain_path, NULL, error, sizeof(error)) != 0) {
		printf("原样导出失败：%s\n", error);
		return 1;
	}

	HiveProtection protect = {0};
	protect.obfuscate_script = 1;
	if (hive_export_map(hive, minified_path, &protect, error, sizeof(error)) != 0) {
		printf("压缩脚本导出失败：%s\n", error);
		return 1;
	}

	HiveProtection full = {1, 1, 1, 1, 1};
	if (hive_export_map(hive, guarded_path, &full, error, sizeof(error)) != 0) {
		printf("保护导出失败：%s\n", error);
		return 1;
	}

	const long plain = file_size(plain_path);
	const long minified = file_size(minified_path);
	const long guarded = file_size(guarded_path);
	printf("导出大小：原样 %ld，压缩脚本 %ld，全开 %ld\n", plain, minified, guarded);

	const int plain_magic = has_mpq_magic(plain_path, 0);
	const int guarded_magic_at_zero = has_mpq_magic(guarded_path, 0);
	const int guarded_magic_at_block = has_mpq_magic(guarded_path, 512);
	printf("文件头：原样 offset 0 %s，全开 offset 0 %s、offset 512 %s\n", plain_magic ? "是 MPQ" : "不是",
		   guarded_magic_at_zero ? "是 MPQ" : "不是", guarded_magic_at_block ? "是 MPQ" : "不是");

	hive_close(hive);

	void* again = hive_open(plain_path, "testdata/warcraft", error, sizeof(error));
	printf("原样导出能重新打开：%s\n", again != NULL ? "能" : "不能");
	if (again != NULL) {
		HiveMapInfo info;
		hive_map_info(again, &info);
		printf("重开后 %dx%d，装饰物 %d，单位 %d\n", info.width, info.height, info.doodad_count, info.unit_count);
		hive_close(again);
	}

	const int ok = plain > 0 && minified > 0 && minified < plain && guarded > 0 && plain_magic
		&& !guarded_magic_at_zero && guarded_magic_at_block && again != NULL;
	printf("%s\n", ok ? "导出与保护选项生效" : "导出结果不符");

	return ok ? 0 : 1;
}
