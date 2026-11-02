#pragma once


#include <QIcon>
#include <QImage>
#include <QPixMap>


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

class QIconResource : public Resource {
  public:
	QIcon icon;

	static constexpr const char* name = "QIconResource";

	explicit QIconResource() = default;

	explicit QIconResource(const fs::path& path) {
		const auto image = resource_manager.load<Texture>(path).value();
		const QImage temp_image(image->data.data(), image->width, image->height, image->channels == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_RGBA8888);
		const auto pix = QPixmap::fromImage(temp_image);
		icon = QIcon(pix);
	}
};