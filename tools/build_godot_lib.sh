#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

cmake --build native/build --target hivewe -j8
mkdir -p godot/lib
cp native/build/libhivewe.dylib godot/lib/
echo "已更新 godot/lib/libhivewe.dylib"
