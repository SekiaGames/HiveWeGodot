#include <cstdio>

#include "map.hpp"

int main() {
	std::printf("构造 Map\n");
	Map map;
	std::printf("构造完成，loaded=%d\n", map.loaded);
	return 0;
}
