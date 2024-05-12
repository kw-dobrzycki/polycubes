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

	size_t rej = 0;
	size_t unrej = 0;
	size_t writers = 0;

//	#pragma omp parallel for default(none) shared(previous, global, stems, maxStemRejections, localRejections)
	for (int j = 0; j < previous.size(); ++j) {
		auto& parent = previous[j];
		EncodeType<n - 1> parentEncoding(parent);

		std::vector<Tet<n>> local;

		#ifdef DATA
		size_t max_stem_rejections = 0;
		size_t local_rejections = 0;
		#endif

		bool parentCritical[n - 1]{};
		findCriticalRecursive<n - 2>(parent.remove(n - 2), parent.units[n - 2], parentCritical);

		for (auto& space: parent.getFreeSpaces()) {

			Tet<n> child = parent.insert(space);
			orient(child);

			bool critical[n]{};
			memcpy(critical, parentCritical, (n - 1) * sizeof(bool));
			findCriticalRecursive<n - 1>(parent, space, critical, false);

			#ifdef DATA
			#pragma omp atomic
			stems += noncritical.size();
			#endif

			bool writer = true;

			auto encode = [](Pos p) -> int {
				int i = n;
				return p.y * i * i + p.z * i + p.x;
			};

			//find the largest extender cube
			unsigned largestIndex = n - 1;
			int largestEncoding = 0;
			for (int i = 0; i < n; ++i) {
				if (critical[i]) continue;
				auto encoding = encode(child.units[i]);
				if (encoding > largestEncoding) {
					largestIndex = i;
					largestEncoding = encoding;
				}
			}

			if (largestIndex < n - 1) {
				Tet<n - 1> largestStem = child.remove(largestIndex);
				if (!equalVolume(parent, largestStem)) {
					continue;
				}
				orient(largestStem);
				writer = EncodeType<n - 1>(largestStem) == parentEncoding;
			}

			if (writer) {
				bool seen = false;
				for (int i = 0; i < local.size(); ++i) {
					if (EncodeType<n>(local[i]) == EncodeType<n>(child)) {
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

//	std::cout <<"stems " <<stems << " rej " << rej << " unrej " << unrej << " writers " << writers << std::endl;

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
	auto results = generate<11>();
	return 0;
}
