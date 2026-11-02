#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

GAME_DIR=${1:?用法: tools/extract_testdata.sh <Warcraft III 目录>}

if [ ! -f "$GAME_DIR/WAR3.MPQ" ]; then
	echo "找不到 $GAME_DIR/WAR3.MPQ" >&2
	exit 1
fi

if [ ! -x native/build/hivewe_smoke ] || [ ! -x native/build/extract_assets ]; then
	echo "先编译：cmake --build native/build -j8" >&2
	exit 1
fi

LIST=$(mktemp)
PART=$(mktemp)
trap 'rm -f "$LIST" "$PART"' EXIT

shopt -s nullglob
MAPS=(testdata/maps/*.w3m testdata/maps/*.w3x)
shopt -u nullglob

if [ ${#MAPS[@]} -eq 0 ]; then
	echo "testdata/maps 下没有地图" >&2
	exit 1
fi

for map in "${MAPS[@]}"; do
	echo "扫描 $(basename "$map")"
	HIVE_ACCESS_LOG="$PART" ./native/build/hivewe_smoke "$map" "$GAME_DIR" > /dev/null 2>&1 || true
	cat "$PART" >> "$LIST"
done

sort -u "$LIST" -o "$LIST"
echo "共 $(wc -l < "$LIST" | tr -d ' ') 个文件"

./native/build/extract_assets "$GAME_DIR" "$LIST" testdata/warcraft
du -sh testdata/warcraft
