#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:/usr/local/bin:$PATH"

APP=${1:-godot/HiveWeGodot.app}
SOURCE=${2:-godot/lib/libhivewe.dylib}
PAUSE=${3:-}

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

[ -d "$APP/Contents" ] || fail "找不到 $APP/Contents"
if [ ! -f "$SOURCE" ]; then
	echo "没有 ${SOURCE}，先编一份"
	bash tools/build_godot_lib.sh || fail "编译原生库失败"
fi

DEST="$APP/Contents/Resources/lib"
rm -rf "$DEST"
mkdir -p "$DEST"
cp "$SOURCE" "$DEST/"

homebrew_deps() {
	otool -L "$1" 2>/dev/null | tail -n +2 | awk '{print $1}' | grep '^/opt/homebrew/' || true
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
		[ -f "$dep" ] || fail "缺依赖 $dep"
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
	codesign --force --sign - "$file" > /dev/null 2>&1 || true
done

echo "放好 $(ls "$DEST" | wc -l | tr -d ' ') 个库：$DEST"
echo "app 本身不要重新签名，签过之后 macOS 会直接拒绝启动"
finish
