#pragma once


#include <QMap>
#include <QModelIndex>


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
#include "BaseTreeModel.hpp"
#include "SLK.hpp"
#include "Globals.hpp"
#include "UnorderedMap.hpp"

class BuffTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		BaseTreeItem* item;
	};

	hive::unordered_map<std::string, Category> categories;
	std::vector<std::string> rowToCategory;

	std::array<std::string, 2> subCategories = {
		"Buffs",
		"Effects",
	};

	BaseTreeItem* getFolderParent(const std::string& id) const override {
		const std::string_view race = buff_slk.data<std::string_view>("race", id);
		if (race.empty()) {
			std::cout << "Empty race for " << id << " in buffs\n";
			return nullptr;
		}
		const bool isEffect = buff_slk.data("iseffect", id) == "1";
		const int subIndex = isEffect ? 1 : 0;

		return categories.at(race).item->children[subIndex];
	}

	DropChange prepareDrop(const std::string& id, const BaseTreeItem* target) const override {
		if (target->baseCategory) {
			// Only the race changes, the sub category stays the same
			return { DropChange::Verdict::accept, {}, { { "race", rowToCategory[target->row()] } } };
		}

		if (!target->subCategory) {
			return {};
		}

		return { DropChange::Verdict::accept,
				 {},
				 { { "race", rowToCategory[target->parent->row()] }, { "iseffect", target->row() == 1 ? "1" : "0" } } };
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
				} else if (item->subCategory) {
					return QString::fromStdString(subCategories[index.row()]);
				} else {
					const QString editorname = sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("editorname")), role).toString();
					if (editorname.isEmpty()) {
						return QAbstractProxyModel::data(index, role).toString() + " " + sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("editorsuffix")), role).toString();
					} else {
						return editorname + " " + sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("editorsuffix")), role).toString();
					}
				}
			default:
				return BaseTreeModel::data(index, role);
		}
	}

	explicit BuffTreeModel(QObject* parent)
		: BaseTreeModel(parent) {
		slk = &buff_slk;
		display_column = "bufftip";

		for (const auto& [key, value] : unit_editor_data.section("unitRace")) {
			if (key == "Sort" || key == "NumValues") {
				continue;
			}

			categories[value[0]].name = value[1];
			categories[value[0]].item = new BaseTreeItem(rootItem);
			categories[value[0]].item->baseCategory = true;
			rowToCategory.push_back(value[0]);
		}

		for (const auto& i : rootItem->children) {
			for (const auto& subCategory : subCategories) {
				BaseTreeItem* item = new BaseTreeItem(i);
				item->subCategory = true;
			}
		}

		for (int i = 0; i < buff_slk.rows(); i++) {
			const std::string& id = buff_slk.index_to_row.at(i);

			BaseTreeItem* parent_item = getFolderParent(id);
			if (!parent_item) {
				continue;
			}

			BaseTreeItem* item = new BaseTreeItem(parent_item);
			item->id = id;
			items.emplace(id, item);
		}

		categoryChangeFields = { "race", "iseffect" };
		mimeType = "application/x-hivewe-buffs";
	}
};