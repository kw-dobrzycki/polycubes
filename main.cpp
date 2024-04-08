#include <iostream>
#include "Tet.h"
#include "poly.h"
#include <omp.h>
#include <chrono>

static auto last = std::chrono::high_resolution_clock::now();

template<unsigned n>
bool connected(const Tet<n>& tet) {

	thread_local size_t stack[n]{};
	thread_local size_t pointer;
	pointer = 1;

	thread_local bool seen[n]{};
	for (int i = 0; i < n; ++i) {
		seen[i] = 0;
	}

	while (pointer > 0) {
		size_t i = stack[--pointer];
		const Pos& ui = tet.units[i];
		if (seen[i]) continue;
		seen[i] = true;

		for (int j = 0; j < n; ++j) {
			const Pos& uj = tet.units[j];
			if ((uj - ui).isUnit() && !seen[j]) {
				stack[pointer++] = j;
			}
		}
	}

	for (int i = 0; i < n; ++i) {
		if (!seen[i]) return false;
	}

	return true;
}

template<unsigned n>
std::vector<Tet<n>> generate() {
	std::vector<Tet<n - 1>> previous = generate<n - 1>();

	std::vector<Tet<n>> global;

	#pragma omp parallel for default(none) shared(previous, global)
	for (int j = 0; j < previous.size(); ++j) {
		auto& parent = previous[j];

		std::vector<Tet<n>> local;

		for (auto& space: parent.getFreeSpaces()) {

			Tet<n> child = parent.insert(space);
			orient<n>(child);

			Tet<n - 1> max_stem;

			for (unsigned i = 0; i < n; ++i) {

				Tet<n - 1> stem = child.remove(i);
				if (!connected<n - 1>(stem)) continue;

				orient<n - 1>(stem);

				if (compareTet<n - 1>(stem, max_stem) == 1) {
					max_stem = stem;
				}
			}

			if (compareTet<n - 1>(parent, max_stem) == 0) {
				bool seen = false;
				for (int i = 0; i < local.size(); ++i) {
					if (compareTet<n>(local[i], child) == 0) {
						seen = true;
						break;
					}
				}
				if (!seen) local.push_back(child);
			}
		}

		#pragma omp critical
		{
			global.insert(global.end(), local.begin(), local.end());
		}
	}
	std::cout << "N=" << n << ": " << global.size() << " time "
			  << std::chrono::duration_cast<std::chrono::milliseconds>(
					  std::chrono::high_resolution_clock::now() - last).count() << "ms"
			  << std::endl;
	last = std::chrono::high_resolution_clock::now();
	return global;
}

template<>
std::vector<Tet<1>> generate() {
	Pos unit{0, 0, 0};
	return {Tet<1>{&unit}};
}

template<>
std::vector<Tet<0>> generate() {
	return {};
};

int main() {
	std::cout << "Threads: " << omp_get_max_threads() << std::endl;
	auto results = generate<10>();
	std::cout << results.size() << std::endl;
	return 0;
}
