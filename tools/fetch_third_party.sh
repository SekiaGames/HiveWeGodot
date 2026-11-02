#!/bin/bash
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$ROOT/third_party"

if [ -n "$1" ]; then
	export http_proxy="$1"
	export https_proxy="$1"
fi

mkdir -p "$DEST/ankerl" "$DEST/vma" "$DEST/vkbootstrap"

fetch() {
	local out="$1"
	local url="$2"
	echo "下载 $out"
	curl -fsSL --max-time 60 -o "$DEST/$out" "$url"
	echo "  $(wc -c < "$DEST/$out") 字节"
}

fetch ankerl/unordered_dense.h https://raw.githubusercontent.com/martinus/unordered_dense/main/include/ankerl/unordered_dense.h
fetch ankerl/stl.h https://raw.githubusercontent.com/martinus/unordered_dense/main/include/ankerl/stl.h
fetch vma/vk_mem_alloc.h https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/master/include/vk_mem_alloc.h
fetch vkbootstrap/VkBootstrap.h https://raw.githubusercontent.com/charles-lunarg/vk-bootstrap/main/src/VkBootstrap.h
fetch vkbootstrap/VkBootstrap.cpp https://raw.githubusercontent.com/charles-lunarg/vk-bootstrap/main/src/VkBootstrap.cpp
fetch vkbootstrap/VkBootstrapDispatch.h https://raw.githubusercontent.com/charles-lunarg/vk-bootstrap/main/src/VkBootstrapDispatch.h

echo "完成。volk 走 brew：brew install vulkan-volk"
