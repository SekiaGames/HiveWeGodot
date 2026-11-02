#pragma once


#include <QPainter>
#include <QIcon>


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
#include "ResourceManager.hpp"
#include "Texture.hpp"

namespace fs = std::filesystem;

/// Loads a texture from the hierarchy and returns an icon
QIcon texture_to_icon(const fs::path& path) {
	const auto tex = resource_manager.load<Texture>(path).value();
	const QImage temp_image = QImage(
		tex->data.data(),
		tex->width,
		tex->height,
		tex->channels == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_RGBA8888
	);
	const auto pix = QPixmap::fromImage(temp_image);
	return QIcon(pix);
}
