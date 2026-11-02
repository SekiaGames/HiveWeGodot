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
	printf("unitcode 预设 %d 个，前两个：%s / %s\n", hive_trigger_preset_count(hive, "unitcode"),
		   hive_trigger_preset_display(hive, "unitcode", 0), hive_trigger_preset_display(hive, "unitcode", 1));

	if (hive_trigger_count(hive) == 0) {
		printf("没有触发器\n");
		return 1;
	}

	const int nodes = hive_trigger_build_nodes(hive, 0);
	printf("节点 %d 个\n", nodes);
	for (int i = 0; i < nodes; ++i) {
		printf("  [%d] type %d depth %d group %d  %s\n", i, hive_trigger_node_type(hive, 0, i),
			   hive_trigger_node_depth(hive, 0, i), hive_trigger_node_group(hive, 0, i),
			   hive_trigger_node_display(hive, 0, i));
	}

	int action = -1;
	for (int i = 0; i < nodes; ++i) {
		if (hive_trigger_node_type(hive, 0, i) == 2 && hive_trigger_node_depth(hive, 0, i) == 0) {
			action = i;
			break;
		}
	}
	if (action < 0) {
		printf("没有动作节点\n");
		return 1;
	}

	const int parameters = hive_trigger_node_parameter_count(hive, 0, action);
	printf("第一个动作 %s，参数 %d 个", hive_trigger_node_name(hive, 0, action), parameters);
	for (int i = 0; i < parameters; ++i) {
		printf(" · %s=%s", hive_trigger_node_parameter_type_name(hive, 0, action, i),
			   hive_trigger_node_parameter_value(hive, 0, action, i));
	}
	printf("\n");

	const int added = hive_trigger_add_node(hive, 0, 2, "CommentString", -1, 0);
	const int after = hive_trigger_build_nodes(hive, 0);
	printf("加动作后 %d -> %d 个节点，新节点是最后一条：%s\n", nodes, after,
		   hive_trigger_node_display(hive, 0, after - 1));

	const int comment = after - 1;
	const int comment_parameters = hive_trigger_node_parameter_count(hive, 0, comment);
	printf("注释参数 %d 个，类型 %s\n", comment_parameters,
		   hive_trigger_node_parameter_type_name(hive, 0, comment, 0));
	if (hive_set_trigger_node_parameter(hive, 0, comment, 0, "测试注释") != 0) {
		printf("写参数失败\n");
		return 1;
	}
	printf("写入后显示：%s\n", hive_trigger_node_display(hive, 0, comment));

	printf("删除节点返回 %d\n", hive_trigger_remove_node(hive, 0, comment));

	const int before_if = hive_trigger_build_nodes(hive, 0);
	if (hive_trigger_add_node(hive, 0, 2, "IfThenElseMultiple", -1, 0) != 0) {
		printf("加 If/Then/Else 失败\n");
		return 1;
	}
	int if_node = -1;
	for (int i = 0; i < hive_trigger_build_nodes(hive, 0); ++i) {
		if (strcmp(hive_trigger_node_name(hive, 0, i), "IfThenElseMultiple") == 0) {
			if_node = i;
		}
	}
	printf("If/Then/Else 节点 %d，分组种类 %d\n", if_node,
		   if_node >= 0 ? hive_trigger_node_group_kind(hive, 0, if_node) : -1);

	if (if_node >= 0 && hive_trigger_add_node(hive, 0, 2, "CommentString", if_node, 1) == 0) {
		const int nested = hive_trigger_build_nodes(hive, 0);
		const int child = if_node + 1;
		printf("加进 Then 后 %d -> %d 个节点，子节点 depth %d group %d 显示 %s\n", before_if, nested,
			   hive_trigger_node_depth(hive, 0, child), hive_trigger_node_group(hive, 0, child),
			   hive_trigger_node_display(hive, 0, child));
	}

	const int ok = added >= 0 && comment_parameters == 1 && if_node >= 0
		&& hive_trigger_node_group_kind(hive, 0, if_node) == 3
		&& strstr(hive_trigger_node_display(hive, 0, action), "Melee") != NULL;
	hive_close(hive);
	return ok ? 0 : 1;
}
