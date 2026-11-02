#include "map_info_editor.h"

#include <QMessageBox>
#include <QPainter>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <cassert>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <execution>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <print>
#include <queue>
#include <random>
#include <ranges>
#include <set>
#include <span>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include "SLK.hpp"
#include "Utilities.hpp"
#include "Globals.hpp"
#include "Tileset.hpp"

namespace fs = std::filesystem;

MapInfoEditor::MapInfoEditor(QWidget* parent, Map& map)
	: QDialog(parent), map(map), info(map.info), trigger_strings(map.trigger_strings), terrain(map.terrain), tilesets(map.tilesets) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	setup_description(info, trigger_strings);
	setup_loading_screen(info, trigger_strings, map.filesystem_path);
	setup_options(info, tilesets);
	setup_map_size(terrain, info);

	connect(ui.buttonBox, &QDialogButtonBox::accepted, [this]() {
		save();
		emit accept();
		close();
	});

	connect(ui.buttonBox, &QDialogButtonBox::rejected, [this]() {
		emit reject();
		close();
	});

	show();
}

void MapInfoEditor::save() const {
	save_description(info, trigger_strings);
	save_loading_screen(info, trigger_strings);
	save_options(info);
	save_map_size(map);
}
