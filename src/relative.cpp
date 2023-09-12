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

const Pos offsets[]{
		{0,  1,  0},
		{0,  0,  -1},
		{1,  0,  0},
		{0,  0,  1},
		{-1, 0,  0},
		{0,  -1, 0},
};

struct compare {
	std::array<Pos::type, 6> bounds;

	bool operator()(Pos a, Pos b) {
		Pos::boundType X = bounds[1] - bounds[0] + 1;
		Pos::boundType Z = bounds[5] - bounds[4] + 1;

		//translate and linearise
		a = a - Pos{bounds[0], bounds[2], bounds[4]};
		b = b - Pos{bounds[0], bounds[2], bounds[4]};
		unsigned la = a.y * X * Z + a.z * X + a.x;
		unsigned lb = b.y * X * Z + b.z * X + b.x;
		return la < lb;
	}
};

// assumes a.n == b.n
bool biggerBoundEncoding(const Tet& a, const Tet& b) {
	//rebase a, b; sort their coordinates by their encoding index, return the bigger
	auto boundsA = a.getBounds();
	auto boundsB = b.getBounds();
	auto seedA = Pos{boundsA[0], boundsA[2], boundsA[4]};
	auto seedB = Pos{boundsB[0], boundsB[2], boundsB[4]};

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

static unsigned long long collisions = 0;
static unsigned long long comparisons = 0;
static unsigned long long tables = 0;
static unsigned long long items = 0;

template<unsigned depth>
struct NestedHash {
	std::unordered_map<uint64_t, NestedHash<depth - 1>> map;
	std::vector<std::vector<uint64_t>> store;

	inline static uint64_t loadhi = 0;
	inline static uint64_t count = 0;
	inline static uint64_t loadtot = 0;

	static void print() {
		std::cout << "Depth " << depth << " total load " << loadtot << " highest load " << loadhi << " count " << count
				  << std::endl;
		NestedHash<depth - 1>::print();
	}

	NestedHash() {
		tables++;
		count++;
	}

	static void reset() {
		loadhi = 0;
		count = 0;
		tables = 0;
		items = 0;
		collisions = 0;
		comparisons = 0;
		loadtot = 0;
		NestedHash<depth - 1>::reset();
	}

	bool contains(const Tet& t) const {
		auto x = t.volumeEncode();
		return lookup(x);
	}

	bool lookup(const std::vector<uint64_t>& encoding, unsigned bit = 0) const {
		if (bit == encoding.size()) {
			for (int i = 0; i < store.size(); ++i) {
				comparisons++;
				bool equal = true;
				for (int j = 0; j < encoding.size(); ++j) {
					if (store[i][j] != encoding[j]) {
						equal = false;
						break;
					}
				}
				if (equal) return true;
			}
			return false;
		}

		if (map.count(encoding[bit]) <= 0)
			return false;

		return map.at(encoding[bit]).lookup(encoding, bit + 1);
	}

	void insert(const Tet& t) {
		auto x = t.volumeEncode();
		add(x);
		items++;
	}

	void add(const std::vector<uint64_t>& encoding, unsigned bit = 0) {
		if (bit == encoding.size()) {
			if (store.size() > 1)
				collisions++;
			store.push_back(encoding);
			loadtot++;
			if (store.size() > loadhi)
				loadhi = store.size();
			return;
		}

		if (map.count(encoding[bit]) <= 0) {
			map.emplace(std::piecewise_construct, std::make_tuple(encoding[bit]), std::make_tuple());
		}
		map[encoding[bit]].add(encoding, bit + 1);
	}
};

template<>
struct NestedHash<0> {
	std::vector<std::vector<uint64_t>> store;

	inline static uint64_t loadhi = 0;
	inline static uint64_t count = 0;
	inline static uint64_t loadtot = 0;

	static void print() {
		std::cout << "Depth " << 0 << " total load " << loadtot << " highest load " << loadhi << " count " << count
				  << std::endl;
	}

	NestedHash() {
		tables++;
		count++;
	}

	static void reset() {
		loadhi = 0;
		count = 0;
		tables = 0;
		comparisons = 0;
		items = 0;
		collisions = 0;
		loadtot = 0;
	}

	bool contains(const Tet& t) const {
		return lookup(t.volumeEncode(), 0);
	}

	bool lookup(const std::vector<uint64_t>& encoding, unsigned) const {
		for (int i = 0; i < store.size(); ++i) {
			comparisons++;
			bool equal = true;
			for (int j = 0; j < encoding.size(); ++j) {
				if (store[i][j] != encoding[j]) {
					equal = false;
					break;
				}
			}
			if (equal) return true;
		}
		return false;
	}

	void insert(const Tet& t) {
		add(t.volumeEncode(), 0);
	}

	void add(const std::vector<uint64_t>& encoding, unsigned) {
		if (store.size() > 1)
			collisions++;
		store.push_back(encoding);
		loadtot++;
		if (store.size() > loadhi)
			loadhi = store.size();
	}
};

bool compareTet(const Tet& a, const Tet& b) {
	for (int i = 0; i < a.n; ++i) {
		if (a.coords[i] != b.coords[i])
			return false;
	}
	return true;
}

std::vector<Tet> generate(unsigned int i) {
	if (i <= 1) {
		return std::vector{Tet{1, {{0, 0, 0}}}};
	}

	auto previous = generate(i - 1);

	constexpr int depth = 1;

	long long int counter = 0;

	NestedHash<depth> cache;
	std::vector<Tet> unique;
	std::cout << "Size: " << previous.size() << std::endl;

	//pad previous with empty Tets for work-sharing
	int fill = numThreads - previous.size() % numThreads;
	for (int j = 0; j < fill; ++j) {
		previous.emplace_back();
	}

	std::cout << "Padded: " << previous.size() << std::endl;

	NestedHash<1> stepUnique;
	for (int j = 0; j < previous.size(); j += numThreads) {

		if (!((j / numThreads) % 30)){
			stepUnique = NestedHash<1>{};
		}

		#pragma omp parallel default(none) shared(previous, j, stepUnique, cache, unique)
		{
			const auto& p = previous[j + omp_get_thread_num()];

			auto spaces = p.getFreeSpaces();
			std::vector<Tet> uniqueBuilds;

			for (auto& f: p.getFreeSpaces()) {
				Tet build = p.insert(f);
				Tet max = getMaxRotation(build);

				if (!cache.contains(max)) {
					uniqueBuilds.push_back(max);
				}
			}

			//prevent writing while some threads might still be reading
			#pragma omp barrier

			//all threads have now found their unique children
			//now compare against those from other threads that have already been written
			#pragma omp critical
			for (auto& m: uniqueBuilds) {
				if (!stepUnique.contains(m)) {
					stepUnique.insert(m);
					cache.insert(m);
					unique.push_back(m);
				}
			}
		}
	}


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
