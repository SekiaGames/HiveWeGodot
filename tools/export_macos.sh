#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

GODOT=${GODOT:-/Applications/Godot_mono.app/Contents/MacOS/Godot}
OUT=${1:-godot/HiveWeGodot.app}

if [ ! -x "$GODOT" ]; then
	echo "找不到 Godot：$GODOT" >&2
	exit 1
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

echo "导出 $OUT"
"$GODOT" --headless --path godot --export-release "macOS" "$work/app.zip" > "$work/export.log" 2>&1 || {
	tail -20 "$work/export.log" >&2
	exit 1
}

rm -rf "$OUT"
unzip -q "$work/app.zip" -d "$work/unpacked"
mv "$work/unpacked/HiveWeGodot.app" "$OUT"

bash tools/bundle_macos_app.sh "$OUT"

echo "好了：$OUT"
