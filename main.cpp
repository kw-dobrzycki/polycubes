#include <iostream>
#include "Tet.h"
#include "poly.h"
#include "critical.h"
#include <omp.h>
#include <chrono>

//#define DATA

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
std::vector<size_t> findNonCritical(const Tet<n>& tet) {
	std::vector<size_t> noncrit;
	noncrit.reserve(n);

	for (int i = 0; i < n; ++i) {
		auto removed = tet.remove(i);
		if (connected(removed)) noncrit.push_back(i);
	}

	return noncrit;
}

template<>
std::vector<size_t> findNonCritical(const Tet<1>&) {
	return {0, 1};
}

template<unsigned n>
std::vector<size_t> findNonCriticalFast(const Tet<n>& tet) {
	return AP(tet);
}

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

		for (auto& space: parent.getFreeSpaces()) {

			Tet<n> child = parent.insert(space);
			std::vector<size_t> noncritical = findNonCriticalFast(child);

			#ifdef DATA
			#pragma omp atomic
			stems += noncritical.size();
			#endif

			Tet<n - 1> max_stem = parent;

			for (auto& noncrit: noncritical) {
				Tet<n - 1> stem = child.remove(noncrit);

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

	#ifdef DATA
	std::cout << "N=" << n << ": " << global.size() << " time "
			  << std::chrono::duration_cast<std::chrono::milliseconds>(
					  std::chrono::high_resolution_clock::now() - last).count() << "ms"
			  << std::endl;

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
	auto results = generate<8>();
	return 0;
}
