//
// Created by croissant on 24/07/2023.
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <unordered_map>
#include "relative.h"
#include "groups.h"
#include "Tet.h"
#include <omp.h>
#include <list>
#include <random>


struct compare {
	std::array<Pos::type, 6> bounds;

	bool operator()(Pos a, Pos b) {
		Pos::boundType X = bounds[1] - bounds[0] + 1;
		Pos::boundType Z = bounds[5] - bounds[4] + 1;

		//translate and linearise //todo perhaps this affects collisions - mixing encodings
		unsigned la = a.y * X * Z + a.z * X + a.x;
		unsigned lb = b.y * X * Z + b.z * X + b.x;
		return la < lb;
	}
};

//0 = equal; 1 = a < b; 2 = a > b
int compareBounds(const std::array<Pos::type, 6>& a, const std::array<Pos::type, 6>& b) {
	auto AX = a[1] - a[0];
	auto AY = a[3] - a[2];
	auto AZ = a[5] - a[4];

	auto BX = b[1] - b[0];
	auto BY = b[3] - b[2];
	auto BZ = b[5] - b[4];

	if (AY > BY)
		return 1;
	else if (AY < BY)
		return 2;
	else if (AZ > BZ)
		return 1;
	else if (AZ < BZ)
		return 2;
	else if (AX > BX)
		return 1;
	else if (AX < BX)
		return 2;

	return 0;
}

unsigned maxComparisons = 0;

// assumes a.n == b.n
// returns a < b
bool biggerBoundEncoding(const Tet& a, const Tet& b) {
	//rebase a, b; sort their coordinates by their encoding index, return the bigger
	auto boundsA = a.getBounds();
	auto boundsB = b.getBounds();
	auto seedA = Pos{boundsA[0], boundsA[2], boundsA[4]};
	auto seedB = Pos{boundsB[0], boundsB[2], boundsB[4]};

	int boundsUnequal = compareBounds(boundsA, boundsB);

	if (boundsUnequal)
		return boundsUnequal == 1;

	maxComparisons++;

	std::vector<Pos> A(a.n);
	std::vector<Pos> B(b.n);
	for (int i = 0; i < a.n; ++i) {
		A[i] = a.coords[i] - seedA;
		B[i] = b.coords[i] - seedB;
	}

	std::sort(A.begin(), A.end(), compare{boundsA});
	std::sort(B.begin(), B.end(), compare{boundsB});

	//compare
	for (int i = 0; i < a.n; ++i) {
		if (A[i].y < B[i].y)
			return false;
		else if (A[i].y > B[i].y)
			return true;

		if (A[i].z < B[i].z)
			return false;
		else if (A[i].z > B[i].z)
			return true;

		if (A[i].x < B[i].x)
			return false;
		else if (A[i].x > B[i].x)
			return true;
	}

	//return either
	return true;
}

Tet getMaxRotation(Tet t) {
	Tet max = t;

	//spin on top
	for (int j = 0; j < 4; ++j) {
		t.rotY();
		if (biggerBoundEncoding(max, t))
			max = t;
	}

	//spin on back
	t.rotX();
	for (int j = 0; j < 4; ++j) {
		t.rotZ();
		if (biggerBoundEncoding(max, t))
			max = t;
	}

	//spin on right
	t.rotY();
	for (int j = 0; j < 4; ++j) {
		t.rotX();
		if (biggerBoundEncoding(max, t))
			max = t;
	}

	//spin on front
	t.rotY();
	for (int j = 0; j < 4; ++j) {
		t.rotZ();
		if (biggerBoundEncoding(max, t))
			max = t;
	}

	//spin on left
	t.rotY();
	for (int j = 0; j < 4; ++j) {
		t.rotX();
		if (biggerBoundEncoding(max, t))
			max = t;
	}

	//spin on bottom
	t.rotY();
	t.rotX();
	for (int j = 0; j < 4; ++j) {
		t.rotY();
		if (biggerBoundEncoding(max, t))
			max = t;
	}

	return max;
}

std::vector<Tet> generate(unsigned int i) {
	if (i <= 1) {
		return std::vector{Tet{1, {{0, 0, 0}}}};
	}

	auto previous = generate(i - 1);

	constexpr int depth = 1;

	long long int counter = 0;
	maxComparisons = 0;

	BoundCache<depth> globalResult(i);

	//pad previous to create empty work for distribution
	unsigned fill = numThreads - previous.size() % numThreads;
	for (int j = 0; j < fill % numThreads; ++j) {
		previous.push_back(Tet(0, {}));
	}

	unsigned chunksize = previous.size() / numThreads;

	std::vector<LocalBoundCache> local(numThreads, i);

	std::cout << "shuffling" << std::endl;

//	std::random_device rd;
//	std::mt19937 g(rd());
//	std::shuffle(previous.begin(), previous.end(), g);

	std::cout << "shuffled" << std::endl;
	#pragma omp parallel default(none) shared(chunksize, previous, globalResult, local, numThreads)
	for (unsigned j = omp_get_thread_num() * chunksize; j < (omp_get_thread_num() + 1) * chunksize; ++j) {

		const auto& p = previous[j];
		LocalBoundCache& localResult = local[omp_get_thread_num()];
		localResult.clear();

		for (const auto& f: p.getFreeSpaces()) {
			Tet build = p.insert(f);
			Tet max = getMaxRotation(build);
			max.volumeEncoding = max.volumeEncode();

			auto bounds = max.getBounds();
			auto encoding = encodeBound({static_cast<Pos::type>(bounds[1] - bounds[0] + 1),
										 static_cast<Pos::type>(bounds[3] - bounds[2] + 1),
										 static_cast<Pos::type>(bounds[5] - bounds[4] + 1)
										});

			//skip padded (artificial) work
			//todo might be quicker to check iteration bound instead of fake work
			if (!encoding) continue;

			const auto& c = globalResult.at(encoding);

			//if not found in the previous iteration
			if (!c.contains(max.volumeEncoding)) {
				localResult.at(encoding).push_back(max);
			}
		}

		//write to global result - check for dups between local results
		#pragma omp barrier
		#pragma omp single
		{
			//caches with different bounds can write simultaneously
			#pragma parallel for
			for (int k = 0; k < globalResult.keys.size(); ++k) {
				auto key = globalResult.keys[k];
				auto& bucket = globalResult.at(key);

				//get result for that bound from each local result
				for (int t = 0; t < numThreads; ++t){
					for (const auto& res: local[t].at(key)) {
						if (!bucket.contains(res.volumeEncoding)){
							bucket.insert(res.volumeEncoding);
						}
					}
				}
			}
		}
		//implied barrier
	}

	std::vector<Tet> unique = globalResult.collect();

	auto n = unique.size();
	bool right;
	switch (i) {
		case 2:
			right = n == 1;
			break;
		case 3:
			right = n == 2;
			break;
		case 4:
			right = n == 8;
			break;
		case 5:
			right = n == 29;
			break;
		case 6:
			right = n == 166;
			break;
		case 7:
			right = n == 1023;
			break;
		case 8:
			right = n == 6922;
			break;
		case 9:
			right = n == 48311;
			break;
		case 10:
			right = n == 346543;
			break;
		case 11:
			right = n == 2522522;
			break;
		case 12:
			right = n == 18598427;
			break;
		case 13:
			right = n == 138462649;
			break;
		case 14:
			right = n == 1039496297;
			break;
		case 15:
			right = n == 7859514470;
			break;
		case 16:
			right = n == 59795121480;
			break;
	}

	std::cout << "Total collisions: " << collisions << std::endl;
	std::cout << "Total comparisons: " << comparisons << std::endl;
	std::cout << "Total max-comparisons: " << maxComparisons << std::endl;
	collisions = 0;

	std::string msg = "INCORRECT";
	if (right)
		msg = "CORRECT";

	std::cout << "n = " << i << ": " << unique.size() << " unique shapes\n";
	std::cout << msg << "\n";

	NestedHash<depth>::print();

	std::cout << "Total tables: " << tables << " total items " << items << std::endl;
	std::cout << "Average load: " << (float) items / tables << std::endl;
	NestedHash<depth>::reset();

	std::cout << std::endl;
	return unique;

}

void write(const std::vector<Tet>& v) {
	std::cout << "saving " << v.size() << " results..." << std::endl;
	std::ofstream file;
	file.open("../blocks.txt");
	for (auto& t: v) {
		for (auto& k: t.coords) {
			file << k.x << " " << k.y << " " << k.z << std::endl;
		}
		file << std::endl;
	}
	file.close();
}

std::vector<std::string> split(const std::string& s, char delimiter) {
	std::stringstream ss(s);
	std::vector<std::string> elems;
	std::string item;
	while (std::getline(ss, item, delimiter)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<Tet> read(const std::string_view path) {
//	std::ifstream file(std::string{path});
//	std::string line;
//
	std::vector<Tet> tets;
//	std::vector<Pos> blocks;
//	std::cout << "reading" << std::endl;
//	while (std::getline(file, line, '\n')) {
//		if (line.empty()) {
//			tets.emplace_back(blocks.size(), blocks);
//			blocks.clear();
//			continue;
//		}
//		auto spaced = split(line, ' ');
//		blocks.push_back({static_cast<Pos::type>(std::stoi(spaced[0])),
//						  static_cast<Pos::type>(std::stoi(spaced[1])),
//						  static_cast<Pos::type>(std::stoi(spaced[2]))
//						 });
//	}
//
//	std::cout << "read " << tets.size() << " shapes";
	return tets;
}
