#pragma once

#include <cstddef>
#include <cstdint>

using HANDLE = void*;
using DWORD = std::uint32_t;
using LPCSTR = const char*;
using LPCWSTR = const wchar_t*;

inline HANDLE const INVALID_HANDLE_VALUE = reinterpret_cast<HANDLE>(-1);
inline constexpr DWORD CASC_LOCALE_ALL = 0xFFFFFFFF;
inline constexpr DWORD CASC_OPEN_BY_NAME = 0x00000004;

struct CASC_FIND_DATA {
	DWORD dwFileSize1;
	DWORD dwFileSize2;
	char szFileName[260];
	char szPlainName[260];
	void* pvFilePtr1;
	void* pvFilePtr2;
	bool bFileAvailable;
};

inline bool CascOpenStorage(LPCSTR, DWORD, HANDLE*) {
	return false;
}

inline bool CascCloseStorage(HANDLE) {
	return true;
}

inline bool CascOpenFile(HANDLE, LPCSTR, DWORD, DWORD, HANDLE*) {
	return false;
}

inline bool CascCloseFile(HANDLE) {
	return true;
}

inline DWORD CascGetFileSize(HANDLE, DWORD*) {
	return 0;
}

inline bool CascReadFile(HANDLE, void*, DWORD, DWORD* bytes_read) {
	if (bytes_read) {
		*bytes_read = 0;
	}
	return false;
}

inline HANDLE CascFindFirstFile(HANDLE, LPCSTR, CASC_FIND_DATA*, LPCWSTR) {
	return INVALID_HANDLE_VALUE;
}

inline bool CascFindNextFile(HANDLE, CASC_FIND_DATA*) {
	return false;
}

inline bool CascFindClose(HANDLE) {
	return true;
}

inline const char* GetCascError() {
	return "CASC 未启用（本工程只支持 MPQ）";
}
