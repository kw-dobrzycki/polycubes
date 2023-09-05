#include "relative.h"
#include <omp.h>

const int numThreads{omp_get_max_threads()};

int main() {
	constexpr auto a = sizeof(Pos);
	constexpr uint8_t n = 11;
	static_assert(n <= 1 << (Pos::width - 1));
	omp_set_num_threads(numThreads);
	generate(n);
	return 0;
}
