cmake_minimum_required(VERSION 3.29)
project(hivewe_core CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

set(VULKAN_SDK "/Volumes/Data/Users/sekia/VulkanSDK/1.4.328.1/macOS" CACHE PATH "Vulkan SDK 根目录")

file(GLOB_RECURSE HIVE_SOURCES
	"${ROOT}/native/src/base/*.cpp"
	"${ROOT}/native/src/brush/*.cpp"
	"${ROOT}/native/src/file_formats/*.cpp"
	"${ROOT}/native/src/utilities/*.cpp"
	"${ROOT}/native/src/resources/*.cpp"
)

file(GLOB_RECURSE HIVE_INCLUDE_DIRS LIST_DIRECTORIES true "${ROOT}/native/src/*")

add_library(hivewe_core STATIC
	${HIVE_SOURCES}
	"${ROOT}/native/src/render/vk_implementation.cpp"
	"${ROOT}/third_party/vkbootstrap/VkBootstrap.cpp"
)

target_include_directories(hivewe_core PUBLIC
	"${ROOT}/native/include"
	"${ROOT}/native/bridge"
	"${ROOT}/native/src"
	${HIVE_INCLUDE_DIRS}
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
	"/opt/homebrew/opt/stormlib/include"
)

target_compile_options(hivewe_core PUBLIC
	-include "${ROOT}/native/include/std_shim.hpp"
	-DHIVE_HEADLESS=1
	-w
)

if(HIVE_ASAN)
	target_compile_options(hivewe_core PUBLIC -fsanitize=address -g -O1)
	target_link_options(hivewe_core PUBLIC -fsanitize=address)
endif()

add_executable(hivewe_smoke test/load_map.cpp)
target_link_libraries(hivewe_smoke PRIVATE
	hivewe_core
	"/opt/homebrew/opt/stormlib/lib/libstorm.a"
	"-lbz2"
	"-lz"
	"/opt/homebrew/opt/bullet/lib/libBulletDynamics.a"
	"/opt/homebrew/opt/bullet/lib/libBulletCollision.a"
	"/opt/homebrew/opt/bullet/lib/libBulletSoftBody.a"
	"/opt/homebrew/opt/bullet/lib/libLinearMath.a"
	"/opt/homebrew/opt/tbb/lib/libtbb.dylib"
	"/opt/homebrew/opt/jpeg-turbo/lib/libjpeg.dylib"
	"/opt/homebrew/opt/jpeg-turbo/lib/libturbojpeg.dylib"
	"${VULKAN_SDK}/lib/libMoltenVK.dylib"
)
find_package(absl CONFIG REQUIRED)
target_link_libraries(hivewe_smoke PRIVATE absl::strings absl::str_format absl::flat_hash_map absl::flat_hash_set)
target_link_options(hivewe_smoke PRIVATE "-Wl,-rpath,${VULKAN_SDK}/lib")

add_executable(probe_files test/probe_files.cpp)
target_link_libraries(probe_files PRIVATE hivewe_core
	"/opt/homebrew/opt/stormlib/lib/libstorm.a" "-lbz2" "-lz"
	"/opt/homebrew/opt/bullet/lib/libBulletDynamics.a"
	"/opt/homebrew/opt/bullet/lib/libBulletCollision.a"
	"/opt/homebrew/opt/bullet/lib/libBulletSoftBody.a"
	"/opt/homebrew/opt/bullet/lib/libLinearMath.a"
	"/opt/homebrew/opt/tbb/lib/libtbb.dylib"
	"/opt/homebrew/opt/jpeg-turbo/lib/libjpeg.dylib"
	"/opt/homebrew/opt/jpeg-turbo/lib/libturbojpeg.dylib"
	"${VULKAN_SDK}/lib/libMoltenVK.dylib"
	absl::strings absl::str_format absl::flat_hash_map absl::flat_hash_set
)
target_link_options(probe_files PRIVATE "-Wl,-rpath,${VULKAN_SDK}/lib")

add_executable(extract_assets test/extract_assets.cpp)
target_link_libraries(extract_assets PRIVATE hivewe_core
	"/opt/homebrew/opt/stormlib/lib/libstorm.a" "-lbz2" "-lz"
	"/opt/homebrew/opt/bullet/lib/libBulletDynamics.a"
	"/opt/homebrew/opt/bullet/lib/libBulletCollision.a"
	"/opt/homebrew/opt/bullet/lib/libBulletSoftBody.a"
	"/opt/homebrew/opt/bullet/lib/libLinearMath.a"
	"/opt/homebrew/opt/tbb/lib/libtbb.dylib"
	"/opt/homebrew/opt/jpeg-turbo/lib/libjpeg.dylib"
	"/opt/homebrew/opt/jpeg-turbo/lib/libturbojpeg.dylib"
	"${VULKAN_SDK}/lib/libMoltenVK.dylib"
	absl::strings absl::str_format absl::flat_hash_map absl::flat_hash_set
)
target_link_options(extract_assets PRIVATE "-Wl,-rpath,${VULKAN_SDK}/lib")
