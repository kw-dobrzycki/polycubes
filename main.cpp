#include <iostream>
#include "Tet.h"
#include "poly.h"
#include <omp.h>
#include <chrono>

static auto last = std::chrono::high_resolution_clock::now();

bool connected(const Pos* tet, unsigned n) {
	std::vector<bool> seen(n);
	std::vector<size_t> q({0});

	while (!q.empty()) {
		size_t i = q.back();
		q.pop_back();
		Pos current = tet[i];
		if (seen[i]) continue;
		seen[i] = true;

		for (int j = 0; j < n; ++j) {
			Pos d = current - tet[j];
			bool touch = false;
			for (const auto& k: offsets) {
				if (d == k) {
					touch = true;
					break;
				}
			}
			if (touch && !seen[j])
				q.push_back(j);
		}
	}

	for (auto&& i: seen)
		if (!i) return false;

	return true;
}

template<unsigned n>
std::vector<Tet<n>> generate() {
	std::vector<Tet<n - 1>> previous = generate<n - 1>();

	std::vector<Tet<n>> global;

//	#pragma omp parallel for default(none) shared(previous, global)
	for (int j = 0; j < previous.size(); ++j) {
		const auto& parent = previous[j];

		std::vector<Tet<n>> local;

		for (auto& space: parent.getFreeSpaces()) {

			Tet<n> child = parent.insert(space);
			orient(child.units, n);

			Tet<n - 1> max_stem;

			for (unsigned i = 0; i < n; ++i) {

				Tet<n - 1> stem = child.remove(i);
				if (!connected(stem.units, n - 1)) continue;

				orient(stem.units, n - 1);

				if (compareTet(stem.units, max_stem.units, n - 1) == 1) {
					max_stem = stem;
				}
			}

			if (compareTet(parent.units, max_stem.units, n - 1) == 0) {
				bool seen = false;
				for (int i = 0; i < local.size(); ++i) {
					if (compareTet(local[i].units, child.units, n) == 0) {
						seen = true;
						break;
					}
				}
				if (!seen) local.push_back(child);
			}
		}

//		#pragma omp critical
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
	auto results = generate<9>();
	std::cout << results.size() << std::endl;
	return 0;
}
