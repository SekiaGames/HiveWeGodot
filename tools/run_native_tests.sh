#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

if [ ! -f native/build/libhivewe.dylib ]; then
	echo "先编库：cmake --build native/build --target hivewe -j8" >&2
	exit 1
fi

mkdir -p /tmp/hivewe_tests

for name in hive_abi_test brush_test save_test pack_test model_test doodad_edit_test pick_test clipboard_test unit_edit_test select_test; do
	clang -std=c17 -I native/bridge "native/test/${name}.c" -L native/build -lhivewe -o "/tmp/hivewe_tests/${name}"
	echo "=== ${name}"
	DYLD_LIBRARY_PATH=native/build "/tmp/hivewe_tests/${name}" 2>/dev/null | grep -vE "loading:|Warning:|^\s*$"
done
