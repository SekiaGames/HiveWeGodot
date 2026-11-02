#!/bin/bash
set -e

UPSTREAM=https://github.com/stijnherfst/HiveWE.git
COMMIT=cd6d61a

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="${1:-$ROOT/native/_upstream}"

if [ -d "$DEST/.git" ]; then
	echo "已存在：$DEST"
	git -C "$DEST" fetch --depth 1 origin "$COMMIT"
	git -C "$DEST" checkout "$COMMIT"
else
	git clone "$UPSTREAM" "$DEST"
	git -C "$DEST" checkout "$COMMIT"
fi

echo "$DEST 就绪，commit $COMMIT"
echo "下一步：python3 $ROOT/tools/decomod.py --src $DEST/src --dst $ROOT/native/src"
