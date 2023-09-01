#include "relative.h"

int main() {
	constexpr auto a = sizeof(Pos);
	constexpr uint8_t n = 11;
	static_assert(n <= 1 << (Pos::width - 1));
	generate(n);
	return 0;
}
