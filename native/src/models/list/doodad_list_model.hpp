#pragma once


#include <QSortFilterProxyModel>


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
#include "qicon_resource.hpp"
#include "resource_manager.hpp"
#include "globals.hpp"
#include "base_list_model.hpp"

class DoodadListModel: public BaseListModel {
	Q_OBJECT

  public:
	explicit DoodadListModel(QObject* parent) : BaseListModel(doodads_slk, parent) {
		for (auto&& [key, value] : world_edit_data.section("DoodadCategories")) {
			const std::string tileset_key = value.front();
			icons[key.front()] = resource_manager.load<QIconResource>(value[1]).value();
		}
	}

	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override {
		if (!proxyIndex.isValid()) {
			return {};
		}

		return sourceModel()->index(proxyIndex.row(), doodads_slk.column_headers.at("name"));
	}

	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		switch (role) {
			case Qt::DisplayRole:
				return sourceModel()->data(mapToSource(index), role).toString();
			case Qt::UserRole:
				return QString::fromStdString("doodads/" + doodads_slk.data("category", index.row()) + "/" + doodads_slk.index_to_row.at(index.row()));
			case Qt::DecorationRole: {
				const char category = doodads_slk.data<std::string_view>("category", index.row()).front();
				if (icons.contains(category)) {
					return icons.at(category)->icon;
				} else {
					return {};
				}
			}
			default:
				return BaseListModel::data(index, role);
		}
	}

  private:
	std::unordered_map<char, std::shared_ptr<QIconResource>> icons;
};

class DoodadListFilter : public QSortFilterProxyModel {
	[[nodiscard]] bool filterAcceptsRow(const int sourceRow, const QModelIndex& sourceParent) const override {
		if (!filterRegularExpression().pattern().isEmpty()) {
			if (QString::fromStdString(doodads_slk.index_to_row.at(sourceRow)).contains(filterRegularExpression())) {
				return true;
			}

			const QModelIndex source_index = sourceModel()->index(sourceRow, 0);
			return source_index.data().toString().contains(filterRegularExpression());
		}

		if (filterCategory) {
			if (doodads_slk.data<std::string_view>("category", sourceRow) != filterCategory->toStdString()) {
				return false;
			}
		}

		if (filterTileset) {
			const std::string_view tilesets = doodads_slk.data<std::string_view>("tilesets", sourceRow);
			if (tilesets.find('*') == std::string::npos && tilesets.find(*filterTileset) == std::string::npos && filterTileset != '*') {
				return false;
			}
		}

		return true;
	}

	bool lessThan(const QModelIndex& left, const QModelIndex& right) const override {
		return doodads_slk.data<std::string_view>("name", left.row()) < doodads_slk.data<std::string_view>("name", right.row());
	}

	std::optional<QString> filterCategory;
	std::optional<char> filterTileset;

public:
	void setFilterCategory(const QString& category) {
		beginFilterChange();
		filterCategory = category;
		endFilterChange(Direction::Rows);
	}

	void setFilterTileset(const char tileset) {
		beginFilterChange();
		filterTileset = tileset;
		endFilterChange(Direction::Rows);
	}

	using QSortFilterProxyModel::QSortFilterProxyModel;
};

#include "doodad_list_model.moc"
