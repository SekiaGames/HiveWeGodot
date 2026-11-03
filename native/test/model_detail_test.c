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

	const int materials = hive_model_material_count(hive, model);
	const int textures = hive_model_texture_count(hive, model);

	int layers = 0;
	int layer_textures = 0;
	for (int m = 0; m < materials; ++m) {
		HiveMaterialInfo material;
		if (hive_model_material(hive, model, m, &material) != 0) {
			printf("材质 %d 取不到\n", m);
			return 1;
		}
		for (int l = 0; l < material.layer_count; ++l) {
			HiveLayerInfo layer;
			if (hive_model_layer(hive, model, m, l, &layer) != 0) {
				printf("材质 %d 的层 %d 取不到\n", m, l);
				return 1;
			}
			layers++;
			for (int t = 0; t < layer.texture_count; ++t) {
				int slot = -1;
				int texture = -1;
				if (hive_model_layer_texture(hive, model, m, l, t, &slot, &texture) != 0) {
					printf("材质 %d 层 %d 的贴图 %d 取不到\n", m, l, t);
					return 1;
				}
				if (texture < 0 || texture >= textures) {
					printf("贴图索引越界：%d\n", texture);
					return 1;
				}
				layer_textures++;
			}
		}
	}

	int named = 0;
	int sized = 0;
	for (int t = 0; t < textures; ++t) {
		const char* name = hive_model_texture_name(hive, model, t);
		if (name != NULL && name[0] != 0) {
			named++;
		}
		HiveTextureInfo texture;
		if (hive_model_texture(hive, model, t, &texture) != 0) {
			printf("贴图 %d 取不到\n", t);
			return 1;
		}
		if (texture.width > 0 && texture.height > 0) {
			sized++;
		}
	}

	const int nodes = hive_model_node_count(hive, model);
	int roots = 0;
	int with_keys = 0;
	int unnamed = 0;
	for (int n = 0; n < nodes; ++n) {
		HiveNodeInfo node;
		if (hive_model_node(hive, model, n, &node) != 0) {
			printf("节点 %d 取不到\n", n);
			return 1;
		}
		if (node.parent_id < 0) {
			roots++;
		}
		if (node.translation_keys + node.rotation_keys + node.scaling_keys > 0) {
			with_keys++;
		}
		const char* name = hive_model_node_name(hive, model, n);
		if (name == NULL || name[0] == 0) {
			unnamed++;
		}
	}

	printf("材质 %d（%d 层 %d 贴图槽），贴图 %d（%d 有名字 %d 有尺寸），节点 %d（%d 个根 %d 有关键帧 %d 无名）\n",
		   materials, layers, layer_textures, textures, named, sized, nodes, roots, with_keys, unnamed);

	if (hive_model_material(hive, model, materials, NULL) != -1) {
		printf("越界材质应当失败\n");
		return 1;
	}
	if (hive_model_node_name(hive, model, nodes) != NULL) {
		printf("越界节点应当失败\n");
		return 1;
	}

	const int ok = layers > 0 && layer_textures > 0 && named == textures && sized > 0 && nodes > 0
		&& roots > 0 && with_keys > 0;
	printf("%s\n", ok ? "材质、贴图与节点明细齐备" : "模型明细数据不全");

	hive_close(hive);
	return ok ? 0 : 1;
}
