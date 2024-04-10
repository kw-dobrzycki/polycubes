#include <iostream>
#include "Tet.h"
#include "poly.h"
#include "critical.h"
#include <omp.h>
#include <chrono>

//#define DATA

static auto last = std::chrono::high_resolution_clock::now();

template<unsigned n>
std::vector<Tet<n>> generate() {
	std::vector<Tet<n - 1>> previous = generate<n - 1>();

	std::vector<Tet<n>> global;

	size_t stems = 0;
	size_t maxStemRejections = 0;
	size_t localRejections = 0;

	#pragma omp parallel for default(none) shared(previous, global, stems, maxStemRejections, localRejections)
	for (int j = 0; j < previous.size(); ++j) {
		auto& parent = previous[j];

		std::vector<Tet<n>> local;

		#ifdef DATA
		size_t max_stem_rejections = 0;
		size_t local_rejections = 0;
		#endif

		bool parent_critical[n - 1]{};
		findCriticalRecursive<n - 2>(parent.remove(n - 2), parent.units[n - 2], parent_critical);

		for (auto& space: parent.getFreeSpaces()) {

			Tet<n> child = parent.insert(space);

			bool critical[n]{};
			memcpy(critical, parent_critical, (n - 1) * sizeof(bool));
			findCriticalRecursive<n - 1>(parent, space, critical, false);

			#ifdef DATA
			#pragma omp atomic
			stems += noncritical.size();
			#endif

			Tet<n - 1> max_stem = parent;

			for (int i = 0; i < n; ++i) {
				if (critical[i]) continue;

				Tet<n - 1> stem = child.remove(i);

				orient<n - 1>(stem);

				if (compareFixed<n - 1>(stem, max_stem) == 1)
					max_stem = stem;

				#ifdef DATA
				else max_stem_rejections++;
				#endif
			}

			if (compareFixed<n - 1>(parent, max_stem) == 0) {
				orient<n>(child);
				bool seen = false;
				for (int i = 0; i < local.size(); ++i) {
					if (compareFixed<n>(local[i], child) == 0) {
						seen = true;

						#ifdef DATA
						local_rejections++;
						#endif

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

		#ifdef DATA
		#pragma omp atomic
		maxStemRejections += max_stem_rejections;

		#pragma omp atomic
		localRejections += local_rejections;
		#endif
	}

	std::cout << "N=" << n << ": " << global.size() << " time "
			  << std::chrono::duration_cast<std::chrono::milliseconds>(
					  std::chrono::high_resolution_clock::now() - last).count() << "ms"
			  << std::endl;

	#ifdef DATA
	std::cout << "Non-max stems / connected stems: " <<
			  maxStemRejections << "/" << stems << " (" <<
			  (float) maxStemRejections / (float) stems <<
			  ")" << std::endl;

	std::cout << "Locally non-unique rejections: " << localRejections << std::endl;
	auto total = maxStemRejections + localRejections;

	std::cout << "Total rejection rate: " <<
			  total << "/" << stems << " (" <<
			  (float) total / (float) stems <<
			  ")" << std::endl << std::endl;
	#endif
	last = std::chrono::high_resolution_clock::now();
	return global;
}

template<>
std::vector<Tet<2>> generate() {
	Pos unit[2]{{0, 0, 0},
				{0, 1, 0}};
	auto tet = Tet<2>{unit};
	orient(tet);
	last = std::chrono::high_resolution_clock::now();
	return {tet};
}

template<>
std::vector<Tet<1>> generate() {
	Pos unit{0, 0, 0};
	last = std::chrono::high_resolution_clock::now();
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
