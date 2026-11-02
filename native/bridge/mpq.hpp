#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "binary_reader.hpp"
#include "no_init_allocator.hpp"

#include <StormLib.h>

namespace mpq {

namespace fs = std::filesystem;

inline std::mutex access_log_mutex;
inline std::ofstream access_log;

inline void start_access_log(const fs::path& path) {
	std::scoped_lock lock(access_log_mutex);
	access_log.open(path, std::ios::trunc);
}

inline void log_access(const std::string& archive_path) {
	std::scoped_lock lock(access_log_mutex);
	if (access_log) {
		access_log << archive_path << '\n';
	}
}

inline std::string to_archive_path(std::string_view path) {
	static constexpr std::string_view w3mod_prefix = "war3.w3mod:";

	std::string_view rest = path;
	if (rest.starts_with(w3mod_prefix)) {
		rest.remove_prefix(w3mod_prefix.size());
	}

	std::string result(rest);
	std::ranges::replace(result, ':', '\\');
	std::ranges::replace(result, '/', '\\');
	return result;
}

class MPQ {
  public:
	HANDLE handle = nullptr;

  private:
	mutable std::mutex mutex;

  public:

	MPQ() = default;

	explicit MPQ(const fs::path& path) {
		open(path);
	}

	~MPQ() {
		close();
	}

	MPQ(const MPQ&) = delete;
	MPQ& operator=(const MPQ&) = delete;

	MPQ(MPQ&& other) noexcept: handle(std::exchange(other.handle, nullptr)) {}
	MPQ& operator=(MPQ&& other) noexcept {
		if (this != &other) {
			close();
			handle = std::exchange(other.handle, nullptr);
		}
		return *this;
	}

	bool open(const fs::path& path) {
		this->close();

		if (!SFileOpenArchive(path.string().c_str(), 0, MPQ_OPEN_READ_ONLY, &this->handle)) {
			this->handle = nullptr;
			return false;
		}
		return true;
	}

	void close() {
		if (this->handle != nullptr) {
			SFileCloseArchive(this->handle);
			this->handle = nullptr;
		}
	}

	[[nodiscard]] bool is_open() const { return this->handle != nullptr; }

	bool add_patch(const fs::path& path) {
		if (this->handle == nullptr) {
			return false;
		}
		return SFileOpenPatchArchive(this->handle, path.string().c_str(), "", 0);
	}

	[[nodiscard]] std::vector<std::string> find_files(const std::string& pattern) const {
		std::vector<std::string> files;
		if (this->handle == nullptr) {
			return files;
		}

		SFILE_FIND_DATA data;
		HANDLE found = SFileFindFirstFile(this->handle, to_archive_path(pattern).c_str(), &data, nullptr);
		if (found == nullptr) {
			return files;
		}

		files.emplace_back(data.cFileName);
		while (SFileFindNextFile(found, &data)) {
			files.emplace_back(data.cFileName);
		}
		SFileFindClose(found);
		return files;
	}

	bool unpack(const fs::path& directory) const {
		if (this->handle == nullptr) {
			return false;
		}

		fs::create_directories(directory);

		SFILE_FIND_DATA data;
		HANDLE found = SFileFindFirstFile(this->handle, "*", &data, nullptr);
		if (found == nullptr) {
			return false;
		}

		do {
			const fs::path entry = to_archive_path(data.cFileName);
			const fs::path target = directory / entry;
			fs::create_directories(target.parent_path());
			SFileExtractFile(this->handle, data.cFileName, target.string().c_str(), SFILE_OPEN_FROM_MPQ);
		} while (SFileFindNextFile(found, &data));

		SFileFindClose(found);
		return true;
	}

	[[nodiscard]] std::expected<BinaryReader, std::string> open_file(const fs::path& path) const {
		const std::string archive_path = to_archive_path(path.string());

		const std::scoped_lock lock(this->mutex);

		HANDLE file = nullptr;
		if (!SFileOpenFileEx(this->handle, archive_path.c_str(), SFILE_OPEN_FROM_MPQ, &file)) {
			return std::unexpected(std::format("{} not in archive", archive_path));
		}

		const DWORD size = SFileGetFileSize(file, nullptr);
		std::vector<uint8_t, default_init_allocator<uint8_t>> buffer(size);

		DWORD read = 0;
		const bool ok = SFileReadFile(file, buffer.data(), size, &read, nullptr);
		SFileCloseFile(file);

		if (!ok) {
			return std::unexpected(std::format("failed reading {}", archive_path));
		}

		log_access(archive_path);

		buffer.resize(read);
		return BinaryReader(std::move(buffer));
	}

	[[nodiscard]] bool file_exists(const fs::path& path) const {
		const std::string archive_path = to_archive_path(path.string());
		const std::scoped_lock lock(this->mutex);
		return SFileHasFile(this->handle, archive_path.c_str());
	}

	[[nodiscard]] bool file_exists_locally(const fs::path& path) const {
		return file_exists(path);
	}
};

}
