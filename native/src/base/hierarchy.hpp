#pragma once


#include <QSettings>


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
#include "types.hpp"
#include "json.hpp"
#include "binary_reader.hpp"
#include "casc.hpp"
#include "mpq.hpp"
#include "no_init_allocator.hpp"
#include "utilities.hpp"

using namespace std::literals::string_literals;
namespace fs = std::filesystem;

class Hierarchy {
  public:
	enum class FileSource {
		none = 0,
		overrides = 1 << 0,
		imports = 1 << 1,
		local_files = 1 << 2,
		casc = 1 << 3,
		all = overrides | imports | local_files | casc
	};

	char tileset = 'L';
	std::string locale = "enus";
	mpq::MPQ game_data;
	json::JSON aliases;

	fs::path map_directory;
	fs::path warcraft_directory;
	fs::path root_directory;

	bool ptr = false;
	/// Use the Reforged/Definitive mode render paths and assets.
	bool remastered = true;
	/// Load Definitive mode (_de.w3mod) assets instead of Reforged (_hd.w3mod) assets.
	bool definitive = false;
	bool teen = false;
	bool allow_local_files = true;

	friend constexpr FileSource operator&(FileSource a, FileSource b);
	friend constexpr FileSource operator|(FileSource a, FileSource b);
	friend constexpr FileSource operator~(FileSource a);

	Hierarchy() {
		QSettings war3reg("HKEY_CURRENT_USER\\Software\\Blizzard Entertainment\\Warcraft III", QSettings::NativeFormat);
		allow_local_files = war3reg.value("Allow Local Files", 0).toInt() != 0;
	}

	/// The folder holding the assets for the current HD graphics mode
	[[nodiscard]]
	std::string_view graphics_mod() const {
		return definitive ? "_de.w3mod" : "_hd.w3mod";
	}

	bool open_casc(const fs::path& directory) {
		QSettings settings;
		ptr = settings.value("flavour", "Retail").toString() == "PTR";
		const QString graphics = settings.value("graphics", "SD").toString();
		remastered = graphics != "SD";
		definitive = graphics == "DE";
		teen = settings.value("teen", "False").toString() == "True";

		warcraft_directory = directory;

		bool open = game_data.open(warcraft_directory / (ptr ? ":w3t" : ":w3"));
		root_directory = warcraft_directory / (ptr ? "_ptr_" : "_retail_");

		if (!open) {
			return false;
		}

		for (const auto& potential_locale :
			 {"dede", "enus", "eses", "esmx", "frfr", "itit", "kokr", "plpl", "ptbr", "ruru", "zhcn", "zhtw"}) {
			if (game_data.file_exists_locally(std::format("war3.w3mod:_locales/{}.w3mod:config.txt", potential_locale))) {
				locale = potential_locale;
				break;
			}
		}

		aliases.load(open_file("filealiases.json").value());
		return true;
	}

	constexpr bool has_flag(Hierarchy::FileSource value, Hierarchy::FileSource flag) const {
		using U = std::underlying_type_t<Hierarchy::FileSource>;
		return (static_cast<U>(value) & static_cast<U>(flag)) != 0;
	}

	[[nodiscard]]
	/// Loads the file in the following order:
	/// 1. Editor overrides (data/overrides folder)
	/// 2. Map imports
	/// 3. Local files (if enabled)
	/// 4. Game casc archive (handles sd, hd, definitive and teen modes)
	auto open_file(
		const fs::path& path,
		const FileSource sources = FileSource::all,
		std::initializer_list<std::string_view> extensions = {}
	) const -> std::expected<BinaryReader, std::string> {
		const auto path_str = path.generic_string();

		const bool overrides = has_flag(sources, FileSource::overrides);
		const bool imports = has_flag(sources, FileSource::imports);
		const bool local = has_flag(sources, FileSource::local_files);
		const bool casc = has_flag(sources, FileSource::casc);

		// The game ignores the extension it is given and instead tries a fixed set of
		// extensions for each location before moving on to the next location.
		std::vector<std::string> variants;
		if (extensions.size() == 0) {
			variants.push_back(path_str);
		} else {
			for (const auto& extension : extensions) {
				fs::path variant = path;
				variant.replace_extension(extension);
				variants.push_back(variant.generic_string());
			}
		}

#define TRY_OPEN(expr) \
	for (const auto& p : variants) { \
		if (auto file = (expr); file) { \
			return file; \
		} \
	}

		if (overrides) {
			TRY_OPEN(read_file(fs::path("data/overrides") / p));
		}

		if (imports && remastered) {
			TRY_OPEN(map_file_read(std::format("{}:_tilesets/{}.w3mod/{}", graphics_mod(), tileset, p)));
		}

		if (imports && remastered) {
			TRY_OPEN(map_file_read(std::format("{}:_locales/{}.w3mod/{}", graphics_mod(), locale, p)));
		}

		if (imports && remastered && teen) {
			TRY_OPEN(map_file_read(std::format("{}:_teen.w3mod/{}", graphics_mod(), p)));
		}

		if (imports && remastered) {
			TRY_OPEN(map_file_read(std::format("{}/{}", graphics_mod(), p)));
		}

		if (imports) {
			TRY_OPEN(map_file_read(std::format("_tilesets/{}.w3mod/{}", tileset, p)));
		}

		if (imports) {
			TRY_OPEN(map_file_read(std::format("_locales/{}.w3mod/{}", locale, p)));
		}

		if (imports && teen) {
			TRY_OPEN(map_file_read(std::format("_teen.w3mod/{}", p)));
		}

		if (imports) {
			TRY_OPEN(map_file_read(p));
		}

		if (local && allow_local_files) {
			// TODO, probably also follows: tilesets, locales, teen, base.
			TRY_OPEN(read_file(root_directory / p));
		}

		if (casc && remastered) {
			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:{}:_tilesets/{}.w3mod:{}", graphics_mod(), tileset, p)));
		}

		if (casc && remastered) {
			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:{}:_locales/{}.w3mod:{}", graphics_mod(), locale, p)));
		}

		if (casc && remastered && teen) {
			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:{}:_teen.w3mod:{}", graphics_mod(), p)));
		}

		if (casc && remastered) {
			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:{}:{}", graphics_mod(), p)));
		}

		if (casc) {
			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:_tilesets/{}.w3mod:{}", tileset, p)));
		}

		if (casc) {
			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:_locales/{}.w3mod:{}", locale, p)));
		}
		if (casc && teen) {
			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:_teen.w3mod:{}", p)));
		}

		if (casc) {
			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:{}", p)));
		}

		if (casc) {
			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:_deprecated.w3mod:{}", p)));
		}

#undef TRY_OPEN

		for (const auto& p : variants) {
			if (aliases.exists(p)) {
				return open_file(aliases.alias(p), sources, extensions);
			}
		}

		return std::unexpected(std::format("{} could not be found in the hierarchy", path_str));
	}

	[[nodiscard]]
	bool file_exists(
		const fs::path& path,
		const FileSource sources = FileSource::all,
		const std::initializer_list<std::string_view> extensions = {}
	) const {
		if (path.empty()) {
			return false;
		}

		const bool overrides = has_flag(sources, FileSource::overrides);
		const bool imports = has_flag(sources, FileSource::imports);
		const bool local = has_flag(sources, FileSource::local_files);
		const bool casc = has_flag(sources, FileSource::casc);

		const auto variant_exists = [&](const fs::path& variant) {
			const auto path_str = variant.generic_string();
			return (overrides && fs::exists("data/overrides" / variant))
				|| (imports && remastered && map_file_exists(std::format("{}:_tilesets/{}.w3mod/{}", graphics_mod(), tileset, path_str)))
				|| (imports && remastered && map_file_exists(std::format("{}:_locales/{}.w3mod/{}", graphics_mod(), locale, path_str)))
				|| (imports && remastered && teen && map_file_exists(std::format("{}:_teen.w3mod/{}", graphics_mod(), path_str)))
				|| (imports && remastered && map_file_exists(std::format("{}/{}", graphics_mod(), path_str)))
				|| (imports && map_file_exists(std::format("_tilesets/{}.w3mod/{}", tileset, path_str)))
				|| (imports && map_file_exists(std::format("_locales/{}.w3mod/{}", locale, path_str)))
				|| (imports && teen && map_file_exists(std::format("_teen.w3mod/{}", path_str))) || (imports && map_file_exists(variant))
				|| (local && allow_local_files && fs::exists(root_directory / variant))
				|| (casc && remastered && game_data.file_exists(std::format("war3.w3mod:{}:_tilesets/{}.w3mod:{}", graphics_mod(), tileset, path_str)))
				|| (casc && remastered && game_data.file_exists(std::format("war3.w3mod:{}:_locales/{}.w3mod:{}", graphics_mod(), locale, path_str)))
				|| (casc && remastered && teen && game_data.file_exists(std::format("war3.w3mod:{}:_teen.w3mod:{}", graphics_mod(), path_str)))
				|| (casc && remastered && game_data.file_exists(std::format("war3.w3mod:{}:{}", graphics_mod(), path_str)))
				|| (casc && game_data.file_exists(std::format("war3.w3mod:_tilesets/{}.w3mod:{}", tileset, path_str)))
				|| (casc && game_data.file_exists(std::format("war3.w3mod:_locales/{}.w3mod:{}", locale, path_str)))
				|| (casc && teen && game_data.file_exists(std::format("war3.w3mod:_teen.w3mod:{}", path_str)))
				|| (casc && game_data.file_exists(std::format("war3.w3mod:{}", path_str)))
				|| (casc && game_data.file_exists(std::format("war3.w3mod:_deprecated.w3mod:{}", path_str)));
		};

		std::vector<fs::path> variants;
		if (extensions.size() == 0) {
			variants.push_back(path);
		} else {
			for (const auto& extension : extensions) {
				fs::path variant = path;
				variant.replace_extension(extension);
				variants.push_back(variant);
			}
		}

		for (const auto& variant : variants) {
			if (variant_exists(variant)) {
				return true;
			}
		}

		for (const auto& variant : variants) {
			const auto path_str = variant.generic_string();
			if (aliases.exists(path_str)) {
				return file_exists(aliases.alias(path_str), sources, extensions);
			}
		}

		return false;
	}

	/// Whether the file exists in the game data lookup, ignoring map imports
	[[nodiscard]]
	bool game_file_exists(const fs::path& path) const {
		return file_exists(path, FileSource::all & ~FileSource::imports);
	}

	[[nodiscard]]
	auto map_file_read(const fs::path& path) const -> std::expected<BinaryReader, std::string> {
		return read_file(map_directory / path);
	}

	/// source somewhere on disk, destination relative to the map
	void map_file_add(const fs::path& source, const fs::path& destination) const {
		fs::copy_file(source, map_directory / destination, fs::copy_options::overwrite_existing);
	}

	void map_file_write(const fs::path& path, const std::span<const u8> data) const {
		std::ofstream outfile(map_directory / path, std::ios::binary);

		if (!outfile) {
			throw std::runtime_error("Error writing file " + path.string());
		}

		outfile.write(reinterpret_cast<char const*>(data.data()), data.size());
	}

	void map_file_remove(const fs::path& path) const {
		fs::remove(map_directory / path);
	}

	bool map_file_exists(const fs::path& path) const {
		return fs::exists(map_directory / path);
	}

	void map_file_rename(const fs::path& original, const fs::path& renamed) const {
		fs::rename(map_directory / original, map_directory / renamed);
	}
};

using U = std::underlying_type_t<Hierarchy::FileSource>;

constexpr Hierarchy::FileSource operator|(Hierarchy::FileSource a, Hierarchy::FileSource b) {
	using U = std::underlying_type_t<Hierarchy::FileSource>;
	return static_cast<Hierarchy::FileSource>(static_cast<U>(a) | static_cast<U>(b));
}

constexpr Hierarchy::FileSource operator&(Hierarchy::FileSource a, Hierarchy::FileSource b) {
	using U = std::underlying_type_t<Hierarchy::FileSource>;
	return static_cast<Hierarchy::FileSource>(static_cast<U>(a) & static_cast<U>(b));
}

constexpr Hierarchy::FileSource operator^(Hierarchy::FileSource a, Hierarchy::FileSource b) {
	using U = std::underlying_type_t<Hierarchy::FileSource>;
	return static_cast<Hierarchy::FileSource>(static_cast<U>(a) ^ static_cast<U>(b));
}

constexpr Hierarchy::FileSource operator~(Hierarchy::FileSource a) {
	using U = std::underlying_type_t<Hierarchy::FileSource>;
	return static_cast<Hierarchy::FileSource>(~static_cast<U>(a));
}

constexpr Hierarchy::FileSource& operator|=(Hierarchy::FileSource& a, Hierarchy::FileSource b) {
	a = a | b;
	return a;
}

constexpr Hierarchy::FileSource& operator&=(Hierarchy::FileSource& a, Hierarchy::FileSource b) {
	a = a & b;
	return a;
}

constexpr Hierarchy::FileSource& operator^=(Hierarchy::FileSource& a, Hierarchy::FileSource b) {
	a = a ^ b;
	return a;
}

inline Hierarchy hierarchy;
