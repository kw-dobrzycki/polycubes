#include "relative.h"
#include <omp.h>

int main() {
	constexpr auto a = sizeof(Pos);
	constexpr uint8_t n = 10;
	static_assert(n <= 1 << (Pos::width - 1));
	omp_set_num_threads(omp_get_max_threads());
	generate(n);
	return 0;
}
