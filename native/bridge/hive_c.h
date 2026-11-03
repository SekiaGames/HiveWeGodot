#ifndef HIVE_C_H
#define HIVE_C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HiveMapInfo {
	int32_t width;
	int32_t height;
	int32_t tiles_x;
	int32_t tiles_y;
	int32_t texture_count;
	int32_t doodad_count;
	int32_t unit_count;
	int32_t region_count;
	int32_t tileset_id;
} HiveMapInfo;

/* 原样对应 HiveWE 的 uvec4：每项低 16 位是纹理索引，高 16 位是额外信息。
   第 0 项的高 16 位是变体号，其余项的高 16 位是该纹理出现在哪几个角的掩码，
   0xFFFF 表示这一项为空。 */
typedef struct HiveTile {
	uint32_t texture[4];
	uint32_t variation;
} HiveTile;

/* 打开地图。data_dir 指向装了客户端的目录（含 WAR3.MPQ），或从 MPQ 里解包出来的资源目录。
   失败返回 NULL 并把原因写进 error。 */
void* hive_open(const char* map_path, const char* data_dir, char* error, int32_t error_size);

void hive_close(void* handle);

/* 把当前地图写回 target_dir。成功返回 0。 */
int32_t hive_save(void* handle, const char* target_dir, char* error, int32_t error_size);

/* 把当前地图打成 w3m / w3x 写到 map_path。成功返回 0。 */
int32_t hive_save_map(void* handle, const char* map_path, char* error, int32_t error_size);

int32_t hive_map_info(void* handle, HiveMapInfo* out);

/* 拷贝地形数据，返回实际元素数。capacity 不够时只写 capacity 个。 */
int32_t hive_copy_heights(void* handle, float* out, int32_t capacity);
int32_t hive_copy_tiles(void* handle, HiveTile* out, int32_t capacity);
int32_t hive_copy_ground_exists(void* handle, uint8_t* out, int32_t capacity);
int32_t hive_copy_water_exists(void* handle, uint8_t* out, int32_t capacity);
/* 每个顶点的水面高度，不含全局水偏移。 */
int32_t hive_copy_water_heights(void* handle, float* out, int32_t capacity);
/* 实际水面高度 = 顶点水面高度 + 这个偏移。 */
float hive_water_offset(void* handle);

/* 第 index 个地面纹理的 id（四字符，如 Adrt），越界返回 NULL。index 等于纹理数时返回 "blight"。 */
const char* hive_texture_id(void* handle, int32_t index);

/* 地面纹理已经被切成 tile 网格：width = height = 单块边长，channels = 层数（16 或 32），
   每层是 width×width 的 RGBA8。变体号就是层号。成功返回 0。 */
int32_t hive_texture_info(void* handle, int32_t index, int32_t* width, int32_t* height, int32_t* channels);
int32_t hive_copy_texture(void* handle, int32_t index, uint8_t* out, int32_t capacity);

/* 区域。坐标是 tile，left/right/top/bottom 已归一化成 left<right、bottom<top。
   颜色是 0..255 的 RGB。 */
typedef struct HiveRegion {
	float left;
	float right;
	float bottom;
	float top;
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} HiveRegion;

/* w3i 里的玩家槽位数，以及该槽位的出生点（原始坐标）。 */
int32_t hive_player_count(void* handle);
int32_t hive_player_start(void* handle, int32_t slot, float* x, float* y);

/* 开始点就是 war3mapUnits.doo 里的 sloc 单位，一个玩家一个。 */
int32_t hive_start_location_count(void* handle);
int32_t hive_start_location(void* handle, int32_t index, int32_t* player, float* x, float* y);
/* 已有就挪位置，没有就新建一个；同时改 w3i 里该玩家的出生点。 */
int32_t hive_set_start_location(void* handle, int32_t player, float x, float y);
int32_t hive_remove_start_location(void* handle, int32_t player);

int32_t hive_copy_regions(void* handle, HiveRegion* out, int32_t capacity);
const char* hive_region_name(void* handle, int32_t index);
/* 新建区域，坐标是 tile，内部归一化。名字和颜色按 HiveWE 的规则自动取。 */
int32_t hive_create_region(void* handle, float left, float bottom, float right, float top);
int32_t hive_delete_region(void* handle, int32_t index);
int32_t hive_set_region_rect(void* handle, int32_t index, float left, float bottom, float right, float top);
int32_t hive_set_region_name(void* handle, int32_t index, const char* value);

/* 路径图。宽高是 tile 数 × 4，每格一个字节，位标志见 HivePathing。
   1 不可走，2 不可飞，4 不可建，8 可采集，16 枯萎，32 水，64 两栖。 */
int32_t hive_pathing_info(void* handle, int32_t* width, int32_t* height);
int32_t hive_copy_pathing(void* handle, uint8_t* out, int32_t capacity);

/* 小地图，尺寸同顶点网格，RGBA8，自上而下。 */
int32_t hive_minimap_info(void* handle, int32_t* width, int32_t* height);
int32_t hive_copy_minimap(void* handle, uint8_t* out, int32_t capacity);

/* 地图文件所在目录，即解包后的临时目录。 */
const char* hive_map_directory(void* handle);

/* 笔刷位置，地形 tile 坐标，笔刷中心落在 (x, y)。 */
void hive_brush_move(void* handle, float x, float y);

/* 笔刷边长，单位是 tile，取值 1 到 249。 */
void hive_brush_size(void* handle, int32_t size);

/* 笔刷形状：0 方形，1 圆形，2 菱形。 */
void hive_brush_shape(void* handle, int32_t shape);

/* 选择工具。每个工具单独设置，后设的会关掉与它不兼容的旧工具。
   返回 0 成功，负数是错误。 */
int32_t hive_set_texture_tool(void* handle, const char* tile_id);
/* deformation：0 抬升，1 降低，2 平台，3 起伏，4 平滑。 */
int32_t hive_set_height_tool(void* handle, int32_t deformation);
/* operation：0 降两级，1 降一级，2 抹平，3 升一级，4 升两级，5 深水，6 浅水，7 斜坡。 */
int32_t hive_set_cliff_tool(void* handle, int32_t operation, const char* cliff_id);
/* operation：0 加水，1 去水，2 加边界，3 去边界。 */
int32_t hive_set_cell_tool(void* handle, int32_t operation);

/* 一次绘制。begin 会快照整张地形用于撤销，apply 可以按帧连调多次，end 收尾并入撤销栈。 */
/* 路径笔刷：mask 是 0x02 不可走、0x04 不可飞、0x08 不可建；operation 0 覆盖、1 加、2 减。 */
int32_t hive_set_pathing_tool(void* handle, int32_t mask, int32_t operation);

void hive_brush_begin(void* handle);
void hive_brush_apply(void* handle, double frame_delta);
void hive_brush_end(void* handle);

/* 撤销 / 重做一步，返回 1 表示动了，0 表示没有可撤销的。 */
int32_t hive_undo(void* handle);
int32_t hive_redo(void* handle);

/* 装饰物。position 是地形坐标系，y 是高度，单位与地形一致；angle 是弧度。 */
typedef struct HiveDoodad {
	float x;
	float y;
	float z;
	float angle;
	float scale_x;
	float scale_y;
	float scale_z;
	int32_t variation;
	int32_t solid;
} HiveDoodad;

int32_t hive_copy_doodads(void* handle, HiveDoodad* out, int32_t capacity);

/* 可放置的装饰物类型（四字符 id），来自 Doodads.slk。 */
int32_t hive_doodad_type_count(void* handle);
const char* hive_doodad_type_id(void* handle, int32_t index);
/* 放一个装饰物，返回新索引，失败返回 -1。坐标是 tile，angle 是弧度。 */
int32_t hive_add_doodad(void* handle, const char* id, float x, float y, float z, float angle);
/* 放置时的随机项，位 1 变体、位 2 旋转、位 4 缩放。下次 hive_add_doodad 生效。 */
void hive_set_doodad_random(void* handle, int32_t flags);

/* 剪贴板。copy 把 indices 指向的装饰物整组拷进去，返回拷了几个；
   中心点取它们的平均位置，放置时这个中心对齐鼠标。 */
int32_t hive_copy_selection(void* handle, const int32_t* indices, int32_t count);
int32_t hive_clipboard_count(void* handle);
/* 剪贴板快照，坐标是复制时的原始位置。 */
int32_t hive_copy_clipboard(void* handle, HiveDoodad* out, int32_t capacity);
/* 写 2 个 float：中心点的 x、y。 */
void hive_clipboard_anchor(void* handle, float* out);
/* 以 (x, y) 为落点放整组，每个都贴地形，整组算一次撤销。
   返回第一个新对象的索引，剪贴板为空返回 -1。 */
int32_t hive_place_clipboard(void* handle, float x, float y);
/* 剪贴板里第 index 个对象的 id 和模型，用于预览。 */
const char* hive_clipboard_id(void* handle, int32_t index);
int32_t hive_model_from_clipboard(void* handle, int32_t index);

/* 射线拾取。origin / direction 用 Godot 坐标（X 水平、Y 高度、Z 水平），
   max_distance 是 tile 单位。返回最近的装饰物或单位索引，没打中返回 -1。
   先做包围盒粗筛，命中后按距离排序取最近的。 */
int32_t hive_pick_doodad(void* handle, const float* origin, const float* direction, float max_distance);
int32_t hive_pick_unit(void* handle, const float* origin, const float* direction, float max_distance);

/* 框选。(x0, y0) 和 (x1, y1) 是矩形的两个角，tile 坐标，内部归一化。
   返回命中个数，索引按升序写进 out，capacity 不够时只写 capacity 个。
   is_unit 为 0 查装饰物，非 0 查单位。 */
int32_t hive_query_area(void* handle, float x0, float y0, float x1, float y1, int32_t is_unit,
						int32_t* out, int32_t capacity);

/* 批量删除，整组进一次撤销。返回删掉的数量。 */
int32_t hive_remove_selection(void* handle, int32_t is_unit, const int32_t* indices, int32_t count);

/* 选中对象的包围盒，Godot 坐标下的 min 和 max，共 6 个 float。is_unit 为 0 时读装饰物。 */
int32_t hive_selection_box(void* handle, int32_t is_unit, int32_t index, float* out);

/* 整组编辑。begin 快照选中的对象并记下鼠标位置，move_to 按鼠标位移平移全组
   （组里有带路径图的对象时吸到 0.5 格），rotate_to 把每个对象转向鼠标，
   end 把整组的前后差异压进一次撤销。 */
int32_t hive_begin_group(void* handle, int32_t is_unit, const int32_t* indices, int32_t count,
						 float x, float y);
int32_t hive_move_group_to(void* handle, int32_t is_unit, float x, float y);
int32_t hive_rotate_group_to(void* handle, int32_t is_unit, float x, float y);
int32_t hive_end_group(void* handle, int32_t is_unit);

/* 读对象当前角度（弧度）。 */
float hive_object_angle(void* handle, int32_t is_unit, int32_t index);
/* 第 index 个装饰物的四字符 id，用于查模型。越界返回 NULL。 */
const char* hive_doodad_id(void* handle, int32_t index);

/* 单位。坐标单位同装饰物：位置是 tile，angle 是弧度。player 是 0..15 的玩家位。 */
typedef struct HiveUnit {
	float x;
	float y;
	float z;
	float angle;
	float scale_x;
	float scale_y;
	float scale_z;
	int32_t player;
	int32_t variation;
} HiveUnit;

int32_t hive_copy_units(void* handle, HiveUnit* out, int32_t capacity);
const char* hive_unit_id(void* handle, int32_t index);

/* 玩家队伍色（RGB 0..1），player 是 0..23，超出返回 -1。 */
int32_t hive_player_color(int32_t player, float* rgb);

/* 模型的动画序列：数量、名字、起止帧。 */
int32_t hive_model_sequence_count(void* handle, int32_t model);
const char* hive_model_sequence_name(void* handle, int32_t model, int32_t index);
int32_t hive_model_sequence_range(void* handle, int32_t model, int32_t index, int32_t* start, int32_t* end);
/* 按序列和帧重算蒙皮顶点与几何体 alpha，之后用 hive_copy_model_positions 取新位置。 */
int32_t hive_model_apply_pose(void* handle, int32_t model, int32_t sequence, int32_t frame);

/* 可放置的单位类型（四字符 id），来自 Units.slk。 */
int32_t hive_unit_type_count(void* handle);
const char* hive_unit_type_id(void* handle, int32_t index);
/* 放一个单位，高度自动贴地形，进撤销栈。返回新索引，失败返回 -1。 */
int32_t hive_add_unit(void* handle, const char* id, float x, float y, float angle, int32_t player);

/* 单位的剪贴板，语义同装饰物那套。 */
int32_t hive_copy_unit_selection(void* handle, const int32_t* indices, int32_t count);
int32_t hive_unit_clipboard_count(void* handle);
int32_t hive_copy_unit_clipboard(void* handle, HiveUnit* out, int32_t capacity);
void hive_unit_clipboard_anchor(void* handle, float* out);
int32_t hive_place_unit_clipboard(void* handle, float x, float y);
const char* hive_unit_clipboard_id(void* handle, int32_t index);
int32_t hive_model_from_unit_clipboard(void* handle, int32_t index);

/* 模型网格。顶点、法线、UV 按 geoset 顺序拼成一个数组，索引各自独立。
   MDX 原始坐标系是 Z 向上，调用方自己转。 */
typedef struct HiveModelInfo {
	int32_t geoset_count;
	int32_t vertex_count;
	int32_t index_count;
	float min_x;
	float min_y;
	float min_z;
	float max_x;
	float max_y;
	float max_z;
} HiveModelInfo;

typedef struct HiveGeosetInfo {
	int32_t vertex_offset;
	int32_t vertex_count;
	int32_t index_offset;
	int32_t index_count;
	int32_t two_sided;
	int32_t unshaded;
	/* 这一层的贴图是 ReplaceableTextures/TeamColor，要按玩家上色。 */
	int32_t team_color;
	float alpha;
} HiveGeosetInfo;

/* 打开模型，返回 >= 0 的句柄，失败返回 -1。句柄在 hive_close 前一直有效。 */
int32_t hive_model_from_doodad(void* handle, int32_t doodad_index);
int32_t hive_model_from_unit(void* handle, int32_t unit_index);
int32_t hive_model_from_path(void* handle, const char* path);
int32_t hive_model_info(void* handle, int32_t model, HiveModelInfo* out);
int32_t hive_model_geoset(void* handle, int32_t model, int32_t geoset, HiveGeosetInfo* out);
/* 该 geoset 用的 BLP 路径，没有贴图返回 NULL。 */
const char* hive_model_geoset_texture(void* handle, int32_t model, int32_t geoset);

int32_t hive_copy_model_positions(void* handle, int32_t model, float* out, int32_t capacity);
int32_t hive_copy_model_normals(void* handle, int32_t model, float* out, int32_t capacity);
int32_t hive_copy_model_uvs(void* handle, int32_t model, float* out, int32_t capacity);
int32_t hive_copy_model_indices(void* handle, int32_t model, int32_t geoset, uint32_t* out, int32_t capacity);

/* 把 BLP / TGA / DDS 解成 RGBA8，返回尺寸或零。 */
int32_t hive_image_info_path(void* handle, const char* path, int32_t* width, int32_t* height);
int32_t hive_copy_image_path(void* handle, const char* path, uint8_t* out, int32_t capacity);

/* 对象属性。category：0 单位、1 物品、2 装饰物、3 可破坏物、4 技能、5 升级、6 buff。
   字段列表按 HiveWE 的规则从 meta slk 生成：跳过 useSpecific 对不上的，
   带 repeat 的按 levels / numvar / maxlevel 展开成多条。 */
/* 对象的显示名（单位取 slk 的 Name，装饰物解 WESTRING），没有就用 id。 */
const char* hive_object_type_name(void* handle, int32_t category, const char* id);
/* 编辑器树里所属的分类，按层取：单位是「种族、单位/建筑/英雄/特殊」两层，
   装饰物和可破坏物只有一层。level 越界返回 NULL。 */
const char* hive_object_type_folder(void* handle, int32_t category, const char* id, int32_t level);

int32_t hive_object_field_count(void* handle, int32_t category, const char* id);
/* 地图里的装饰物 id 落在哪张表：2 装饰物、3 可破坏物，都不在返回 -1。 */
int32_t hive_doodad_category(void* handle, const char* id);
/* meta 里的四字符字段 id，如 uhpm。 */
const char* hive_object_field_key(void* handle, int32_t category, int32_t index);
/* 显示名，"分类 - 名称 (key)"。 */
const char* hive_object_field_label(void* handle, int32_t category, int32_t index);
/* meta 的 type 列：int / real / unreal / string / bool 等。 */
const char* hive_object_field_type(void* handle, int32_t category, int32_t index);
const char* hive_object_field_description(void* handle, int32_t category, int32_t index);
/* 读当前值。改过的值优先。取不到返回 NULL。 */
const char* hive_object_value(void* handle, int32_t category, const char* id, int32_t index);
/* 写值，进 shadow 表，保存时写进 war3map.w3*。返回 0 成功。 */
int32_t hive_set_object_value(void* handle, int32_t category, const char* id, int32_t index, const char* value);
/* 该字段是否被改过。 */
int32_t hive_object_value_modified(void* handle, int32_t category, const char* id, int32_t index);

/* 自定义对象：把 source_id 整行复制成 new_id，shadow 里记下 oldid，
   保存时写进 war3map.w3* 的自定义段。返回 0 成功。 */
int32_t hive_create_object(void* handle, int32_t category, const char* source_id, const char* new_id);
/* 删掉一个自定义对象。基础对象删不掉，返回 -1。 */
int32_t hive_delete_object(void* handle, int32_t category, const char* id);
/* 是不是自定义对象。 */
int32_t hive_object_is_custom(void* handle, int32_t category, const char* id);
/* 生成一个没被占用的四字符 id，首字母大小写跟 source_id 一致。 */
const char* hive_unique_object_id(void* handle, const char* source_id);

/* 地图信息。四个文本字段存在 war3map.wts 里，返回的是解析后的内容。 */
const char* hive_map_name(void* handle);
int32_t hive_set_map_name(void* handle, const char* value);
const char* hive_map_author(void* handle);
int32_t hive_set_map_author(void* handle, const char* value);
const char* hive_map_description(void* handle);
int32_t hive_set_map_description(void* handle, const char* value);
const char* hive_map_suggested_players(void* handle);
int32_t hive_set_map_suggested_players(void* handle, const char* value);
/* 6 个 int：地图版本、编辑器版本、游戏主/次/补丁/构建版本。 */
void hive_map_versions(void* handle, int32_t* out);
/* 可玩区域尺寸。 */
void hive_map_playable_size(void* handle, int32_t* width, int32_t* height);
/* 可玩区域四边各留多少不可玩，单位是 tile。 */
void hive_map_playable_insets(void* handle, int32_t* left, int32_t* right, int32_t* top, int32_t* bottom);

/* 从四边调整地图尺寸，delta 是 tile 增量。新的边长要落在 32..480。
   地形、路径图、阴影图、所有预设对象一起重排，撤销栈清空。成功返回 0。 */
int32_t hive_resize_map(void* handle, int32_t delta_left, int32_t delta_right, int32_t delta_top, int32_t delta_bottom);
/* 设置可玩区域：四边各留多少不可玩，单位是 tile。 */
int32_t hive_set_playable_area(void* handle, int32_t left, int32_t right, int32_t top, int32_t bottom);

/* 触发器。分类、触发器、变量都来自 war3map.wtg。
   分类和触发器靠 parent_id 组树，顶层节点的 parent_id 是 0。 */
int32_t hive_trigger_category_count(void* handle);
int32_t hive_trigger_category_id(void* handle, int32_t index);
const char* hive_trigger_category_name(void* handle, int32_t index);
int32_t hive_trigger_category_parent(void* handle, int32_t index);

int32_t hive_trigger_count(void* handle);
int32_t hive_trigger_id(void* handle, int32_t index);
const char* hive_trigger_name(void* handle, int32_t index);
const char* hive_trigger_description(void* handle, int32_t index);
int32_t hive_trigger_parent(void* handle, int32_t index);
int32_t hive_trigger_is_comment(void* handle, int32_t index);
int32_t hive_trigger_is_enabled(void* handle, int32_t index);
int32_t hive_trigger_initially_on(void* handle, int32_t index);
/* 脚本类触发器（classifier 是 script）的脚本文本。 */
int32_t hive_trigger_is_script(void* handle, int32_t index);
const char* hive_trigger_custom_text(void* handle, int32_t index);
int32_t hive_set_trigger_custom_text(void* handle, int32_t index, const char* value);
/* ECA 是棵树：先 build 展开成节点列表，再按下标取字段。
   节点带 depth（0 是顶层）与 group（在父节点里的分组序号：
   If/Then/Else 是 0/1/2，And/Or 与循环只有 0）。 */
int32_t hive_trigger_build_nodes(void* handle, int32_t trigger);
int32_t hive_trigger_node_depth(void* handle, int32_t trigger, int32_t node);
/* type：0 事件、1 条件、2 动作。 */
int32_t hive_trigger_node_type(void* handle, int32_t trigger, int32_t node);
int32_t hive_trigger_node_group(void* handle, int32_t trigger, int32_t node);
/* 分组种类：0 无、1 单列表（And/Or、ForLoop）、3 If/Then/Else。 */
int32_t hive_trigger_node_group_kind(void* handle, int32_t trigger, int32_t node);
const char* hive_trigger_node_name(void* handle, int32_t trigger, int32_t node);
/* 参数代入后的整句文字，和 HiveWE 的 get_parameters_names 一致。 */
const char* hive_trigger_node_display(void* handle, int32_t trigger, int32_t node);

int32_t hive_trigger_node_parameter_count(void* handle, int32_t trigger, int32_t node);
/* 参数类型：-1 无效、0 预设、1 变量、2 函数、3 字符串。 */
int32_t hive_trigger_node_parameter_type(void* handle, int32_t trigger, int32_t node, int32_t parameter);
const char* hive_trigger_node_parameter_value(void* handle, int32_t trigger, int32_t node, int32_t parameter);
/* 参数在 TriggerData 里的类型名，如 unitcode。 */
const char* hive_trigger_node_parameter_type_name(void* handle, int32_t trigger, int32_t node, int32_t parameter);
int32_t hive_set_trigger_node_parameter(void* handle, int32_t trigger, int32_t node, int32_t parameter,
									   const char* value);

/* 往 parent（-1 是顶层）下面加一个 ECA，参数按 TriggerData 的定义留空。 */
int32_t hive_trigger_add_node(void* handle, int32_t trigger, int32_t type, const char* name, int32_t parent,
							  int32_t group);
int32_t hive_trigger_remove_node(void* handle, int32_t trigger, int32_t node);

/* 该类型的预设值（TriggerParams 段里属于这个类型的行）。 */
int32_t hive_trigger_preset_count(void* handle, const char* type_name);
const char* hive_trigger_preset_code(void* handle, const char* type_name, int32_t index);
const char* hive_trigger_preset_display(void* handle, const char* type_name, int32_t index);

/* 可用的 ECA 函数列表。type：0 事件、1 条件、2 动作。 */
int32_t hive_trigger_function_count(void* handle, int32_t type);
const char* hive_trigger_function_name(void* handle, int32_t type, int32_t index);

/* 新建触发器，挂在 parent_id 这个分类下，返回新索引。 */
int32_t hive_create_trigger(void* handle, const char* name, int32_t parent_id);
int32_t hive_delete_trigger(void* handle, int32_t index);
int32_t hive_set_trigger_name(void* handle, int32_t index, const char* value);
int32_t hive_set_trigger_description(void* handle, int32_t index, const char* value);
int32_t hive_set_trigger_enabled(void* handle, int32_t index, int32_t enabled);
int32_t hive_set_trigger_initially_on(void* handle, int32_t index, int32_t on);

int32_t hive_trigger_variable_count(void* handle);
const char* hive_trigger_variable_name(void* handle, int32_t index);
const char* hive_trigger_variable_type(void* handle, int32_t index);
int32_t hive_trigger_variable_is_array(void* handle, int32_t index);
const char* hive_trigger_variable_initial(void* handle, int32_t index);

/* 全局脚本，war3map.j 里用户自己写的那段。 */
const char* hive_map_global_script(void* handle);
int32_t hive_set_map_global_script(void* handle, const char* value);

#ifdef __cplusplus
}
#endif

#endif
