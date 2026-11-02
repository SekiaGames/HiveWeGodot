#!/usr/bin/env python3

import argparse
import os
import re
import shutil

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
RE_IMPORT_STD = re.compile(r'^import std;\s*$')
RE_IMPORT_ANGLE = re.compile(r'^import\s+<([^>]+)>;\s*$')
RE_IMPORT_QUOTED = re.compile(r'^import\s+"([^"]+)"\s*;\s*$')
RE_IMPORT_NAMED = re.compile(r'^import\s+([\w.]+)\s*;\s*$')


def collect_modules(src):
    modules = {}
    for dirpath, _, files in os.walk(src):
        for name in files:
            if not name.endswith('.ixx'):
                continue
            path = os.path.join(dirpath, name)
            text = open(path, encoding='utf-8', errors='ignore').read()
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


def convert_ixx(path, dst, src, includes):
    text = open(path, encoding='utf-8', errors='ignore').read()
    lines = rewrite(strip_export_block(text.split('\n')), includes, keep_pragma=True)
    if '#pragma once' in lines:
        lines.remove('#pragma once')
        lines.insert(0, '#pragma once')
        lines.insert(1, '')

    rel = os.path.relpath(path, src)
    out = os.path.join(dst, os.path.splitext(rel)[0] + '.hpp')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))


def convert_cpp(path, dst, src, includes):
    text = open(path, encoding='utf-8', errors='ignore').read()
    lines = rewrite(text.split('\n'), includes, keep_pragma=False)
    rel = os.path.relpath(path, src)
    out = os.path.join(dst, rel)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--src', required=True, help='HiveWE 的 src 目录')
    parser.add_argument('--dst', required=True, help='输出目录')
    args = parser.parse_args()

    src = os.path.abspath(args.src)
    dst = os.path.abspath(args.dst)

    if os.path.isdir(dst):
        shutil.rmtree(dst)
    os.makedirs(dst)

    modules = collect_modules(src)
    includes = build_include_map(modules)

    for path in modules.values():
        convert_ixx(path, dst, src, includes)

    cpp = 0
    for dirpath, _, files in os.walk(src):
        for name in files:
            if name.endswith('.cpp'):
                convert_cpp(os.path.join(dirpath, name), dst, src, includes)
                cpp += 1

    print(f'{len(modules)} 个模块 → {dst}')
    print(f'{cpp} 个 cpp 同步改写')


if __name__ == '__main__':
    main()
