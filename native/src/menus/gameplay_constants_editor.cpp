#include "gameplay_constants_editor.h"

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
#include "gameplay_constants.hpp"
#include "table_model.hpp"

#include "single_model.h"

#include <QHeaderView>
#include <QTableView>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QVBoxLayout>

GameplayConstantsEditor::GameplayConstantsEditor(QWidget* parent, GameplayConstants& constants, TriggerStrings& trigger_strings) : QDialog(parent) {
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle("Gameplay Constants");
	resize(1000, 600);

	auto* layout = new QVBoxLayout(this);
	search = new QLineEdit(this);
	search->setPlaceholderText("Search...");
	layout->addWidget(search);

	TableModel* table = new TableModel(&constants.data, &constants.metadata, &trigger_strings, this);
	SingleModel* single_model = new SingleModel(table, this);
	single_model->setID(constants_row_key);

	QSortFilterProxyModel* proxy_model = new QSortFilterProxyModel(this);
	proxy_model->setSourceModel(single_model);
	proxy_model->setFilterKeyColumn(-1); // All columns
	proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);

	TableDelegate* delegate = new TableDelegate(single_model);

	QTableView* view = new QTableView;
	layout->addWidget(view);
	view->setVerticalHeader(new AlterHeader(Qt::Vertical, view));
	view->setModel(proxy_model);
	view->setItemDelegate(delegate);
	view->horizontalHeader()->hide();
	view->setAlternatingRowColors(true);
	view->setIconSize({24, 24});
	layout->addWidget(view);

	connect(search, &QLineEdit::textChanged, proxy_model, &QSortFilterProxyModel::setFilterFixedString);

	show();

	// So the columns don't resize on search
	QTimer::singleShot(0, [view, proxy_model]() {
		view->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
		const int vertical_width = view->verticalHeader()->width();
		view->verticalHeader()->setFixedWidth(vertical_width);

		view->resizeColumnsToContents();
		for (int i = 0; i < proxy_model->columnCount(); ++i) {
			const int horizontal_width = view->columnWidth(i);

			view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
			view->setColumnWidth(i, horizontal_width);
		}
	});
}
