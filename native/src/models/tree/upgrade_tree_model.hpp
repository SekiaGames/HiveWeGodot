#pragma once


#include <QMap>
#include <QModelIndex>


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
#include "slk.hpp"
#include "globals.hpp"
#include "unordered_map.hpp"

class UpgradeTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		BaseTreeItem* item;
	};

	hive::unordered_map<std::string, Category> categories;
	std::vector<std::string> rowToCategory;

	BaseTreeItem* getFolderParent(const std::string& id) const override {
		const std::string_view race = upgrade_slk.data<std::string_view>("race", id);
		if (race.empty()) {
			std::cout << "Empty race for " << id << " in items\n";
			return nullptr;
		}

		return categories.at(race).item;
	}

	DropChange prepareDrop(const std::string& id, const BaseTreeItem* target) const override {
		if (!target->baseCategory) {
			return {};
		}

		return { DropChange::Verdict::accept, {}, { { "race", rowToCategory[target->row()] } } };
	}

	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override {
		if (!proxyIndex.isValid()) {
			return {};
		}

		BaseTreeItem* item = static_cast<BaseTreeItem*>(proxyIndex.internalPointer());

		if (item->baseCategory || item->subCategory) {
			return {};
		}

		return createIndex(slk->row_headers.at(item->id), slk->column_headers.at("name1"), item);
	}

  public:
	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		BaseTreeItem* item = static_cast<BaseTreeItem*>(index.internalPointer());

		switch (role) {
			case Qt::DecorationRole:
				if (item->baseCategory || item->subCategory) {
					return folderIcon;
				}
				return sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("art1")), role);
			case Qt::EditRole:
			case Qt::DisplayRole:
				if (item->baseCategory) {
					return QString::fromStdString(categories.at(rowToCategory[index.row()]).name);
				} else {
					return QAbstractProxyModel::data(index, role).toString() + " " + sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("editorsuffix1")), role).toString();
				}
			default:
				return BaseTreeModel::data(index, role);
		}
	}

	explicit UpgradeTreeModel(QObject* parent)
		: BaseTreeModel(parent) {
		slk = &upgrade_slk;

		for (const auto& [key, value] : unit_editor_data.section("unitRace")) {
			if (key == "Sort" || key == "NumValues") {
				continue;
			}

			categories[value[0]].name = value[1];
			categories[value[0]].item = new BaseTreeItem(rootItem);
			categories[value[0]].item->baseCategory = true;
			rowToCategory.push_back(value[0]);
		}

		for (int i = 0; i < upgrade_slk.rows(); i++) {
			const std::string& id = upgrade_slk.index_to_row.at(i);

			const std::string_view race = upgrade_slk.data<std::string_view>("race", id);
			if (race.empty()) {
				std::cout << "Empty race for " << i << " in items\n";
				continue;
			}

			BaseTreeItem* parent_item = getFolderParent(id);
			if (!parent_item) {
				continue;
			}

			BaseTreeItem* item = new BaseTreeItem(parent_item);
			item->id = id;
			items.emplace(id, item);
		}

		categoryChangeFields = { "race" };
		mimeType = "application/x-hivewe-upgrades";
	}
};