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
#include "binary_reader.hpp"
#include "hierarchy.hpp"

struct Sound {
	std::string name;
	std::string file;
	std::string eax_effect;
	// int flags;
	bool looping;
	bool is_3d;
	bool stop_out_of_range;
	bool music;

	int fade_in_rate;
	int fade_out_rate;
	int volume;
	float pitch;
	float pitch_variance;
	int priority;
	int channel;
	float min_distance;
	float max_distance;
	float distance_cutoff;
	float cone_inside;
	float cone_outside;
	int cone_outside_volume;
	float cone_orientation_x;
	float cone_orientation_y;
	float cone_orientation_z;
};

class Sounds {
  public:
	std::vector<Sound> sounds;

	void load() {
		BinaryReader reader = hierarchy.map_file_read("war3map.w3s").value();

		int version = reader.read<u32>();
		if (version != 1 && version != 2 && version != 3) {
			std::cout << "Unknown war3map.w3s version: " << version << " Attempting to load but may crash\n";
		}

		sounds.resize(reader.read<u32>());
		for (auto& i : sounds) {
			i.name = reader.read_c_string();
			i.file = reader.read_c_string();
			i.eax_effect = reader.read_c_string();
			u32 flags = reader.read<u32>();
			i.looping = flags & 0b00000001;
			i.is_3d = flags & 0b00000010;
			i.stop_out_of_range = flags & 0b00000100;
			i.music = flags & 0b00001000;

			i.fade_in_rate = reader.read<u32>();
			i.fade_out_rate = reader.read<u32>();
			i.volume = reader.read<u32>();
			i.pitch = reader.read<float>();
			i.pitch_variance = reader.read<float>();
			i.priority = reader.read<u32>();
			i.channel = reader.read<u32>();
			i.min_distance = reader.read<float>();
			i.max_distance = reader.read<float>();
			i.distance_cutoff = reader.read<float>();
			i.cone_inside = reader.read<float>();
			i.cone_outside = reader.read<float>();
			i.cone_outside_volume = reader.read<u32>();
			i.cone_orientation_x = reader.read<float>();
			i.cone_orientation_y = reader.read<float>();
			i.cone_orientation_z = reader.read<float>();

			if (version >= 2) {
				reader.advance_c_string();
				reader.advance_c_string();
				reader.advance_c_string();
				reader.advance(4); // int
				reader.advance_c_string();
				reader.advance(4); // int
				reader.advance_c_string();
				reader.advance(4); // int

				reader.advance_c_string();
				reader.advance_c_string();
				reader.advance_c_string();
				reader.advance_c_string();
				if (version >= 3) {
					reader.advance(4); // int
				}
			}
		}
	}

	void save() const {}
};
