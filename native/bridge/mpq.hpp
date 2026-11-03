#pragma once

#include <algorithm>
#include <cstring>
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
#include "protection.hpp"

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

/// 逐段按大小写不敏感查找。地图的 slk 里写的是 Doodads\\Terrain\\，磁盘上可能是
/// Doodads/terrain/，在区分大小写的卷上直接拼路径会找不到。
inline fs::path resolve_path(const fs::path& root, const std::string& relative) {
	fs::path current = root;
	std::size_t start = 0;

	while (start <= relative.size()) {
		const std::size_t slash = relative.find_first_of("/\\", start);
		const std::string segment = relative.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
		if (segment.empty()) {
			if (slash == std::string::npos) {
				break;
			}
			start = slash + 1;
			continue;
		}

		const fs::path direct = current / segment;
		std::error_code ec;
		if (fs::exists(direct, ec)) {
			current = direct;
		} else {
			bool matched = false;
			for (const auto& entry : fs::directory_iterator(current, ec)) {
				const std::string name = entry.path().filename().string();
				if (name.size() == segment.size() && strncasecmp(name.c_str(), segment.c_str(), name.size()) == 0) {
					current = entry.path();
					matched = true;
					break;
				}
			}
			if (!matched) {
				return root / relative;
			}
		}

		if (slash == std::string::npos) {
			break;
		}
		start = slash + 1;
	}
	return current;
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
			// 归档里的路径是反斜杠，落盘要换成分隔符，不然子目录会变成一个带反斜杠的文件名
			std::string name = to_archive_path(data.cFileName);
			std::ranges::replace(name, '\\', '/');
			const fs::path entry = name;
			const fs::path target = directory / entry;
			if (target.filename().string().starts_with("(")) {
				continue;
			}
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

/// 导出 MPQ 时可选的处理，对应上游 MapProtectionDialog 的六个勾选项（不含「每次导出都问」）。
struct PackOptions {
	bool delete_editor_files = false;
	bool remove_metadata = false;
	bool obfuscate_script = false;
	bool junk_header_offset = false;
	bool encrypt_imports = false;
};

[[nodiscard]] inline bool pack_directory(const fs::path& directory, const fs::path& archive_path,
										 std::string& error, const PackOptions& options) {
	std::vector<fs::path> files;
	for (const auto& entry : fs::recursive_directory_iterator(directory)) {
		if (!entry.is_regular_file()) {
			continue;
		}
		const fs::path relative = fs::relative(entry.path(), directory);
		const std::string name = relative.string();
		if (!name.empty() && name.front() == '(') {
			continue;
		}
		if (options.delete_editor_files && protection::is_editor_file(relative)) {
			continue;
		}
		files.push_back(entry.path());
	}

	if (files.empty()) {
		error = "目录里没有文件：" + directory.string();
		return false;
	}

	std::error_code ec;
	fs::remove(archive_path, ec);

	uint32_t table_size = 1024;
	while (table_size < files.size() * 4) {
		table_size *= 2;
	}

	DWORD create_flags = MPQ_CREATE_ARCHIVE_V1;
	if (!options.remove_metadata) {
		create_flags |= MPQ_CREATE_LISTFILE;
	}

	HANDLE archive = nullptr;
	if (!SFileCreateArchive(archive_path.string().c_str(), create_flags, table_size, &archive)) {
		error = "创建归档失败：" + archive_path.string();
		return false;
	}

	const auto close_with = [&](const std::string& message) {
		SFileCloseArchive(archive);
		error = message;
		return false;
	};

	for (const auto& file : files) {
		const fs::path relative = fs::relative(file, directory);
		const std::string name = to_archive_path(relative.string());
		const std::string plain = relative.generic_string();
		const bool script = plain == "war3map.j" || plain == "war3map.lua";
		const bool minify = options.obfuscate_script && script;
		const bool encrypt = options.encrypt_imports && !protection::is_standard_game_file(relative);

		if (!minify && !encrypt) {
			if (!SFileAddFileEx(archive, file.string().c_str(), name.c_str(), MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING,
								MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_NEXT_SAME)) {
				return close_with("写入失败：" + name);
			}
			continue;
		}

		std::string data;
		if (minify) {
			std::ifstream in(file, std::ios::binary);
			data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
			data = protection::obfuscate_script(data, plain == "war3map.lua");
		} else {
			std::ifstream in(file, std::ios::binary);
			data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
		}

		HANDLE handle = nullptr;
		DWORD flags = MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING;
		if (encrypt) {
			flags |= MPQ_FILE_ENCRYPTED;
		}
		if (!SFileCreateFile(archive, name.c_str(), 0, static_cast<DWORD>(data.size()), 0, flags, &handle)
			|| !SFileWriteFile(handle, data.data(), static_cast<DWORD>(data.size()), MPQ_COMPRESSION_ZLIB)
			|| !SFileFinishFile(handle)) {
			return close_with("写入失败：" + name);
		}
	}

	SFileCloseArchive(archive);

	if (options.junk_header_offset && !protection::prepend_junk_header(archive_path)) {
		error = "写入文件头偏移失败";
		return false;
	}
	return true;
}

[[nodiscard]] inline bool pack_directory(const fs::path& directory, const fs::path& archive_path, std::string& error) {
	return pack_directory(directory, archive_path, error, PackOptions {});
}

}
