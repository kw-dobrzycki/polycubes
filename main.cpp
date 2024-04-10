#include <iostream>
#include "Tet.h"
#include "poly.h"
#include <omp.h>
#include <chrono>

static auto last = std::chrono::high_resolution_clock::now();

template<unsigned n>
bool connected(const Tet<n>& tet) {

	size_t q[n]{};
	size_t p1 = 0;
	size_t p2 = 1;

	bool seen[n]{};
	seen[0] = 1;

	while (p1 < p2) {
		size_t i = q[p1++];
		const Pos& ui = tet.units[i];

		for (int j = 0; j < n; ++j) {
			const Pos& uj = tet.units[j];
			if ((uj - ui).isUnit()) {
				if (seen[j]) continue;
				seen[j] = true;
				q[p2++] = j;
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

	size_t stems = 0;
	size_t connectionRejections = 0;
	size_t maxStemRejections = 0;
	size_t localRejections = 0;

	#pragma omp parallel for default(none) shared(previous, global, stems, maxStemRejections, connectionRejections, localRejections)
	for (int j = 0; j < previous.size(); ++j) {
		auto& parent = previous[j];

		std::vector<Tet<n>> local;

		size_t max_stem_rejections = 0;
		size_t connection_rejections = 0;
		size_t local_rejections = 0;

		#pragma omp atomic
		stems += parent.getFreeSpaces().size() * n;

		for (auto& space: parent.getFreeSpaces()) {

			Tet<n> child = parent.insert(space);

			Tet<n - 1> max_stem;

			for (unsigned i = 0; i < n; ++i) {

				Tet<n - 1> stem = child.remove(i);
				if (!connected<n - 1>(stem)) {
					connection_rejections++;
					continue;
				}

				orient<n - 1>(stem);

				if (compareFixed<n - 1>(stem, max_stem) == 1) {
					max_stem = stem;
				} else {
					max_stem_rejections++;
				}
			}

			if (compareFixed<n - 1>(parent, max_stem) == 0) {
				orient<n>(child);
				bool seen = false;
				for (int i = 0; i < local.size(); ++i) {
					if (compareFixed<n>(local[i], child) == 0) {
						seen = true;
						local_rejections++;
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

		#pragma omp atomic
		maxStemRejections += max_stem_rejections;

		#pragma omp atomic
		connectionRejections += connection_rejections;

		#pragma omp atomic
		localRejections += local_rejections;
	}
	std::cout << "N=" << n << ": " << global.size() << " time "
			  << std::chrono::duration_cast<std::chrono::milliseconds>(
					  std::chrono::high_resolution_clock::now() - last).count() << "ms"
			  << std::endl;

	std::cout << "Disconnected stems / total stems: " <<
			  connectionRejections << "/" << stems << " (" <<
			  (float) connectionRejections / (float) stems <<
			  ")" << std::endl;

	std::cout << "Non-max stems / connected stems: " <<
			  maxStemRejections << "/" << stems - connectionRejections << " (" <<
			  (float) maxStemRejections / (float) (stems - connectionRejections) <<
			  ")" << std::endl;

	std::cout << "Locally non-unique rejections: " << localRejections << std::endl;
	auto total = maxStemRejections + connectionRejections + localRejections;

	std::cout << "Total ejection rate: " <<
			  total << "/" << total + global.size() << " (" <<
			  (float) total / (total + global.size()) <<
			  ")" << std::endl << std::endl;

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
	return 0;
}
