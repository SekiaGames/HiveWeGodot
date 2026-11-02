cmake_minimum_required(VERSION 3.29)
project(hivewe_core CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

set(VULKAN_SDK "/Volumes/Data/Users/sekia/VulkanSDK/1.4.328.1/macOS" CACHE PATH "Vulkan SDK 根目录")

file(GLOB_RECURSE CORE_SOURCES
	"${ROOT}/native/src/base/*.cpp"
	"${ROOT}/native/src/brush/*.cpp"
	"${ROOT}/native/src/file_formats/*.cpp"
	"${ROOT}/native/src/utilities/*.cpp"
	"${ROOT}/native/src/resources/*.cpp"
)

file(GLOB_RECURSE CORE_INCLUDE_DIRS LIST_DIRECTORIES true "${ROOT}/native/src/*")

add_library(hivewe_core STATIC ${CORE_SOURCES})

target_include_directories(hivewe_core PRIVATE
	"${ROOT}/native/include"
	"${ROOT}/native/src"
	${CORE_INCLUDE_DIRS}
	"${ROOT}/native/src/base/triggers"
	"${ROOT}/native/src/base/map"
	"${ROOT}/native/src/file_formats/mdx"
	"${ROOT}/third_party"
	"${ROOT}/third_party/vma"
	"${ROOT}/third_party/vkbootstrap"
	"/opt/homebrew/opt/vulkan-volk/include"
	"${VULKAN_SDK}/include"
	"/opt/homebrew/opt/glm/include"
	"/opt/homebrew/opt/tbb/include"
	"/opt/homebrew/opt/abseil/include"
	"/opt/homebrew/opt/nlohmann-json/include"
	"/opt/homebrew/opt/jpeg-turbo/include"
	"/opt/homebrew/opt/bullet/include"
	"/opt/homebrew/opt/bullet/include/bullet"
)

target_compile_options(hivewe_core PRIVATE
	-include "${ROOT}/native/include/std_shim.hpp"
	-Wno-everything
)
