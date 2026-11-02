# HiveWeGodot

把 HiveWE 的 C++ 核心原样拿过来，界面与渲染换成 Godot C#。独立的地图编辑器，与 SekiaWorld 没有依赖关系。

地形算法、笔刷、地图读写一行不改。只做两件事：把 C++20 模块语法拆成头文件（Apple clang 不支持模块），把 Qt 与 Vulkan 的依赖挡在编译层外面。

## 当前状态

| 阶段 | 状态 |
|---|---|
| 去模块化 | 完成：95 个模块 + 67 个 cpp + 52 个头文件，产物在 `native/src` |
| 去 Qt | 完成：靠 `native/include` 的兼容层，核心层源码未改一行 |
| Vulkan | 保留依赖让 `resources/` 层原样编过，运行期不触碰 |
| 核心库 | 已出 `libhivewe_core.a`，7.3 MB，6291 个符号 |
| headless 化 | 完成，`HIVE_HEADLESS` 在 8 处短路 Vulkan 副作用 |
| 读地图 | 完成：解包 w3m 后 60 ms 加载完，193×97、7 种纹理、2011 装饰物、61 单位、18721 高度点 |
| C ABI | 完成：`libhivewe.dylib`，24 个函数，纯 C 测试程序跑通 |
| Godot 接入 | 完成：Godot 4.7 + .NET 8，DllImport 调到核心 |
| 地形渲染 | 完成：MultiMesh + 高度图 + 四层纹理混合 + 水面，BootyBay 出画面 |
| 笔刷 | 完成：纹理 / 高度 / 水 / 悬崖，带撤销重做 |
| 保存 | 完成：改完能存回 `.w3m`，重开数据一致 |
| 装饰物 | 完成：MDX 静态渲染 + 放置。469 种可选，2011 个里摆上 1991 个 |
| 单位 | 完成：61 个里摆上 59 个，18 种模型（建筑和船也在内） |
| 拾取 | 完成：射线选中装饰物和单位，黄色线框标出包围盒 |
| 删除 | 完成：选中后 Delete 删除，进撤销栈 |
| 拖动 | 完成：选中后拖动改位置，高度自动贴地形 |
| 旋转 | 完成：Ctrl + 拖动绕中心转，按 slk 的 fixedrot 吸附 |
| 路径图 | 完成：只读叠加显示，勾「显示路径」看不可走区域；不能画 |
| 区域 | 完成：矩形线框贴合地形，颜色取自 w3r；不能新建或编辑 |
| 界面 | 地形调色板、小地图、工具条、状态栏可用；区域 / 对象编辑器未做 |

## 文档

- [`docs/架构.md`](docs/架构.md)：分层处置、Qt 兼容层、第三方替身、C ABI、界面与渲染分工、实施顺序、风险
- [`docs/HiveWE功能清单.md`](docs/HiveWE功能清单.md)：HiveWE 有哪些编辑能力，各模块完整度
- [`docs/HiveWE版本与客户端支持.md`](docs/HiveWE版本与客户端支持.md)：各版本对应哪个客户端版本、功能演进、相对官方 WE 的差距

## 快速开始

装依赖：

```bash
brew install cmake glm tbb abseil jpeg-turbo nlohmann-json stormlib bullet vulkan-volk
bash tools/fetch_third_party.sh            # 拉 VMA / vk-bootstrap / unordered_dense
```

Vulkan 用本机 LunarG SDK，不需要另装 molten-vk。路径在 `native/CMakeLists.txt` 的 `VULKAN_SDK` 里改。

拉源码并转换：

```bash
bash tools/fetch_upstream.sh               # clone HiveWE 到 native/_upstream，固定在 cd6d61a
python3 tools/decomod.py --src native/_upstream/src --dst native/src
```

编译：

```bash
cd native
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

产物 `native/build/libhivewe_core.a`。

编 C ABI 库：

```bash
cmake --build native/build --target hivewe -j8
```

产物 `native/build/libhivewe.dylib`。

跑 Godot：

```bash
bash tools/build_godot_lib.sh                       # 编 dylib 并拷到 godot/lib
/Applications/Godot_mono.app/Contents/MacOS/Godot --path godot
```

操作：

| 按键 | 作用 |
|---|---|
| 1-9 | 选第 N 个地面纹理 |
| Q / E | 笔刷缩小 / 放大 |
| Ctrl+O | 打开地图 |
| Z / Y | 撤销 / 重做 |
| 右键拖动 | 平移 |
| 滚轮 | 缩放 |
| 左键 | 绘制 |
| Ctrl+S | 保存到 `output/edited.w3m` |
| Delete | 删除选中对象（Ctrl+Z 撤销） |
| 左键拖框 | 框选对象，Shift 加选、Alt 减选 |
| Ctrl + 左键拖动 | 旋转选中对象 |
| Ctrl+C / Ctrl+V | 复制选中对象 / 整组贴到鼠标处 |
| Esc | 取消粘贴 |

左侧面板的「显示路径」会把不可走的格子标红。「装饰物」「单位」两个下拉选类型后，点地图即可放置。

选中单个对象后右侧出现属性面板，列出该类型的全部字段（单位 267 个、可破坏物 52 个），顶部可筛选，改过的字段标紫，Ctrl+S 一并写进地图。

左侧面板可直接点：纹理墙、五个高度工具、加水去水、四种悬崖操作、笔刷尺寸与形状。面板底部是小地图，点哪里相机跳到哪里。

回归测试：

```bash
bash tools/run_native_tests.sh                      # 纯 C 测 ABI、笔刷、保存、打包
HIVE_SCREENSHOT=/tmp/shot.png \
  /Applications/Godot_mono.app/Contents/MacOS/Godot --path godot    # 存一张截图后退出
HIVE_AUTOTEST=/tmp/edit HIVE_SCREENSHOT=/tmp/edit \
  /Applications/Godot_mono.app/Contents/MacOS/Godot --path godot    # 自动画三笔再截图
HIVE_RELOAD_TEST=1 HIVE_SCREENSHOT=/tmp/reload.png \
  /Applications/Godot_mono.app/Contents/MacOS/Godot --path godot    # 换成 IceCrown 再截图
HIVE_PICK_TEST=1 HIVE_SCREENSHOT=/tmp/pick.png \
  /Applications/Godot_mono.app/Contents/MacOS/Godot --path godot    # 拾取、删除、移动、旋转
HIVE_CLIP_TEST=/tmp/clip_after.png HIVE_SCREENSHOT=/tmp/clip_preview.png \
  /Applications/Godot_mono.app/Contents/MacOS/Godot --path godot    # 复制粘贴，存预览与放置后两张
HIVE_UNIT_TEST=/tmp/unit_after.png HIVE_SCREENSHOT=/tmp/unit_preview.png \
  /Applications/Godot_mono.app/Contents/MacOS/Godot --path godot    # 放一个单位再复制粘贴
HIVE_SELECT_TEST=/tmp/select_after.png HIVE_SCREENSHOT=/tmp/select.png \
  /Applications/Godot_mono.app/Contents/MacOS/Godot --path godot    # 框选一片再批量删除
HIVE_OBJECT_TEST=/tmp/object_after.png HIVE_SCREENSHOT=/tmp/object.png \
  /Applications/Godot_mono.app/Contents/MacOS/Godot --path godot    # 选中对象改一条属性
```

验证读图。仓库自带 `testdata/`（三张地图 + 25 MB 资源子集），直接跑就行：

```bash
./native/build/hivewe_smoke
./native/build/hivewe_smoke "testdata/maps/(12)IceCrown.w3m"
```

也可以指向装了客户端的目录，程序会优先用里面的 MPQ：

```bash
./native/build/hivewe_smoke "testdata/maps/(2)BootyBay.w3m" "/path/to/Warcraft III"
```

`testdata/warcraft` 是从完整客户端按访问清单提取出来的，重建见架构文档。

## 目录

```
HiveWeGodot/
├── docs/                       架构与功能清单
├── tools/
│   ├── decomod.py              去模块化转换（规则与补丁表都在里面）
│   ├── fetch_upstream.sh       按固定 commit 拉 HiveWE
│   └── fetch_third_party.sh    拉 VMA / vk-bootstrap / unordered_dense
├── native/
│   ├── _upstream/              HiveWE 原仓库（不进库）
│   ├── src/                    转换产物：.hpp / .cpp / .h
│   ├── include/                Qt 兼容层与第三方替身
│   ├── CMakeLists.txt          hivewe_core 静态库
│   └── build/                  构建目录（不进库）
├── third_party/                vma / vkbootstrap / ankerl（不进库）
└── godot/                      Godot 4 C# 工程（待建）
```

`native/src` 是生成物，但进库。改动都写在 `decomod.py` 的补丁表里，重新生成时自动套用；看代码不用先跑脚本。

## 已知坑

| 坑 | 说明 |
|---|---|
| 模块名与文件名不一致 | 模块名 PascalCase，文件名 snake_case，95 个里 90 个对不上。转换时按模块名查真实文件名 |
| 裸 `module X;` | 10 个实现单元声明，要换成 `#include "X.hpp"` |
| 普通 `.h` 文件 | 52 个里面 35 个含 `import`，不能只复制 |
| `signals:` 段 | 7 个信号，换成 `std::function` 成员 |
| `#include "*.moc"` | 17 处 Qt 元对象文件，编译前删掉 |
| `public slots:` | 8 处，换成 `public:` |
| Apple libc++ 缺 C++23 特性 | `move_only_function`、`enumerate_view`、`execution::par`，见架构文档第 3 节 |
| shell 拼 `-I` 列表 | 变量展开后 clang 可能静默丢掉，改用 `@rsp` 响应文件或 CMake |
| 与上游分叉 | 转换是单向的，HiveWE 后续更新不能直接同步 |
