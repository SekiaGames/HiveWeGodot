#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

inline constexpr int SOIL_LOAD_AUTO = 0;
inline constexpr int SOIL_LOAD_L = 1;
inline constexpr int SOIL_LOAD_LA = 2;
inline constexpr int SOIL_LOAD_RGB = 3;
inline constexpr int SOIL_LOAD_RGBA = 4;

namespace soil_detail {

inline void copy_bgr(unsigned char* dst, const unsigned char* src, const int channels) {
	if (channels >= 3) {
		dst[0] = src[2];
		dst[1] = src[1];
		dst[2] = src[0];
		if (channels == 4) {
			dst[3] = src[3];
		}
	} else {
		dst[0] = src[0];
	}
}

/* 只解 TGA：War3 的路径贴图和一部分 UI 贴图用这个格式，BLP 走 HiveWE 自己的解码器 */
inline unsigned char* load_tga(
	const unsigned char* data, const int length, int* out_width, int* out_height, int* out_channels
) {
	if (length < 18) {
		return nullptr;
	}

	const int id_length = data[0];
	const int color_map_type = data[1];
	const int image_type = data[2];
	const int width = data[12] | (data[13] << 8);
	const int height = data[14] | (data[15] << 8);
	const int bpp = data[16];
	const int descriptor = data[17];

	if (color_map_type != 0 || width <= 0 || height <= 0) {
		return nullptr;
	}
	if (image_type != 2 && image_type != 3 && image_type != 10 && image_type != 11) {
		return nullptr;
	}
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		return nullptr;
	}

	const int channels = bpp / 8;
	const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
	unsigned char* pixels = static_cast<unsigned char*>(std::malloc(pixel_count * channels));
	if (pixels == nullptr) {
		return nullptr;
	}

	const unsigned char* cursor = data + 18 + id_length;
	const unsigned char* end = data + length;
	const bool rle = image_type == 10 || image_type == 11;
	const bool top_origin = (descriptor & 0x20) != 0;

	if (!rle) {
		if (cursor + pixel_count * channels > end) {
			std::free(pixels);
			return nullptr;
		}
		for (size_t i = 0; i < pixel_count; ++i) {
			copy_bgr(pixels + i * channels, cursor + i * channels, channels);
		}
	} else {
		size_t written = 0;
		while (written < pixel_count && cursor < end) {
			const unsigned char packet = *cursor++;
			const int count = (packet & 0x7F) + 1;

			if ((packet & 0x80) != 0) {
				if (cursor + channels > end) {
					break;
				}
				const unsigned char* source = cursor;
				cursor += channels;
				for (int i = 0; i < count && written < pixel_count; ++i, ++written) {
					copy_bgr(pixels + written * channels, source, channels);
				}
			} else {
				for (int i = 0; i < count && written < pixel_count; ++i, ++written) {
					if (cursor + channels > end) {
						break;
					}
					copy_bgr(pixels + written * channels, cursor, channels);
					cursor += channels;
				}
			}
		}
	}

	if (!top_origin) {
		const size_t row_size = static_cast<size_t>(width) * channels;
		unsigned char* scratch = static_cast<unsigned char*>(std::malloc(row_size));
		if (scratch != nullptr) {
			for (int y = 0; y < height / 2; ++y) {
				unsigned char* top = pixels + static_cast<size_t>(y) * row_size;
				unsigned char* bottom = pixels + static_cast<size_t>(height - 1 - y) * row_size;
				std::memcpy(scratch, top, row_size);
				std::memcpy(top, bottom, row_size);
				std::memcpy(bottom, scratch, row_size);
			}
			std::free(scratch);
		}
	}

	*out_width = width;
	*out_height = height;
	*out_channels = channels;
	return pixels;
}

}

inline unsigned char* SOIL_load_image_from_memory(
	const unsigned char* data, const int length, int* width, int* height, int* channels, int
) {
	if (width != nullptr) {
		*width = 0;
	}
	if (height != nullptr) {
		*height = 0;
	}
	if (channels != nullptr) {
		*channels = 0;
	}
	if (data == nullptr) {
		return nullptr;
	}
	return soil_detail::load_tga(data, length, width, height, channels);
}

inline const char* SOIL_last_result() {
	return "只支持 TGA，其余格式走 HiveWE 自带解码器";
}
