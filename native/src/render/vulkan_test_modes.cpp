// Standalone test windows for the Vulkan renderers, run instead of the editor:
// `--vulkan-model-grid` and `--vulkan-model-editor [path]` show those views with stock models.
// Each takes `--quit-after <seconds>`.

#include "vulkan_test_modes.h"

#include <QApplication>
#include <QMainWindow>
#include <QTimer>

#include <QHBoxLayout>
#include <QScrollBar>

#include "DockManager.h"
#include "object_editor/model_grid_widget.h"
#include "model_editor/model_editor_viewport.h"

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
#include "vk_context.hpp"
#include "resource_manager.hpp"
#include "hierarchy.hpp"
#include "mdx.hpp"
#include "binary_reader.hpp"

namespace fs = std::filesystem;

int run_vulkan_model_grid_test() {
	// A mix of units, buildings, particle-heavy spells, and one path that does not exist
	const std::vector<ModelEntry> entries = {
		{"units/human/Footman/Footman.mdx", ModelCategory::Units},
		{"units/orc/Grunt/Grunt.mdx", ModelCategory::Units},
		{"units/nightelf/HeroDemonHunter/HeroDemonHunter.mdx", ModelCategory::Units},
		{"units/undead/Abomination/Abomination.mdx", ModelCategory::Units},
		{"units/human/HeroPaladin/HeroPaladin.mdx", ModelCategory::Units},
		{"units/creeps/SatyrHellcaller/SatyrHellcaller.mdx", ModelCategory::Units},
		{"buildings/human/TownHall/TownHall.mdx", ModelCategory::Buildings},
		{"buildings/orc/GreatHall/GreatHall.mdx", ModelCategory::Buildings},
		{"buildings/other/CircleOfPower/CircleOfPower.mdx", ModelCategory::Buildings},
		{"abilities/spells/human/ThunderClap/ThunderClapCaster.mdx", ModelCategory::Abilities},
		{"abilities/spells/other/Doom/DoomTarget.mdx", ModelCategory::Abilities},
		{"abilities/spells/undead/DeathCoil/DeathCoilMissile.mdx", ModelCategory::Abilities},
		{"abilities/weapons/FireBallMissile/FireBallMissile.mdx", ModelCategory::Abilities},
		{"environment/UndeadBuildingFire/UndeadLargeBuildingFire1.mdx", ModelCategory::Environment},
		{"units/this/does/not/Exist.mdx", ModelCategory::Units},
	};

	for (const auto& entry : entries) {
		if (auto file = hierarchy.open_file(entry.path, Hierarchy::FileSource::all, {".mdx", ".mdl"}); !file) {
			std::println("Missing {}: {}", entry.path.string(), file.error());
		}
	}

	int exit_code;
	{
		QMainWindow window;
		window.resize(1280, 800);
		window.setWindowTitle("Vulkan model grid");

		auto* dock_manager = new ads::CDockManager(&window);

		auto* grid_host = new QWidget;
		auto* grid_layout = new QHBoxLayout(grid_host);
		grid_layout->setContentsMargins(0, 0, 0, 0);
		grid_layout->setSpacing(0);
		auto* grid = new ModelGridWidget(entries, grid_host);
		auto* bar = new QScrollBar(Qt::Vertical, grid_host);
		grid_layout->addWidget(grid, 1);
		grid_layout->addWidget(bar);
		QObject::connect(bar, &QScrollBar::valueChanged, grid, &ModelGridWidget::set_scroll_offset);
		QObject::connect(grid, &ModelGridWidget::scroll_changed, bar, &QScrollBar::setValue);
		QObject::connect(grid, &ModelGridWidget::content_height_changed, bar, [bar, grid](const int total) {
			bar->setRange(0, std::max(0, total - grid->height()));
			bar->setPageStep(grid->height());
			bar->setSingleStep(grid->cell_pixel_size());
		});
		QObject::connect(grid, &ModelGridWidget::clicked, [](const fs::path& path) {
			std::println("clicked {}", path.string());
		});

		auto* grid_dock = new ads::CDockWidget(dock_manager, "Model grid");
		grid_dock->setWidget(grid_host);
		dock_manager->addDockWidget(ads::CenterDockWidgetArea, grid_dock);

		auto* preview = new ModelGridWidget({{"units/human/HeroPaladin/HeroPaladin.mdx", ModelCategory::Map}}, nullptr, true);
		auto* preview_dock = new ads::CDockWidget(dock_manager, "Single preview");
		preview_dock->setWidget(preview);
		dock_manager->addDockWidget(ads::RightDockWidgetArea, preview_dock);

		auto* empty = new ModelGridWidget({}, nullptr, true);
		empty->set_empty_message("Select an asset to show a preview");
		auto* empty_dock = new ads::CDockWidget(dock_manager, "Empty preview");
		empty_dock->setWidget(empty);
		dock_manager->addDockWidget(ads::BottomDockWidgetArea, empty_dock, preview_dock->dockAreaWidget());

		QTimer stats;
		QObject::connect(&stats, &QTimer::timeout, [&] {
			std::println(
				"Vulkan model grid | validation errors: {}, warnings: {}",
				vk_context.validation_errors.load(),
				vk_context.validation_warnings.load()
			);
		});
		stats.start(1000);

		const QStringList arguments = QCoreApplication::arguments();
		if (const qsizetype index = arguments.indexOf("--quit-after"); index >= 0 && index + 1 < arguments.size()) {
			QTimer::singleShot(arguments[index + 1].toInt() * 1000, &QApplication::quit);
		}

		window.show();
		exit_code = QApplication::exec();
	}

	std::println(
		"Vulkan model grid finished with {} validation errors and {} warnings",
		vk_context.validation_errors.load(),
		vk_context.validation_warnings.load()
	);
	// Textures cached by the resource manager must go before the device does
	resource_manager.clear();
	shutdown_vulkan();
	return exit_code;
}

int run_vulkan_model_editor_test() {
	if (!initialize_vulkan()) {
		return EXIT_FAILURE;
	}

	const QStringList arguments = QCoreApplication::arguments();
	const fs::path path = [&] {
		const qsizetype index = arguments.indexOf("--vulkan-model-editor");
		if (index + 1 < arguments.size() && !arguments[index + 1].startsWith("--")) {
			return fs::path(arguments[index + 1].toStdString());
		}
		return fs::path("units/human/HeroPaladin/HeroPaladin.mdx");
	}();

	auto file = hierarchy.open_file(path, Hierarchy::FileSource::all, {".mdx", ".mdl"});
	if (!file) {
		std::println("Vulkan model editor: {}", file.error());
		return EXIT_FAILURE;
	}
	const auto mdx = std::make_shared<mdx::MDX>(file.value());

	int exit_code;
	{
		QMainWindow window;
		window.resize(1600, 900);
		window.setWindowTitle("Vulkan model editor");

		auto* viewport = new ModelEditorViewport(mdx, mdx->validate());
		// Keep the user's saved panel layout untouched: don't load or save it, use the default layout
		ImGui::GetIO().IniFilename = nullptr;
		viewport->build_default_layout = true;
		viewport->container = QWidget::createWindowContainer(viewport);
		window.setCentralWidget(viewport->container);

		QTimer stats;
		QObject::connect(&stats, &QTimer::timeout, [&] {
			std::println(
				"Vulkan model editor | validation errors: {}, warnings: {}",
				vk_context.validation_errors.load(),
				vk_context.validation_warnings.load()
			);
		});
		stats.start(1000);

		if (const qsizetype index = arguments.indexOf("--quit-after"); index >= 0 && index + 1 < arguments.size()) {
			QTimer::singleShot(arguments[index + 1].toInt() * 1000, &QApplication::quit);
		}

		window.show();
		exit_code = QApplication::exec();
	}

	std::println(
		"Vulkan model editor finished with {} validation errors and {} warnings",
		vk_context.validation_errors.load(),
		vk_context.validation_warnings.load()
	);
	resource_manager.clear();
	shutdown_vulkan();
	return exit_code;
}
