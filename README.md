# HiveWeGodot

把 HiveWE 的 C++ 核心原样拿过来，界面换成 Godot C#。
独立的地图编辑器，与 SekiaWorld 工程没有任何依赖关系。

思路：不重写地形算法、笔刷、地图读写，那些代码一行不改；只做两件事——把 C++20 模块语法拆成头文件（Apple clang 不支持模块），把 Qt 界面换成 Godot。

## 当前状态

| 阶段 | 状态 |
|---|---|
| 依赖 | 已齐：cmake / glm / tbb / abseil / jpeg-turbo / nlohmann-json / stormlib / bullet / vulkan-volk + 本机 Vulkan SDK |
| 去模块化 | 完成，95 个模块 + 67 个 cpp + 52 个头文件，产物在 `native/src` |
| 去 Qt | 完成，靠 `native/include` 的兼容层，核心层源码未改一行 |
| 去 Vulkan | 暂不摘：`resources/` 层包着 `VkTexture`，留着让包装层原样编过（见步骤 3） |
| 编核心库 | **已出 `libhivewe_core.a`（7.3 MB，6291 个符号）** |
| C ABI | 未开始 |
| Godot 界面 | 未开始 |

## 目录

```
HiveWeGodot/
├── README.md
├── tools/
│   └── decomod.py              去模块化转换脚本（.ixx → .hpp）
├── third_party/                手动放的 header-only 库
│   └── ankerl/unordered_dense.h
├── native/
│   ├── _upstream/              HiveWE 原仓库（git clone，不改）
│   ├── src/                   转换产物（脚本生成，可随时重跑）
│   └── CMakeLists.txt         核心库构建（待写）
└── godot/                      Godot 4 C# 工程（待建）
```

## 步骤 1：装依赖

```bash
brew install cmake glm stormlib tbb bullet abseil jpeg-turbo nlohmann-json
```

已装：`glm` `tbb` `abseil` `jpeg-turbo` `nlohmann-json`
待装：`cmake` `stormlib` `bullet`

后三个如果在 ghcr.io 上 SSL 失败，重试或者挂代理再跑。装不上的话源码编也行：

```bash
git clone --depth 1 https://github.com/ladislav-zezula/StormLib.git
git clone --depth 1 https://github.com/bulletphysics/bullet3.git
```

第三方头用脚本一次拉齐（删掉 `unordered_dense` 那段单独 curl）：

```bash
bash tools/fetch_third_party.sh http://127.0.0.1:7897
```

代理参数可省，直连能通就不用。拉的是 `ankerl/unordered_dense.h`、`ankerl/stl.h`、`vma/vk_mem_alloc.h`、`vkbootstrap/*`，全部落在 `third_party/`，已在 `.gitignore` 里。

Vulkan 用本机已装的 LunarG SDK，不另外装 molten-vk：

```bash
export VULKAN_SDK=/Volumes/Data/Users/sekia/VulkanSDK/1.4.328.1/macOS
```

SDK 里有 `include/vulkan/vulkan.h` 和 `lib/libMoltenVK.dylib`，实测能编能跑（Vulkan 1.4.328）。

SDK 不含 volk，单独装（注意别装成 DSP 那个 `volk`，要 `vulkan-volk`）：

```bash
brew install vulkan-volk
```

三个第三方头都要有，缺一个 `resources/` 层就编不动：`resources/ground_texture.hpp`、`skinned_mesh.hpp` 都 `#include "VkContext.hpp"`，而 `vk_context.hpp` 用 vk-bootstrap 建实例、用 VMA 分配内存。

**这些只服务于编译，运行期不碰。** 核心的 `VkContext` 是全局对象但默认构造不建实例，`init()` 才连设备；只要不调 `render_manager.init()` 和各个 `render_*`，编出来的库跑起来不会加载 MoltenVK。等核心跑通后把 `resources/` 换成 CPU 侧数据，这几个依赖可以整个摘掉。

编译核心库：

```bash
cd native
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

产物 `native/build/libhivewe_core.a`。覆盖 `base` / `brush` / `file_formats` / `utilities` / `resources` 五层共 18 个 cpp。

## 步骤 2：拉源码并去模块化

```bash
cd native
git clone --depth 1 https://github.com/stijnherfst/HiveWE.git _upstream
cd ..
python3 tools/decomod.py --src native/_upstream/src --dst native/src
```

输出 94 个 `.hpp` + 67 个 `.cpp`，保持原目录结构（不能拍平，`import "brush/brush.h"` 这类引号引用依赖层级）。

转换规则：`export module X;` → `#pragma once`，`import Y;` → `#include "Y.hpp"`，`import std;` → 一坨标准头，`export` 关键字删掉。

已验证：94 个模块里只有 1 个语法错（`thread_pool.hpp` 用了 `std::move_only_function`，Apple libc++ 没这个），其余失败都是缺第三方头。

## 步骤 3：去 Qt、去 Vulkan

这一步是手工改，因为两边的引用都在核心数据层里，不在 UI 层。

### 3.0 Qt 兼容层（已完成一部分）

改了 `native/include/` 下的一组同名转发头 + 一个 `qt_shim.hpp`：

```
native/include/
├── qt_shim.hpp      QRect / QRectF / QPoint / QMouseEvent / QKeyEvent / QWheelEvent / Qt:: 枚举
├── QRect            ├─ 转发到 qt_shim.hpp
├── QRectF           │
├── QPoint           │
├── QMouseEvent      │
├── QKeyEvent        │
└── QWheelEvent      ┘
```

好处：转换产物里的 `#include <QRect>` 一行都不用改，加 `-I native/include` 就能编。

已通过：`utilities/rects.hpp`、`base/camera.hpp`。

还没 shim 的（要用改代码而不是 shim）：`QObject`（信号机制）、`QMessageBox`、`QSettings`、`QWidget` / `QIcon` / `QProcess`（所在的四个文件整块不编）。

**Qt（22 处头文件引用）**

散布在 `base/camera.ixx`（`QMouseEvent` / `QWheelEvent`）、`base/terrain.ixx`（`QObject` 基类 + 2 个信号）、`base/map.ixx`（`QObject`）、`base/hierarchy.ixx`（`QSettings`）、`base/map_info.ixx` / `base/tilesets.ixx`（`QMessageBox` 报错）、`brush/*`（事件签名）、`utilities/rects.ixx`（`QRect`）。

改法：

| 原 | 改 |
|---|---|
| `class Terrain : public QObject` + `signals:` | 删基类，用 `std::function<void()>` 回调 |
| `QMouseEvent*` / `QKeyEvent*` 参数 | 自定义 POD `{x, y, button, modifiers}`，键码用自己的枚举 |
| `QMessageBox::critical` | 回调 `on_error(std::string)` |
| `QSettings` | 读 `settings.json` |
| `QRect` / `QRectF` | `Rects` 模块里自己的矩形类型 |
| `window_handler.ixx` | 整类删掉（Qt 窗口登记表） |

**Vulkan：不是删 200 行那么简单**

原以为 `volk.h` 只出现在 5 个文件、删掉 `render_*` 就完事。查下来有连锁：

- `base/terrain`、`base/pathing_map` 直接持 `Buffer` / `UpdatableTexture`
- `resources/texture`、`resources/ground_texture`、`resources/cliff_mesh`、`resources/skinned_mesh` 全部包着 `VkTexture` / 蒙皮网格
- `base/doodads`、`base/units` 持 `SkinnedMesh`
- `brush/*` 引 `SkinnedMesh`

也就是说 **`resources/` 这一层（1272 行）大半是 Vulkan 包装**，删渲染等于重写这一层。

两条路：

| 路 | 做法 | 代价 |
|---|---|---|
| A. 装 Vulkan 依赖 | 本地 SDK + `vulkan-volk` + VMA 空壳头，让包装层照原样编 | 依赖变重，但代码不用动 |
| B. 重写 resources 层 | 把 `VkTexture` 换成 CPU 侧的 `Image` 数据 | 1272 行里的大半要重写 |

**先走 A**：代码一行不改，等核心跑通、确认地形数据能出来，再评估要不要换 B。

## 步骤 4：编核心库

```bash
cmake -S native -B native/build -DCMAKE_CXX_STANDARD=23
cmake --build native/build -j
```

产物是一个静态库 `libhivewe_core.a`，只含 `base/` `brush/` `file_formats/` `resources/` 里的模块。

不编的目录：`main_window/` `menus/` `custom_widgets/` `object_editor/` `trigger_editor/` `model_editor/` `asset_manager/` `qt_imgui/` `render/`。

## 步骤 5：C ABI

```cpp
// native/src/hive_c.h
void* hive_open(const char* map, const char* game_dir);
void  hive_close(void* h);
int   hive_save(void* h, const char* path);

void  hive_terrain_info(void* h, int* w, int* hgt, int* tileset, int* tex_count);
void  hive_copy_heights(void* h, float* out, int count);
void  hive_copy_tiles(void* h, HiveTile* out, int count);

void  hive_set_tool(void* h, int tool, int shape, int size);
void  hive_pointer(void* h, float x, float y, int state);
void  hive_undo(void* h);
void  hive_redo(void* h);
```

坐标统一用 tile 浮点（HiveWE 的原生单位），Godot 侧乘 128。

## 步骤 6：Godot 界面

`godot/` 下建 Godot 4 C# 工程，`[DllImport("hivewe_core")]` 调上面那套接口。

界面照 HiveWE：顶部 Ribbon（大图标 + Home / View / Map 分页），左侧面板 Tab（地形 / 装饰物 / 单位 / pathing / 区域），中间 3D 视口。

地形渲染用 `MultiMesh`，每 tile 一个实例，`INSTANCE_CUSTOM` 带高度和纹理索引，混合公式照抄 `data/shaders/terrain.frag` 那 4 行。

## 已知坑

| 坑 | 说明 |
|---|---|
| `std::move_only_function` | Apple libc++ 没有，`utilities/thread_pool.ixx` 用了 4 处，换成 `std::function` |
| 转换后的 include 路径 | 脚本用 `#include "X.hpp"`，靠 `-I` 找，CMake 里要把所有子目录都加进 `target_include_directories` |
| `#pragma once` 位置 | 原文件的 `module;` 片段在前、`export module` 在后，脚本会把 `#pragma once` 提到第一行 |
| 与上游分叉 | 转换是单向的，HiveWE 后续更新不能直接同步 |
| brew 在沙箱里跑不动 | `sandbox-exec: Operation not permitted`，依赖要在普通终端装 |
