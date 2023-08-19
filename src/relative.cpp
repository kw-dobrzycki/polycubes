//
// Created by croissant on 24/07/2023.
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include "relative.h"
#include "groups.h"
#include "Tet.h"
#include "bloom_filter.hpp"

const unsigned* const selfGroupOf{SelfGroup::generateSelfGroups()};
const unsigned* const localGroupOf{LocalGroup::generateLocalGroups()};

unsigned opposite[]{
		5, 3, 4, 1, 2, 0
};

const Pos offsets[]{
		{0,  1,  0},
		{0,  0,  -1},
		{1,  0,  0},
		{0,  0,  1},
		{-1, 0,  0},
		{0,  -1, 0},
};

auto compareLinearCoordinates = sparseCompare<unsigned, 265000, 50>;

unsigned toLinear(const Pos& p) {
	unsigned b = 0;
	b = (b & ~0x3F000) | (p.x & 0x3F) << 12;
	b = (b & ~0xFC0) | (p.y & 0x3F) << 6;
	b = (b & ~0x3F) | (p.z & 0x3F);
	return b;
}

void toLinearVector(const Tet& t, std::vector<unsigned>& dst) {
	for (int i = 0; i < t.n; ++i) {
		dst[i] = toLinear(t.coords[i]);
	}
}

std::pair<
		std::pair<std::vector<unsigned int>, std::vector<unsigned int>>, //first rarest
		std::pair<std::vector<unsigned int>, std::vector<unsigned int>>  //second rarest
>
getRareSeeds(const std::vector<unsigned int>& A, const std::vector<unsigned int>& B) {
	thread_local int* countA = new int[43451]();
	thread_local int* countB = new int[43451]();
	thread_local unsigned* indices = new unsigned[50]();
	int items = 0;

	for (int i = 0; i < A.size(); ++i) {
		countA[A[i]]++;
		countB[B[i]]++;
		indices[items++] = A[i];
		indices[items++] = B[i];
	}

	countA[43450] = std::max(A.size(), B.size()) + 1;
	countB[43450] = countA[43450];

	unsigned min1 = 43450;
	unsigned min2 = min1;
	for (int i = 0; i < items; ++i) {

		//get argmin
		if (countA[indices[i]] * countB[indices[i]]) { //if in both
			auto a = std::min(countA[indices[i]], countB[indices[i]]);
			auto b = std::min(countA[min1], countB[min1]);
			auto c = std::min(countA[min2], countB[min2]);

			if (a < b) {
				min2 = min1;
				min1 = indices[i];
			} else if (a < c && indices[i] != min1) {
				min2 = indices[i];
			}
		}

		//and if it's the lowest possible, break
		if (countA[min2] == 1 || countB[min2] == 1) {
			break;
		}
	}

	//clean up
	for (int i = 0; i < items; ++i) {
		countA[indices[i]] = 0;
		countB[indices[i]] = 0;
		indices[i] = 0;
	}

	std::vector<unsigned> a1;
	std::vector<unsigned> a2;
	std::vector<unsigned> b1;
	std::vector<unsigned> b2;

	for (int i = 0; i < A.size(); ++i) {
		//if element in A belongs to the rarest group
		if (A[i] == min1) {
			a1.push_back(i);
		}

		if (A[i] == min2) {
			a2.push_back(i);
		}

		if (B[i] == min1) {
			b1.push_back(i);
		}

		if (B[i] == min2) {
			b2.push_back(i);
		}
	}

	return {{a1, b1},
			{a2, b2}};
}

Pos& rotX(Pos& p, unsigned i = 1) {
	for (int j = 0; j < i; ++j) {
		auto t = p.z;
		p.z = -p.y;
		p.y = t;
	}
	return p;
}

Pos& rotY(Pos& p, unsigned i = 1) {
	for (int j = 0; j < i; ++j) {
		auto t = p.x;
		p.x = -p.z;
		p.z = t;
	}
	return p;
}

Pos& rotZ(Pos& p, unsigned i = 1) {
	for (int j = 0; j < i; ++j) {
		auto t = p.y;
		p.y = -p.x;
		p.x = t;
	}
	return p;
}

bool markerIsIn(const Pos& marker, const std::vector<unsigned> v) {
	auto m = toLinear(marker);
	for (int i = 0; i < v.size(); ++i) {
		if (v[i] == m) {
			return true;
		}
	}
	return false;
}

bool fullCompareRotateSeeds(const std::vector<unsigned>& referenceLinear,
							const std::vector<unsigned>& referenceLinearSeeds,
							const Tet& B,
							const std::vector<unsigned>& rebaseB,
							const std::vector<unsigned>& compareB) {


	for (int i = 0; i < rebaseB.size(); ++i) {

		//choose a non-origin piece to test
		Pos marker = B.coords[compareB[(i + 1) % compareB.size()]] - B.coords[rebaseB[i]];

		std::vector<unsigned> rot1;
		std::vector<unsigned> rot2;

		//spin on top
		for (int j = 0; j < 4; ++j) {
			if (markerIsIn(marker, referenceLinearSeeds)) {
				rot1.push_back(0);
				rot2.push_back(j);
			}
			rotY(marker);
		}

		//spin on back
		rotX(marker);
		for (int j = 0; j < 4; ++j) {
			if (markerIsIn(marker, referenceLinearSeeds)) {
				rot1.push_back(1);
				rot2.push_back(j);
			}
			rotZ(marker);
		}

		//spin on bottom
		rotX(marker);
		for (int j = 0; j < 4; ++j) {
			if (markerIsIn(marker, referenceLinearSeeds)) {
				rot1.push_back(5);
				rot2.push_back(j);
			}
			rotY(marker);
		}

		//spin on front
		rotX(marker);
		for (int j = 0; j < 4; ++j) {
			if (markerIsIn(marker, referenceLinearSeeds)) {
				rot1.push_back(3);
				rot2.push_back(j);
			}
			rotZ(marker);
		}

		//spin on right
		rotX(marker);
		rotZ(marker);
		for (int j = 0; j < 4; ++j) {
			if (markerIsIn(marker, referenceLinearSeeds)) {
				rot1.push_back(2);
				rot2.push_back(j);
			}
			rotX(marker);
		}

		//spin on left
		rotZ(marker);
		rotZ(marker);
		for (int j = 0; j < 4; ++j) {
			if (markerIsIn(marker, referenceLinearSeeds)) {
				rot1.push_back(4);
				rot2.push_back(j);
			}
			rotX(marker);
		}

		//now test all of B using potential matches
		std::vector<unsigned> comparatorLinear(B.n);
		for (int j = 0; j < rot1.size(); ++j) {
			Tet comparator = B;
			//rebase comparator as well
			auto seed = B.coords[rebaseB[i]];
			for (int k = 0; k < B.n; ++k) {
				comparator.coords[k] = comparator.coords[k] - seed;
			}
			switch (rot1[j]) {
				case 0:
					comparator.rotY(rot2[j]);
					break;
				case 1:
					comparator.rotX();
					comparator.rotZ(rot2[j]);
					break;
				case 2:
					comparator.rotZ();
					comparator.rotX(rot2[j]);
					break;
				case 3:
					comparator.rotX();
					comparator.rotX();
					comparator.rotX();
					comparator.rotZ(rot2[j]);
					break;
				case 4:
					comparator.rotZ();
					comparator.rotZ();
					comparator.rotZ();
					comparator.rotX(rot2[j]);
					break;
				case 5:
					comparator.rotX();
					comparator.rotX();
					comparator.rotY(rot2[j]);
					break;
			}

			toLinearVector(comparator, comparatorLinear);
			if (compareLinearCoordinates(referenceLinear, comparatorLinear)) {
				return true;
			}
		}
	}

	return false;
}

bool fullCompare(const Tet& A, const Tet& B) {
	/* translation equivalence: relative coordinates occupy the same space*/
	auto seeds = getRareSeeds(A.encodeSelf(), B.encodeSelf()); //todo perhaps could use local here
	const auto& rare1 = seeds.first;
	const auto& rare2 = seeds.second;

	if (rare1.first.size() != rare1.second.size() ||
		rare2.first.size() != rare2.second.size() ||
		rare1.first.empty())
		return false;

	std::vector<unsigned> rebaseB;
	std::vector<unsigned> compareB;

	auto* rarity = &rare2;
	if (rare1.first.size() > 1) {
		rarity = &rare1;

		rebaseB = rare1.second;
		compareB = rare1.second;
	} else {
		rebaseB = {rare1.second[0]};
		compareB = {rare2.second[0]};
	}

	//get rebased linear A seeds (rebased on the same rarity that is used to test B)
	auto seedA = A.coords[rare1.first[0]];
	std::vector<unsigned> referenceLinearSeeds(rarity->first.size());
	for (int i = 0; i < rarity->first.size(); ++i) {
		referenceLinearSeeds[i] = toLinear(A.coords[rarity->first[i]] - seedA);
	}

	//get rebased linear A
	std::vector<unsigned> referenceLinear(A.n);
	for (int i = 0; i < A.n; ++i) {
		referenceLinear[i] = toLinear(A.coords[i] - seedA);
	}

	return fullCompareRotateSeeds(referenceLinear, referenceLinearSeeds, B, rebaseB, compareB);
}

unsigned getRarestLocalType(const Tet& t) {
	thread_local auto* count = new unsigned int[43451]();
	auto code = t.encodeLocal();

	for (int i = 0; i < t.n; ++i) {
		count[code[i]]++;
	}

	unsigned min = 43450;
	count[min] = t.n + 1;

	for (int i = 0; i < t.n; ++i) {
		if (count[code[i]] > count[min]) {
			min = i;
		}
		count[code[i]] = 0;
	}

	return min;
}

unsigned getHighestLocalType(const Tet& t) {
	const auto& x = t.encodeLocal();
	unsigned m = 0;
	for (int i = 0; i < t.n; ++i) {
		if (x[i] > x[m]) {
			m = i;
		}
	}
	return x[m];
}

struct _compare {
	std::array<int, 6> bounds;

	bool operator()(Pos a, Pos b) {
		auto X = bounds[1] - bounds[0] + 1;
		auto Z = bounds[5] - bounds[4] + 1;

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

	std::sort(A.begin(), A.end(), _compare{boundsA});
	std::sort(B.begin(), B.end(), _compare{boundsB});

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

unsigned hashBound(std::array<int, 6> bound) {
	unsigned hash = 0;
	hash = hash & ~0xFF0000 | ((bound[1] - bound[0]) & 0xFF) << 16;
	hash = hash & ~0xFF00 | ((bound[3] - bound[2]) & 0xFF) << 8;
	hash = hash & ~0xFF | (bound[5] - bound[4]);
	return hash;
}

struct CachedBound {
	bloom_filter filter;
	std::vector<Tet> store;
	unsigned hashedBound;
	unsigned long long capacity = 1000;

	CachedBound(unsigned hashedBound) : hashedBound(hashedBound) {
		bloom_parameters param;
		param.projected_element_count = capacity;
		param.false_positive_probability = 0.0001;
		param.compute_optimal_parameters();
		filter = bloom_filter(param);
	}

	bool contains(const Tet& tet) const {
		return filter.contains(tet.boundEncode());
	}

	void insert(const Tet& tet) {
		if (store.size() == capacity) {
			bloom_parameters param;
			param.projected_element_count = capacity * 2;
			param.false_positive_probability = 0.000001;
			filter = bloom_filter(param);
			for (int i = 0; i < store.size(); ++i) {
				filter.insert(store[i]);
			}
		}

		filter.insert(tet.boundEncode());
		store.push_back(tet);
	}
};

struct CachedUnique {
	Tet unique;
	std::vector<LocalGroup::type> code;
	std::vector<LocalGroup::type> complementCode;
};

std::vector<Tet> generate(unsigned int i) {
	if (i <= 1) {
		return std::vector{Tet(i, std::vector<Pos>(1, {0, 0, 0}))};
	}

	auto previous = generate(i - 1);

	std::map<unsigned, CachedBound> cache;

	long long int skipped = 0;
	long long int localSkip = 0;
	long long int inverseSkip = 0;
	long long int boundsSkip = 0;
	long long int popSkip = 0;
	long long int full = 0;
	long long int fullFalse = 0;

	long long unsigned newShapeCount = 0;
	long long int k = 0;
	for (auto& p: previous) {
		if (!(++k % 100)) {
			std::cout << "n = " << i << ": " << (float) k / previous.size() << " with " << newShapeCount
					  << " new unique shapes" << std::endl;
			newShapeCount = 0;
		}

		auto faces = p.getFreeSpaces();

		for (auto& f: faces) {
			Tet build(p.insert(f));
			Tet max = getMaxRotation(build);

			auto boundhash = hashBound(max.getBounds());
			if (!cache.contains(boundhash)) {
				cache.insert({boundhash, boundhash});
			}

			auto& cached = cache.at(boundhash);

			if (!cached.contains(max)) {
				cached.insert({max.n, max.coords});
				newShapeCount++;
			} else {

				auto buildCode = build.encodeLocal();

				Tet buildComplement = build.getComplement();
				auto buildComplementCode = buildComplement.encodeLocal();

				bool newShape = false;

				for (int j = 0; j < cached.store.size(); ++j) {

				}

				if (newShape) {
					newShapeCount++;
				}
			}
		}
	}

	std::vector<Tet> allUnique;
	for (const auto& [bound, c]: cache) {
		for (int j = 0; j < c.store.size(); ++j) {
			allUnique.push_back(c.store[j]);
		}
	}

	std::cout << "n = " << i << "\n";
	std::cout <<  allUnique.size() << " unique shapes\n";
	std::cout << "Full comparisons skipped: " << skipped << "\n";
	std::cout
			<< "Full comparisons computed: " << full << "\n"
			<< "Full comparisons false: " << (float) fullFalse / full * 100
			<< "\ninverse: " << (float) inverseSkip / skipped * 100
			<< "\nlocal: " << (float) localSkip / skipped * 100
			<< "\npopulation: " << (float) popSkip / skipped * 100
			<< "\nbounds: " << (float) boundsSkip / skipped * 100
			<< std::endl;




	return allUnique;

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
	std::ifstream file(std::string{path});
	std::string line;

	std::vector<Tet> tets;
	std::vector<Pos> blocks;
	std::cout << "reading" << std::endl;
	while (std::getline(file, line, '\n')) {
		if (line.empty()) {
			tets.emplace_back(blocks.size(), blocks);
			blocks.clear();
			continue;
		}
		auto spaced = split(line, ' ');
		blocks.push_back({std::stoi(spaced[0]), std::stoi(spaced[1]), std::stoi(spaced[2])});
	}

	std::cout << "read " << tets.size() << " shapes";
	return tets;
}
