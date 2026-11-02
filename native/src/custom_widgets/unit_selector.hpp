#pragma once


#include <QListView>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
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
#include "globals.hpp"
#include "unit_list_model.hpp"
#include "table_model.hpp"

class UnitSelector : public QWidget {
	Q_OBJECT

  public:
	explicit UnitSelector(QWidget* parent = nullptr)
		: QWidget(parent) {
		list_model = new UnitListModel(this);
		list_model->setSourceModel(units_table);

		filter_model = new UnitListFilter(this);
		filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
		filter_model->setSourceModel(list_model);
		filter_model->sort(0, Qt::AscendingOrder);
		filter_model->setFilterRace("human");

		race = new QComboBox(this);
		search = new QLineEdit(this);
		units = new QListView(this);

		search->setPlaceholderText("Search");
		search->setClearButtonEnabled(true);

		units->setUniformItemSizes(true);
		units->setIconSize(QSize(24, 24));
		units->setModel(filter_model);

		QVBoxLayout* layout = new QVBoxLayout;
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(race);
		layout->addWidget(search);
		layout->addWidget(units);
		setLayout(layout);

		for (const auto& [key, value] : unit_editor_data.section("unitRace")) {
			if (key == "Sort" || key == "NumValues") {
				continue;
			}
			race->addItem(QString::fromStdString(value[1]), QString::fromStdString(value[0]));
		}

		connect(race, QOverload<int>::of(&QComboBox::currentIndexChanged), [&]() {
			filter_model->setFilterRace(race->currentData().toString());
		});

		connect(search, &QLineEdit::textEdited, filter_model, &QSortFilterProxyModel::setFilterFixedString);
		connect(search, &QLineEdit::returnPressed, [&]() {
			units->setCurrentIndex(units->model()->index(0, 0));
			units->setFocus();
		});

		connect(units, &QListView::clicked, [&](const QModelIndex& index) {
			const int row = filter_model->mapToSource(index).row();
			emit unitSelected(units_slk.index_to_row.at(row));
		});
		connect(units, &QListView::activated, [&](const QModelIndex& index) {
			const int row = filter_model->mapToSource(index).row();
			emit unitSelected(units_slk.index_to_row.at(row));
		});
	}

	UnitListModel* list_model;
	UnitListFilter* filter_model;

	QComboBox* race;
	QLineEdit* search;
	QListView* units;

  public:
	void forceSelection() {
		if (units->selectionModel()->selectedRows().isEmpty()) {
			return;
		}
		QModelIndex index = units->selectionModel()->selectedRows().front();
		if (!index.isValid()) {
			return;
		}
		const int row = filter_model->mapToSource(index).row();
		emit unitSelected(units_slk.index_to_row.at(row));
	}

  signals:
	void unitSelected(std::string id);
};

