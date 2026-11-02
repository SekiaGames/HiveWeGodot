#include "map_info_editor.h"

#include <QMessageBox>
#include <QPainter>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <codecvt>
#include <compare>
#include <complex>
#include <concepts>
#include <coroutine>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <cwctype>
#include <deque>
#include <exception>
#include <execution>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <initializer_list>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <new>
#include <numbers>
#include <numeric>
#include <optional>
#include <ostream>
#include <print>
#include <queue>
#include <random>
#include <ranges>
#include <ratio>
#include <regex>
#include <set>
#include <shared_mutex>
#include <source_location>
#include <span>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <stop_token>
#include <streambuf>
#include <string>
#include <string_view>
#include <syncstream>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
#include "slk.hpp"
#include "utilities.hpp"
#include "globals.hpp"
#include "tilesets.hpp"

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
		if (accept) { accept(); }
		close();
	});

	connect(ui.buttonBox, &QDialogButtonBox::rejected, [this]() {
		if (reject) { reject(); }
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
