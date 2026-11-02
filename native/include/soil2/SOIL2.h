#pragma once

#include <cstdint>

inline constexpr int SOIL_LOAD_AUTO = 0;
inline constexpr int SOIL_LOAD_L = 1;
inline constexpr int SOIL_LOAD_LA = 2;
inline constexpr int SOIL_LOAD_RGB = 3;
inline constexpr int SOIL_LOAD_RGBA = 4;

inline unsigned char* SOIL_load_image_from_memory(const unsigned char*, int, int* width, int* height, int* channels, int) {
	if (width) {
		*width = 0;
	}
	if (height) {
		*height = 0;
	}
	if (channels) {
		*channels = 0;
	}
	return nullptr;
}

inline const char* SOIL_last_result() {
	return "SOIL 未启用，非 BLP 贴图无法解码";
}
