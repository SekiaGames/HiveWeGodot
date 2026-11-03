#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:/usr/local/bin:$PATH"

OUT=${1:-godot/HiveWeGodot.app}
PAUSE=${2:-}
if [ $# -eq 0 ]; then
	PAUSE=1
fi

GODOT=${GODOT:-/Applications/Godot_mono.app/Contents/MacOS/Godot}

finish() {
	echo
	if [ -n "$PAUSE" ]; then
		read -n 1 -s -r -p "按任意键关闭"
		echo
	fi
}

fail() {
	echo
	echo "失败：$1"
	finish
	exit 1
}

[ -x "$GODOT" ] || fail "找不到 Godot：${GODOT}"

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

echo "导出到 $OUT"
if ! "$GODOT" --headless --path godot --export-release "macOS" "$work/app.zip" > "$work/export.log" 2>&1; then
	tail -20 "$work/export.log"
	fail "Godot 导出失败"
fi

rm -rf "$OUT"
unzip -q "$work/app.zip" -d "$work/unpacked"
mv "$work/unpacked/HiveWeGodot.app" "$OUT"

bash tools/bundle-macos-app.command "$OUT" godot/lib/libhivewe.dylib

case "$OUT" in
	/*) echo "好了：$OUT" ;;
	*) echo "好了：$(pwd)/$OUT" ;;
esac
open -R "$OUT" 2>/dev/null || true
finish
