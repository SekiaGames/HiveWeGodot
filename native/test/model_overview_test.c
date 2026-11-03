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

	const int model = hive_model_from_path(hive, "Units/creeps/TimberWolf/TimberWolf.mdx");
	if (model < 0) {
		printf("打不开模型\n");
		return 1;
	}

	HiveModelInfo info;
	hive_model_info(hive, model, &info);
	const int materials = hive_model_material_count(hive, model);
	const int textures = hive_model_texture_count(hive, model);
	const int nodes = hive_model_node_count(hive, model);
	const int sequences = hive_model_sequence_count(hive, model);
	printf("模型 %d：%d 层 %d 顶点，%d 材质 %d 贴图 %d 节点 %d 序列\n", model, info.geoset_count, info.vertex_count,
		   materials, textures, nodes, sequences);

	if (materials <= 0 || textures <= 0 || nodes <= 0 || sequences <= 0 || info.geoset_count <= 0) {
		printf("有一项是空的\n");
		return 1;
	}

	// 几何体表格要拿到的东西：面数、贴图、双面、alpha
	int faces = 0;
	int with_texture = 0;
	for (int g = 0; g < info.geoset_count; ++g) {
		HiveGeosetInfo geoset;
		hive_model_geoset(hive, model, g, &geoset);
		faces += geoset.index_count / 3;
		const char* texture = hive_model_geoset_texture(hive, model, g);
		if (texture != NULL && texture[0] != 0) {
			with_texture++;
		}
	}

	printf("几何体合计 %d 个三角面，%d 层有贴图\n", faces, with_texture);

	// 概览的包围盒要可用
	const int box_ok = info.max_x > info.min_x && info.max_z > info.min_z && info.max_y > info.min_y;
	printf("包围盒 x %.1f..%.1f y %.1f..%.1f z %.1f..%.1f\n", info.min_x, info.max_x, info.min_y, info.max_y,
		   info.min_z, info.max_z);

	const int ok = faces > 0 && with_texture > 0 && box_ok;
	printf("%s\n", ok ? "模型概览与几何体表数据齐备" : "模型概览数据不全");

	hive_close(hive);
	return ok ? 0 : 1;
}
