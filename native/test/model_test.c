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

	HiveMapInfo info;
	hive_map_info(hive, &info);

	HiveDoodad* doodads = calloc(info.doodad_count, sizeof(HiveDoodad));
	const int count = hive_copy_doodads(hive, doodads, info.doodad_count);
	printf("装饰物 %d 个\n", count);

	int loaded = 0;
	int missing = 0;
	for (int i = 0; i < count && i < 40; ++i) {
		const char* id = hive_doodad_id(hive, i);
		const int model = hive_model_from_doodad(hive, i);
		if (model < 0) {
			missing++;
			continue;
		}
		loaded++;

		if (loaded <= 4) {
			HiveModelInfo model_info;
			hive_model_info(hive, model, &model_info);
			printf("  %s：%d geoset，%d 顶点，%d 索引，%.1f×%.1f×%.1f\n",
				   id ? id : "?",
				   model_info.geoset_count,
				   model_info.vertex_count,
				   model_info.index_count,
				   model_info.max_x - model_info.min_x,
				   model_info.max_y - model_info.min_y,
				   model_info.max_z - model_info.min_z);

			for (int g = 0; g < model_info.geoset_count; ++g) {
				const char* texture = hive_model_geoset_texture(hive, model, g);
				printf("    贴图 %s\n", texture ? texture : "（无）");
				if (texture != NULL) {
					int width = 0;
					int height = 0;
					if (hive_image_info_path(hive, texture, &width, &height) == 0) {
						printf("      解码 %d×%d\n", width, height);
					} else {
						printf("      解码失败\n");
					}
				}
			}
		}
	}

	printf("前 40 个里，模型加载成功 %d，失败 %d\n", loaded, missing);

	const int model = hive_model_from_doodad(hive, 0);
	if (model >= 0) {
		HiveModelInfo model_info;
		hive_model_info(hive, model, &model_info);

		float* positions = calloc(model_info.vertex_count * 3, sizeof(float));
		float* normals = calloc(model_info.vertex_count * 3, sizeof(float));
		float* uvs = calloc(model_info.vertex_count * 2, sizeof(float));
		uint32_t* indices = calloc(model_info.index_count, sizeof(uint32_t));

		hive_copy_model_positions(hive, model, positions, model_info.vertex_count * 3);
		hive_copy_model_normals(hive, model, normals, model_info.vertex_count * 3);
		hive_copy_model_uvs(hive, model, uvs, model_info.vertex_count * 2);

		HiveGeosetInfo geoset;
		hive_model_geoset(hive, model, 0, &geoset);
		hive_copy_model_indices(hive, model, 0, indices, geoset.index_count);

		printf("首个顶点 (%.3f, %.3f, %.3f)，法线 (%.2f, %.2f, %.2f)，UV (%.2f, %.2f)\n",
			   positions[0], positions[1], positions[2],
			   normals[0], normals[1], normals[2],
			   uvs[0], uvs[1]);
		printf("首三个索引 %u %u %u\n", indices[0], indices[1], indices[2]);

		free(indices);
		free(uvs);
		free(normals);
		free(positions);
	}

	free(doodads);
	hive_close(hive);
	return 0;
}
