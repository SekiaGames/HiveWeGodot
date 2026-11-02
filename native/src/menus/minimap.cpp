#include "minimap.h"

#include <QMouseEvent>

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

Minimap::Minimap(QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);
	show();
}

void Minimap::set_minimap(Texture texture) {
	QImage temp_image = QImage(texture.data.data(), texture.width, texture.height, texture.width * texture.channels, QImage::Format::Format_RGBA8888);
	ui.image->setPixmap(QPixmap::fromImage(temp_image));
}

int x_offset;
int y_offset;

void Minimap::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		if (event->modifiers() & Qt::ControlModifier) {
			x_offset = event->globalPosition().x() - geometry().x();
			y_offset = event->globalPosition().y() - geometry().y();
		} else {
			float x = (event->position().x() - ui.image->horizontal_border) / (width() - ui.image->horizontal_border * 2.f);
			float y = (event->position().y() - ui.image->vertical_border) / (height() - ui.image->vertical_border * 2.f);

			x = std::clamp(x, 0.f, 1.f);
			y = std::clamp(y, 0.f, 1.f);

			emit clicked({ x, y });
		}
	}
}

void Minimap::mouseReleaseEvent(QMouseEvent*) {
	setCursor(Qt::ArrowCursor);
}

void Minimap::mouseMoveEvent(QMouseEvent* event) {
	if (event->modifiers() & Qt::ControlModifier) {
		setCursor(Qt::SizeAllCursor);
	}
	if (event->buttons() & Qt::LeftButton) {
		if (event->modifiers() & Qt::ControlModifier) {
			move(event->globalPosition().x() - x_offset, event->globalPosition().y() - y_offset);
		} else if (rect().contains(event->position().toPoint())) {
			float x = (event->position().x() - ui.image->horizontal_border) / (width() - ui.image->horizontal_border * 2.f);
			float y = (event->position().y() - ui.image->vertical_border) / (height() - ui.image->vertical_border * 2.f);
			
			x = std::clamp(x, 0.f, 1.f);
			y = std::clamp(y, 0.f, 1.f);

			emit clicked({ x, y });
		}
	}
}