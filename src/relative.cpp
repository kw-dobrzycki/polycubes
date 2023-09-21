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

static unsigned long long collisions = 0;
static unsigned long long comparisons = 0;
static unsigned long long tables = 0;
static unsigned long long items = 0;

template<unsigned depth>
struct NestedHash {
	std::unordered_map<uint64_t, NestedHash<depth - 1>> map;
	std::vector<uint64_t> store;

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
		auto& x = t.volumeEncoding;
		return lookup(x);
	}

	bool lookup(const std::vector<uint64_t>& encoding, unsigned bit = 0) const {
		if (bit == encoding.size()) {
			for (unsigned long long i: store) {
				comparisons++;
				if (i == encoding[bit - 1]) return true;
			}
			return false;
		}

		if (map.count(encoding[bit]) <= 0)
			return false;

		return map.at(encoding[bit]).lookup(encoding, bit + 1);
	}

	void insert(const Tet& t) {
		auto& x = t.volumeEncoding;
		add(x);
		items++;
	}

	void add(const std::vector<uint64_t>& encoding, unsigned bit = 0) {
		if (bit == encoding.size()) {
			if (store.size() > 1)
				collisions++;
			store.push_back(encoding[bit - 1]);
			loadtot++;
			if (store.size() > loadhi)
				loadhi = store.size();
			return;
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
		return lookup(t.volumeEncoding, 0);
	}

	bool lookup(const std::vector<uint64_t>& encoding, unsigned) const {
		for (const auto& i: store) {
			comparisons++;
			if (i == encoding) return true;
		}
		return false;
	}

	void insert(const Tet& t) {
		add(t.volumeEncoding, 0);
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

uint64_t encodeBound(Pos bound) {
	std::vector<Pos::type> v{bound.x, bound.y, bound.z};
	std::sort(v.begin(), v.end());
	bound.x = v[2];
	bound.z = v[1];
	bound.y = v[0];
	uint64_t e = 0;
	e |= bound.x << Pos::width * 2;
	e |= bound.y << Pos::width;
	e |= bound.z;
	return e;
}

template<unsigned depth>
struct BoundCache {

	std::unordered_map<uint64_t, NestedHash<depth>> caches;

	explicit BoundCache(unsigned n) : caches(allocCaches(n)) {}

private:
	std::unordered_map<uint64_t, NestedHash<depth>> allocCaches(Pos::type n) { //assert range
		std::unordered_map<uint64_t, NestedHash<depth>> cache;
		for (Pos::type i = 1; i < n + 1; ++i) {
			for (Pos::type j = 1; j < n + 1; ++j) {
				for (Pos::type k = 1; k < n + 1; ++k) {

					//if enough volume for n pieces
					if (i * j * k >= n) {

						//get canonical orientation
						auto encoding = encodeBound(Pos{i, j, k});
						if (!caches.contains(encoding)) {
							cache.insert({encoding, {}});
						}
					}
				}
			}
		}
		return cache;
	}
};

struct LocalBoundCache {
	std::unordered_map<uint64_t, std::vector<Tet>> caches;

	LocalBoundCache(unsigned n) : caches(allocCaches(n)) {}

	void clear() {
		for (auto& [k, v]: caches) {
			v.clear();
		}
	}

private:
	std::unordered_map<uint64_t, std::vector<Tet>> allocCaches(Pos::type n) { //assert range
		std::unordered_map<uint64_t, std::vector<Tet>> cache;
		for (Pos::type i = 1; i < n + 1; ++i) {
			for (Pos::type j = 1; j < n + 1; ++j) {
				for (Pos::type k = 1; k < n + 1; ++k) {

					//if enough volume for n pieces
					if (i * j * k >= n) {

						//get canonical orientation
						auto encoding = encodeBound(Pos{i, j, k});
						if (!caches.contains(encoding)) {
							cache.insert({encoding, {}});
						}
					}
				}
			}
		}
		return cache;
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
	maxComparisons = 0;

	BoundCache<depth> cache(i);
	std::vector<Tet> unique;

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
	#pragma omp parallel default(none) shared(chunksize, previous, cache, local, unique, numThreads)
	for (unsigned j = omp_get_thread_num() * chunksize; j < (omp_get_thread_num() + 1) * chunksize; ++j) {

		const auto& p = previous[j];
		LocalBoundCache& results = local[omp_get_thread_num()];
		results.clear();

		for (const auto& f: p.getFreeSpaces()) {
			Tet build = p.insert(f);
			Tet max = getMaxRotation(build);
			max.volumeEncoding = max.volumeEncode();

			auto bounds = max.getBounds();
			auto encoding = encodeBound({static_cast<Pos::type>(bounds[1] - bounds[0] + 1),
										 static_cast<Pos::type>(bounds[3] - bounds[2] + 1),
										 static_cast<Pos::type>(bounds[5] - bounds[4] + 1)
										});
			if (!encoding) continue;
			const auto& c = cache.caches.at(encoding);

			if (!c.contains(max)) {
				results.caches.at(encoding).push_back(max);
			}
		}

		#pragma omp barrier
		#pragma omp single
		{
			for (auto& [key, c]: cache.caches) { //todo make this parallel, move unique elsewhere
				for (int m = 0; m < numThreads; ++m) {
					for (const auto& res: local[m].caches.at(key)) {
						if (!c.contains(res)) {
							c.insert(res);
							unique.push_back(res);
						}
					}
				}
			}
		}
		//implied barrier
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
