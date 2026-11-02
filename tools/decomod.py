#!/usr/bin/env python3
"""把 HiveWE 的 C++20 模块（.ixx）转成普通头文件（.hpp）。

Apple clang 不支持 `export module`，去模块化之后才能编。
保留原目录结构，因为 `import "brush/brush.h";` 这类引号 import 依赖层级。

    python3 tools/decomod.py --src native/_upstream/src --dst native/src
"""

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
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <execution>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <print>
#include <queue>
#include <random>
#include <ranges>
#include <set>
#include <span>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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


def rewrite(lines, keep_pragma):
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
            out.append(f'#include "{named.group(1)}.hpp"')
            continue

        if stripped.startswith('export '):
            line = line.replace('export ', '', 1)
        out.append(line)

    return out


def convert_ixx(path, dst, src):
    text = open(path, encoding='utf-8', errors='ignore').read()
    lines = rewrite(strip_export_block(text.split('\n')), keep_pragma=True)
    if '#pragma once' in lines:
        lines.remove('#pragma once')
        lines.insert(0, '#pragma once')
        lines.insert(1, '')

    rel = os.path.relpath(path, src)
    out = os.path.join(dst, os.path.splitext(rel)[0] + '.hpp')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))


def convert_cpp(path, dst, src):
    text = open(path, encoding='utf-8', errors='ignore').read()
    lines = rewrite(text.split('\n'), keep_pragma=False)
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

    ixx = 0
    for path in modules.values():
        convert_ixx(path, dst, src)
        ixx += 1

    cpp = 0
    for dirpath, _, files in os.walk(src):
        for name in files:
            if name.endswith('.cpp'):
                convert_cpp(os.path.join(dirpath, name), dst, src)
                cpp += 1

    print(f'{ixx} 个模块 → {dst}')
    print(f'{cpp} 个 cpp 同步改写')


if __name__ == '__main__':
    main()
