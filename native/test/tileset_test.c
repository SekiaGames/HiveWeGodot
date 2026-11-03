#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hive_c.h"

int main(void) {
	char error[512] = {0};
	void* hive = hive_open("testdata/maps/(2)BootyBay.w3m", "testdata/warcraft", error, sizeof(error));
	if (hive == NULL) {
		printf("打开失败：%s\n", error);
		return 1;
	}

	const int count = hive_tileset_texture_count(hive, "L");
	if (count <= 0) {
		printf("地形集 L 里没有瓦片\n");
		return 1;
	}

	int cliffs = 0;
	int named = 0;
	for (int i = 0; i < count; ++i) {
		const char* id = hive_tileset_texture_id(hive, "L", i);
		const char* name = hive_tileset_texture_name(hive, "L", i);
		if (id == NULL || strlen(id) != 4) {
			printf("第 %d 块 id 不对\n", i);
			return 1;
		}
		if (name != NULL && name[0] != 0) {
			named++;
		}
		if (hive_tileset_texture_cliff(hive, "L", i)) {
			cliffs++;
		}
	}
	printf("地形集 L：%d 块瓦片，%d 块有名字，%d 块是悬崖\n", count, named, cliffs);

	uint8_t* pixels = malloc(512 * 512 * 4);
	const char* first = hive_tileset_texture_id(hive, "L", 0);
	int size = 0;
	const int bytes = hive_terrain_tile_image(hive, first, pixels, 512 * 512 * 4, &size);
	printf("瓦片 %s 的图：%d×%d，%d 字节\n", first, size, size, bytes);
	if (bytes <= 0 || size <= 0 || bytes != size * size * 4) {
		printf("缩略图不对\n");
		return 1;
	}

	HiveMapInfo before;
	hive_map_info(hive, &before);

	char ids[64][5];
	char* pointers[64];
	int old_count = 0;
	// index 等于纹理数时返回的是 blight，不在可换的列表里
	for (int i = 0; i < before.texture_count - 1 && i < 64; ++i) {
		const char* id = hive_texture_id(hive, i);
		if (id == NULL) {
			break;
		}
		snprintf(ids[old_count], sizeof(ids[old_count]), "%s", id);
		pointers[old_count] = ids[old_count];
		old_count++;
	}

	// 去掉第一块，其余保持顺序，旧索引按新位置重映射
	const int new_count = old_count - 1;
	int mapping[64];
	for (int i = 0; i < old_count; ++i) {
		mapping[i] = i == 0 ? 0 : i - 1;
	}
	if (hive_change_tileset(hive, "L", (const char**)&pointers[1], new_count, mapping, error, sizeof(error)) != 0) {
		printf("换地形集失败：%s\n", error);
		return 1;
	}

	HiveMapInfo after;
	hive_map_info(hive, &after);
	printf("换之前 %d 块纹理，换之后 %d 块，少的那块是 %s\n", before.texture_count, after.texture_count, ids[0]);
	printf("换之后第一块是 %s，期望 %s\n", hive_texture_id(hive, 0), ids[1]);

	const int ok = named == count && cliffs > 0 && after.texture_count == before.texture_count - 1
		&& strcmp(hive_texture_id(hive, 0), ids[1]) == 0;
	printf("%s\n", ok ? "地形集读取与切换正常" : "地形集结果不符");

	free(pixels);
	hive_close(hive);
	return ok ? 0 : 1;
}
