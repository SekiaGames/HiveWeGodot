#pragma once


#include <QMap>
#include <QMargins>
#include <QObject>
#include <QModelIndex>
#include <QSize>
#include <QIcon>


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
#include "base_tree_model.hpp"
#include "qicon_resource.hpp"
#include "slk.hpp"
#include "globals.hpp"
#include "resource_manager.hpp"

class DestructibleTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		std::shared_ptr<QIconResource> icon;
		BaseTreeItem* item;
	};

	std::unordered_map<char, Category> categories;
	std::vector<char> rowToCategory;

	BaseTreeItem* getFolderParent(const std::string& id) const override {
		const std::string_view category = destructibles_slk.data<std::string_view>("category", id);

		return categories.at(category.front()).item;
	}

	DropChange prepareDrop(const std::string& id, const BaseTreeItem* target) const override {
		if (!target->baseCategory) {
			return {};
		}

		return { DropChange::Verdict::accept, {}, { { "category", std::string(1, rowToCategory[target->row()]) } } };
	}

  public:
	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		BaseTreeItem* item = static_cast<BaseTreeItem*>(index.internalPointer());

		switch (role) {
			case Qt::EditRole:
			case Qt::DisplayRole:
				if (item->baseCategory) {
					return QString::fromStdString(categories.at(rowToCategory[index.row()]).name);
				} else {
					return QAbstractProxyModel::data(index, role).toString() + " " + sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("editorsuffix")), role).toString();
				}
			case Qt::DecorationRole:
				if (item->baseCategory || item->subCategory) {
					return folderIcon;
				}

				return categories.at(rowToCategory[index.parent().row()]).icon->icon;
			default:
				return BaseTreeModel::data(index, role);
		}
	}

	explicit DestructibleTreeModel(QObject* parent)
		: BaseTreeModel(parent) {
		slk = &destructibles_slk;

		for (const auto& [key, value] : world_edit_data.section("DestructibleCategories")) {
			categories[key.front()].name = value[0];
			categories[key.front()].icon = resource_manager.load<QIconResource>(value[1]).value();
			categories[key.front()].item = new BaseTreeItem(rootItem);
			categories[key.front()].item->baseCategory = true;
			rowToCategory.push_back(key.front());
		}

		for (const auto& [id, index] : destructibles_slk.row_headers) {
			BaseTreeItem* item = new BaseTreeItem(getFolderParent(id));
			item->id = id;
			items.emplace(id, item);
		}

		categoryChangeFields = { "category" };
		mimeType = "application/x-hivewe-destructibles";
	}
};