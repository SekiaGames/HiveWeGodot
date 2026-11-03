#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

APP=${1:?用法: tools/bundle_macos_app.sh <HiveWeGodot.app> [libhivewe.dylib]}
SOURCE=${2:-godot/lib/libhivewe.dylib}

if [ ! -d "$APP/Contents" ]; then
	echo "找不到 $APP/Contents" >&2
	exit 1
fi
if [ ! -f "$SOURCE" ]; then
	echo "找不到 $SOURCE" >&2
	exit 1
fi

DEST="$APP/Contents/Resources/lib"
rm -rf "$DEST"
mkdir -p "$DEST"
cp "$SOURCE" "$DEST/"

homebrew_deps() {
	otool -L "$1" | tail -n +2 | awk '{print $1}' | grep '^/opt/homebrew/' || true
}

copied=" "
level=$(homebrew_deps "$DEST/libhivewe.dylib")
while [ -n "$level" ]; do
	next=""
	for dep in $level; do
		name=$(basename "$dep")
		case "$copied" in
			*" $name "*) continue ;;
		esac
		if [ ! -f "$dep" ]; then
			echo "缺依赖 $dep" >&2
			exit 1
		fi
		cp "$dep" "$DEST/$name"
		copied="$copied$name "
		next="$next $(homebrew_deps "$DEST/$name" | tr '\n' ' ')"
	done
	level=$next
done

for file in "$DEST"/*.dylib; do
	name=$(basename "$file")
	install_name_tool -id "@loader_path/$name" "$file"
	for dep in $(homebrew_deps "$file"); do
		install_name_tool -change "$dep" "@loader_path/$(basename "$dep")" "$file"
	done
done

for file in "$DEST"/*.dylib; do
	codesign --force --sign - "$file" > /dev/null 2>&1 || true
done
# 不重签 app 本身，动过签名之后的 app 在 macOS 上会直接启动失败

echo "放进 $DEST 的库：$(ls "$DEST" | wc -l | tr -d ' ') 个"
