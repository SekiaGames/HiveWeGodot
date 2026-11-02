#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

if [ ! -f native/build/libhivewe.dylib ]; then
	echo "先编库：cmake --build native/build --target hivewe -j8" >&2
	exit 1
fi

mkdir -p /tmp/hivewe_tests

for name in hive_abi_test brush_test save_test pack_test model_test doodad_edit_test pick_test clipboard_test unit_edit_test select_test object_field_test object_custom_test map_info_test map_resize_test doodad_random_test triggers_test trigger_eca_test team_color_test pose_test region_edit_test; do
	if ! clang -std=c17 -I native/bridge "native/test/${name}.c" -L native/build -lhivewe -o "/tmp/hivewe_tests/${name}"; then
		echo "编译失败：${name}" >&2
		exit 1
	fi
	echo "=== ${name}"
	if ! output=$(DYLD_LIBRARY_PATH=native/build "/tmp/hivewe_tests/${name}" 2>&1); then
		echo "${output}" | grep -vE "loading:|Warning:|^\s*$"
		echo "失败：${name}" >&2
		exit 1
	fi
	echo "${output}" | grep -vE "loading:|Warning:|^\s*$"
done
