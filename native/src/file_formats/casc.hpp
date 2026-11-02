#pragma once


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
#include "no_init_allocator.hpp"
#include "binary_reader.hpp"
#include <CascLib.h>

namespace fs = std::filesystem;

// A thin wrapper around CascLib https://github.com/ladislav-zezula/CascLib
namespace casc {
	class CASC {
	  public:
		HANDLE handle = nullptr;

		CASC() = default;

		explicit CASC(const fs::path& path) {
			open(path);
		}

		~CASC() {
			close();
		}

		CASC(CASC&& move)
		noexcept {
			handle = move.handle;
			move.handle = nullptr;
		}
		CASC(const CASC&) = default;
		CASC& operator=(const CASC&) = delete;
		CASC& operator=(CASC&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
			return *this;
		}

		bool open(const fs::path& path) {
			if (handle != nullptr)
				close();
			const bool opened = CascOpenStorage(path.c_str(), CASC_LOCALE_ALL, &handle);
			if (!opened) {
				std::print("Error opening {} with error: {}\n", path.string(), GetCascError());
			}
			return opened;
		}

		void close() {
			CascCloseStorage(handle);
			handle = nullptr;
		}

		std::vector<std::string> find_files(const std::string& path) const {
			std::vector<std::string> files;

			CASC_FIND_DATA data;
			const HANDLE found = CascFindFirstFile(handle, path.c_str(), &data, L"");

			if (found == INVALID_HANDLE_VALUE) {
				return {};
			}

			files.push_back(data.szFileName);

			while (CascFindNextFile(found, &data)) {
				files.push_back(data.szFileName);
			}

			return files;
		}

		[[nodiscard]] std::expected<BinaryReader, std::string> open_file(const fs::path& path) const {
			HANDLE file_handle = nullptr;
			const bool opened = CascOpenFile(handle, path.string().c_str(), 0, CASC_OPEN_BY_NAME, &file_handle);
			if (!opened) {
				return std::unexpected(std::format("Error opening {} with error: {}\n", path.string(), GetCascError()));
			}

			const u32 size = CascGetFileSize(file_handle, nullptr);
			std::vector<u8, default_init_allocator<u8>> buffer(size);

			#ifdef _MSC_VER
			unsigned long bytes_read;
			#else
			unsigned bytes_read;
			#endif
			const bool success = CascReadFile(file_handle, buffer.data(), size, &bytes_read);
			CascCloseFile(file_handle);
			if (!success) {
				return std::unexpected(std::format("Error failed to read file: {}\n", GetCascError()));
			}
			return BinaryReader(std::move(buffer));
		}

		bool file_exists(const fs::path& path) const {
			HANDLE file_handle = nullptr;
			const bool exists = CascOpenFile(handle, path.string().c_str(), 0, CASC_OPEN_BY_NAME, &file_handle);
			if (exists) {
				CascCloseFile(file_handle);
			}
			return exists;
		}

		/// Whether the file's data is actually downloaded/available locally, as opposed to merely
		/// being present in the storage index (which lists all locales regardless of what is installed).
		bool file_exists_locally(const fs::path& path) const {
			CASC_FIND_DATA data;
			const HANDLE found = CascFindFirstFile(handle, path.string().c_str(), &data, L"");
			if (found == INVALID_HANDLE_VALUE) {
				return false;
			}
			const bool available = data.bFileAvailable;
			CascFindClose(found);
			return available;
		}
	};
} // namespace casc