#include <iostream>
#include "Tet.h"
#include "poly.h"
#include "critical.h"
#include <omp.h>
#include <chrono>

//#define DATA

static auto last = std::chrono::high_resolution_clock::now();

template<unsigned n>
bool linearSearch(std::vector<Tet<n>>& local, const Tet<n>& tet) {
	bool seen = false;
	for (int i = 0; i < local.size(); ++i) {
		if (EncodeType<n>(local[i]) == EncodeType<n>(tet)) {
			seen = true;
			break;
		}
	}
	return seen;
}

template<unsigned n>
std::vector<Tet<n>> generate() {
	std::vector<Tet<n - 1>> previous = generate<n - 1>();

	std::vector<Tet<n>> global;

	#ifdef DATA
	size_t allChildren = 0;
	size_t maxStemRejections = 0;
	size_t localRejections = 0;
	size_t volumeRejections = 0;
	#endif

//	#pragma omp parallel for default(none) shared(previous, global, stems, maxStemRejections, localRejections)
	for (int j = 0; j < previous.size(); ++j) {
		auto& parent = previous[j];
		EncodeType<n - 1> parentEncoding(parent);

		std::vector<Tet<n>> local;

		bool parentCritical[n - 1]{};
		findCriticalRecursive<n - 2>(parent.remove(n - 2), parent.units[n - 2], parentCritical);

		auto spaces = parent.getFreeSpaces();
		#ifdef DATA
		allChildren += spaces.size();
		#endif

		for (auto& space: spaces) {

			Tet<n> child = parent.insert(space);
			orient(child);

			if (linearSearch(local, child)) {
				#ifdef DATA
				localRejections++;
				#endif
				continue;
			}

			bool critical[n]{};
			memcpy(critical, parentCritical, (n - 1) * sizeof(bool));
			findCriticalRecursive<n - 1>(parent, space, critical, false);

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
					#ifdef DATA
					volumeRejections++;
					#endif
					continue;
				}
				orient(largestStem);
				writer = EncodeType<n - 1>(largestStem) == parentEncoding;
			}

			if (writer) {
				local.push_back(child);
			} else {
				maxStemRejections++;
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


	#ifdef DATA
	std::cout << "All children candidates\t\t" << allChildren <<
			  "\nNon-max stem rejections\t\t" << maxStemRejections <<
			  "\nLocal duplicate rejections\t" << localRejections <<
			  "\nVolume heuristic rejections\t" << volumeRejections <<
			  std::endl;
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
