#!/usr/bin/env python3

import argparse
import os
import re

STD_HEADERS = """#include <algorithm>
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
#include <vector>"""

RE_MODULE = re.compile(r'^export module\s+([\w.]+)\s*;\s*$', re.M)
RE_MODULE_IMPL = re.compile(r'^module\s+([\w.]+)\s*;\s*$')
RE_IMPORT_STD = re.compile(r'^import std(?:\.compat)?;\s*$')
RE_IMPORT_ANGLE = re.compile(r'^import\s+<([^>]+)>;\s*$')
RE_IMPORT_QUOTED = re.compile(r'^import\s+"([^"]+)"\s*;\s*$')
RE_IMPORT_NAMED = re.compile(r'^import\s+([\w.]+)\s*;\s*$')
RE_MOC = re.compile(r'^#include "[\w/]+\.moc"\n?', re.M)
RE_EMIT = re.compile(r'^(\s*)emit (?!dataChanged)(\w+)\((.*)\);$', re.M)
RE_SLOTS = re.compile(r'^(\s*)(?:public|protected|private)?\s*slots:\s*$', re.M)
RE_EXEC_PAR = re.compile(r'std::(for_each|sort|transform|accumulate)\(\s*std::execution::\w+\s*,\s*')

PATCHES = [
    (
        'physical_device.enable_extensions_if_present({VK_KHR_PRESENT_ID_EXTENSION_NAME,'
        ' VK_KHR_PRESENT_WAIT_EXTENSION_NAME})',
        'physical_device.enable_extensions_if_present(std::vector<const char*>'
        ' {VK_KHR_PRESENT_ID_EXTENSION_NAME, VK_KHR_PRESENT_WAIT_EXTENSION_NAME})',
    ),
    (
        '  signals:\n\tvoid size_changed(glm::ivec2 size);\n\tvoid selection_changed();',
        '\tstd::function<void(glm::ivec2)> size_changed;\n\tstd::function<void()> selection_changed;',
    ),
    (
        '  signals:\n\tvoid angle_changed();\n\tvoid scale_changed();\n\tvoid position_changed();'
        '\n\tvoid request_doodad_select(std::string id);',
        '\tstd::function<void()> angle_changed;\n\tstd::function<void()> scale_changed;'
        '\n\tstd::function<void()> position_changed;\n\tstd::function<void(std::string)> request_doodad_select;',
    ),
    (
        '  signals:\n\tvoid regions_changed();',
        '\tstd::function<void()> regions_changed;',
    ),
    (
        '  signals:\n\tvoid minimap_changed(Texture minimap);\n\tvoid tileset_changed();',
        '\tstd::function<void(Texture)> minimap_changed;\n\tstd::function<void()> tileset_changed;',
    ),
    (
        '\tbool is_mdx(const BinaryReader& reader) {',
        '\tinline bool is_mdx(const BinaryReader& reader) {',
    ),
    (
        '\textern const std::unordered_map<int, std::string> replaceable_id_to_texture {',
        '\tinline const std::unordered_map<int, std::string> replaceable_id_to_texture {',
    ),
    (
        '\tRenderManager() {\n\t\tskinned_mesh_globals.init();',
        '\tRenderManager() {\n#if HIVE_HEADLESS\n\t\treturn;\n#endif\n\t\tskinned_mesh_globals.init();',
    ),
    (
        '\t\t\t\t\tauto res = hierarchy.open_file(path);\n'
        '\t\t\t\t\treturn std::move(res.value().buffer);',
        '\t\t\t\t\tauto res = hierarchy.open_file(path);\n'
        '\t\t\t\t\tif (!res) {\n'
        '\t\t\t\t\t\treturn std::vector<u8, default_init_allocator<u8>>();\n'
        '\t\t\t\t\t}\n'
        '\t\t\t\t\treturn std::move(res.value().buffer);',
    ),
    (
        '\t\t\tif (auto result = load_hierarchy(path, source); !result) {\n'
        '\t\t\t\tthrow std::runtime_error(result.error());\n'
        '\t\t\t}',
        '\t\t\tif (auto result = load_hierarchy(path, source); !result) {\n'
        '#if HIVE_HEADLESS\n'
        '\t\t\t\tstd::print(stderr, "[数据缺失] {}\\n", result.error());\n'
        '\t\t\t\treturn;\n'
        '#else\n'
        '\t\t\t\tthrow std::runtime_error(result.error());\n'
        '#endif\n'
        '\t\t\t}',
    ),
    (
        'Buffer create_buffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const bool host_visible) {',
        'Buffer create_buffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const bool host_visible) {\n'
        '#if HIVE_HEADLESS\n\treturn Buffer {};\n#endif',
    ),
    (
        '\tconst bool array_view = false\n) {',
        '\tconst bool array_view = false\n) {\n#if HIVE_HEADLESS\n\treturn Image {};\n#endif',
    ),
    (
        'Image upload_texture(const TextureData& data, const bool generate_mips = true) {',
        'Image upload_texture(const TextureData& data, const bool generate_mips = true) {\n'
        '#if HIVE_HEADLESS\n\treturn Image {};\n#endif',
    ),
    (
        '\t\t\tground_textures.push_back(resource_manager.load<GroundTexture>(texture.file_path).value());',
        '\t\t\tauto loaded_texture = resource_manager.load<GroundTexture>(texture.file_path);\n'
        '\t\t\tif (!loaded_texture) {\n'
        '\t\t\t\tstd::println(stderr, "[贴图缺失] {}", loaded_texture.error());\n'
        '\t\t\t}\n'
        '\t\t\tground_textures.push_back(std::move(loaded_texture).value());',
    ),
    (
        '\tuint32_t add(const VkImageView view, const VkSampler sampler) {\n\t\tstd::lock_guard lock(mutex);',
        '\tuint32_t add(const VkImageView view, const VkSampler sampler) {\n'
        '#if HIVE_HEADLESS\n'
        '\t\tstd::lock_guard headless_lock(mutex);\n'
        '\t\treturn next_slot++;\n'
        '#endif\n'
        '\t\tstd::lock_guard lock(mutex);',
    ),
    (
        'void destroy_buffer_deferred(const Buffer& buffer) {',
        'void destroy_buffer_deferred(const Buffer& buffer) {\n#if HIVE_HEADLESS\n\treturn;\n#endif',
    ),
    (
        'Buffer create_device_buffer(const std::span<const std::byte> data, const VkBufferUsageFlags usage) {',
        'Buffer create_device_buffer(const std::span<const std::byte> data, const VkBufferUsageFlags usage) {\n#if HIVE_HEADLESS\n\treturn Buffer {};\n#endif',
    ),
    (
        'void upload_buffer(const Buffer& buffer, const std::span<const std::byte> data, const VkDeviceSize offset = 0) {',
        'void upload_buffer(const Buffer& buffer, const std::span<const std::byte> data, const VkDeviceSize offset = 0) {\n#if HIVE_HEADLESS\n\treturn;\n#endif',
    ),
    (
        'void destroy_image_deferred(const Image& image) {',
        'void destroy_image_deferred(const Image& image) {\n#if HIVE_HEADLESS\n\treturn;\n#endif',
    ),
    (
        '\t\tBinaryReader reader = hierarchy.open_file(path, Hierarchy::FileSource::all, {".mdx", ".mdl"}).value();',
        '\t\tauto opened = hierarchy.open_file(path, Hierarchy::FileSource::all, {".mdx", ".mdl"});\n\t\tif (!opened) {\n#if HIVE_HEADLESS\n\t\t\tstd::println(stderr, "[悬崖模型缺失] {}", opened.error());\n\t\t\treturn;\n#else\n\t\t\tthrow std::runtime_error(opened.error());\n#endif\n\t\t}\n\t\tBinaryReader reader = std::move(*opened);',
    ),
    (
        '\tvoid update(const VkExtent2D extent, const std::span<const u8> pixels) {',
        '\tvoid update(const VkExtent2D extent, const std::span<const u8> pixels) {\n#if HIVE_HEADLESS\n\t\treturn;\n#endif',
    ),
    (
        '\tstatic void upload(const std::span<const Upload> uploads) {',
        '\tstatic void upload(const std::span<const Upload> uploads) {\n#if HIVE_HEADLESS\n\t\treturn;\n#endif',
    ),
    (
        '\tint map_version;\n\tint editor_version;\n\tint game_version_major;\n\tint game_version_minor;\n\tint game_version_patch;\n\tint game_version_build;',
        '\tint map_version = 1;\n\tint editor_version = 6059;\n\tint game_version_major = 1;\n\tint game_version_minor = 27;\n\tint game_version_patch = 0;\n\tint game_version_build = 0;',
    ),
    (
        'class Terrain: public QObject {',
        'class Terrain: public QObject {\n\tfriend struct HiveBridge;',
    ),
    (
        '#include "casc.hpp"',
        '#include "casc.hpp"\n#include "mpq.hpp"',
    ),
    (
        '\tcasc::CASC game_data;',
        '\tmpq::MPQ game_data;',
    ),
    (
        'VkPipeline create_graphics_pipeline(const PipelineDescription& description) {',
        'VkPipeline create_graphics_pipeline(const PipelineDescription& description) {\n'
        '#if HIVE_HEADLESS\n\treturn VK_NULL_HANDLE;\n#endif',
    ),
]


def apply_patches(text):
    for old, new in PATCHES:
        text = text.replace(old, new)
    text = RE_EMIT.sub(r'\1if (\2) { \2(\3); }', text)
    text = RE_SLOTS.sub(r'\1public:', text)
    text = RE_EXEC_PAR.sub(r'std::\1(', text)
    return RE_MOC.sub('', text)


def collect_modules(src):
    modules = {}
    for dirpath, _, files in os.walk(src):
        for name in files:
            if not name.endswith('.ixx'):
                continue
            path = os.path.join(dirpath, name)
            text = open(path, encoding='utf-8-sig', errors='ignore').read()
            found = RE_MODULE.search(text)
            if found:
                modules[found.group(1)] = path
    return modules


def build_include_map(modules):
    return {
        module: os.path.splitext(os.path.basename(path))[0] + '.hpp'
        for module, path in modules.items()
    }


def strip_export_block(lines):
    out = []
    i = 0
    while i < len(lines):
        if lines[i].strip() == 'export {':
            depth = 1
            i += 1
            while i < len(lines) and depth > 0:
                if lines[i].strip() == '}':
                    depth -= 1
                    if depth == 0:
                        i += 1
                        break
                out.append(lines[i])
                i += 1
            continue
        out.append(lines[i])
        i += 1
    return out


def rewrite(lines, includes, keep_pragma):
    out = []
    pragma_done = False
    for line in lines:
        stripped = line.strip()

        if stripped == 'module;':
            continue

        if RE_MODULE.match(stripped):
            if keep_pragma and not pragma_done:
                out.append('#pragma once')
                pragma_done = True
            continue

        impl = RE_MODULE_IMPL.match(stripped)
        if impl:
            module = impl.group(1)
            out.append(f'#include "{includes.get(module, module + ".hpp")}"')
            continue

        if RE_IMPORT_STD.match(stripped):
            out.append(STD_HEADERS)
            continue

        angle = RE_IMPORT_ANGLE.match(stripped)
        if angle:
            out.append(f'#include <{angle.group(1)}>')
            continue

        quoted = RE_IMPORT_QUOTED.match(stripped)
        if quoted:
            out.append(f'#include "{quoted.group(1)}"')
            continue

        named = RE_IMPORT_NAMED.match(stripped)
        if named:
            module = named.group(1)
            out.append(f'#include "{includes.get(module, module + ".hpp")}"')
            continue

        if stripped.startswith('export '):
            line = line.replace('export ', '', 1)
        out.append(line)

    return out


def emit(path, dst, src, includes, keep_pragma, suffix):
    text = open(path, encoding='utf-8-sig', errors='ignore').read()
    if keep_pragma:
        lines = rewrite(strip_export_block(text.split('\n')), includes, keep_pragma=True)
        if '#pragma once' in lines:
            lines.remove('#pragma once')
            lines.insert(0, '#pragma once')
            lines.insert(1, '')
    else:
        lines = rewrite(text.split('\n'), includes, keep_pragma=False)

    content = apply_patches('\n'.join(lines))
    rel = os.path.relpath(path, src)
    out = os.path.join(dst, os.path.splitext(rel)[0] + suffix)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'w', encoding='utf-8') as f:
        f.write(content)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()

    src = os.path.abspath(args.src)
    dst = os.path.abspath(args.dst)

    os.makedirs(dst, exist_ok=True)

    modules = collect_modules(src)
    includes = build_include_map(modules)

    for path in modules.values():
        emit(path, dst, src, includes, keep_pragma=True, suffix='.hpp')

    cpp = 0
    header = 0
    for dirpath, _, files in os.walk(src):
        for name in files:
            path = os.path.join(dirpath, name)
            if name.endswith('.cpp'):
                emit(path, dst, src, includes, keep_pragma=False, suffix='.cpp')
                cpp += 1
            elif name.endswith(('.h', '.hpp', '.inl')):
                emit(path, dst, src, includes, keep_pragma=False, suffix=os.path.splitext(name)[1])
                header += 1

    print(f'{len(modules)} 个模块 → {dst}')
    print(f'{cpp} 个 cpp、{header} 个头文件同步改写')


if __name__ == '__main__':
    main()
