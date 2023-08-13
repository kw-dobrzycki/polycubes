//
// Created by croissant on 24/07/2023.
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "relative.h"
#include "groups.h"
#include "Tet.h"

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

bool fullCompare(const Tet& A, const Tet& B) {
	/* translation equivalence: relative coordinates occupy the same space
	 * 1) Find shared piece types which occur least commonly (seeds)
	 * 2) Set A on a seed. Set B on every seed, each time:
	 * 3) Rotate B, recalculate linear coordinates and compare
	 */

	auto minSeeds = getRareSeeds(A.encodeLocal(), B.encodeLocal());

	if (minSeeds.first.empty())
		return false;

	std::vector<unsigned> reference(A.n);
	Tet relative = A;
	Tet comparator = B;
	std::vector<unsigned> seeds = minSeeds.second;

	if (minSeeds.first.size() <= minSeeds.second.size()) {
		//rebase B (as reference)
		auto seed = B.coords[minSeeds.second[0]];
		for (int i = 0; i < B.n; ++i) {
			reference[i] = toLinear(B.coords[i] - seed);
			relative.coords[i] = B.coords[i] - seed;
		}
		comparator = A;
		seeds = minSeeds.first;

	} else {
		//rebase A (as reference)
		auto seed = A.coords[minSeeds.first[0]];
		for (int i = 0; i < A.n; ++i) {
			reference[i] = toLinear(A.coords[i] - seed);
			relative.coords[i] = A.coords[i] - seed;
		}
	}

	std::vector<unsigned> rotated(comparator.n);

	for (int i = 0; i < seeds.size(); ++i) {
		auto seed = comparator.coords[seeds[i]];

		//rebase comparator
		for (int j = 0; j < comparator.n; ++j) {
			comparator.coords[j] = comparator.coords[j] - seed;
		}

		//rotate, linearise and compare
		//spin on top
		for (int j = 0; j < 4; ++j) {
			comparator.rotY();
			toLinearVector(comparator, rotated);
			if (compareLinearCoordinates(reference, rotated))
				return true;
		}

		//spin on back
		comparator.rotX();
		for (int j = 0; j < 4; ++j) {
			comparator.rotZ();
			toLinearVector(comparator, rotated);
			if (compareLinearCoordinates(reference, rotated))
				return true;
		}

		//spin on right
		comparator.rotY();
		for (int j = 0; j < 4; ++j) {
			comparator.rotX();
			toLinearVector(comparator, rotated);
			if (compareLinearCoordinates(reference, rotated))
				return true;
		}

		//spin on front
		comparator.rotY();
		for (int j = 0; j < 4; ++j) {
			comparator.rotZ();
			toLinearVector(comparator, rotated);
			if (compareLinearCoordinates(reference, rotated))
				return true;
		}

		//spin on left
		comparator.rotY();
		for (int j = 0; j < 4; ++j) {
			comparator.rotX();
			toLinearVector(comparator, rotated);
			if (compareLinearCoordinates(reference, rotated))
				return true;
		}

		//spin on bottom
		comparator.rotY();
		comparator.rotX();
		for (int j = 0; j < 4; ++j) {
			comparator.rotY();
			toLinearVector(comparator, rotated);
			if (compareLinearCoordinates(reference, rotated))
				return true;
		}
	}

	return false;
}

std::pair<std::vector<unsigned int>, std::vector<unsigned int>>
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

	countA[43450] = A.size() + 1;
	countB[43450] = A.size() + 1;

	unsigned min = 43450;
	for (int i = 0; i < items; ++i) {

		//get argmin
		if (countA[indices[i]] * countB[indices[i]]) { //if in both
			auto a = std::min(countA[indices[i]], countB[indices[i]]);
			auto b = std::min(countA[min], countB[min]);
			if (a < b) {
				min = indices[i];
			}
		}

		//and if it's the lowest possible, break
		if (countA[min] == 1 || countB[min] == 1) {
			break;
		}
	}

	//clean up
	for (int i = 0; i < items; ++i) {
		countA[indices[i]] = 0;
		countB[indices[i]] = 0;
		indices[i] = 0;
	}

	std::vector<unsigned> a;
	std::vector<unsigned> b;

	for (int i = 0; i < A.size(); ++i) {
		//if element in A belongs to the rarest group
		if (A[i] == min) {
			a.push_back(i);
		}

		if (B[i] == min) {
			b.push_back(i);
		}
	}

	return {a, b};
}

bool compareBounds(const Tet& a, const Tet& b) {
	auto ar = a.getBounds();
	auto br = b.getBounds();

	for (int i = 0; i < 3; ++i) {
		if (ar[i] != br[i])
			return false;
	}

	return true;
}

std::vector<Tet> generate(unsigned int i) {
	if (i <= 1) {
		return std::vector{Tet(i, std::vector<Pos>(1, {0, 0, 0}))};
	}

	auto previous = generate(i - 1);
	std::vector<Tet> unique;
	std::vector<std::vector<LocalGroup::type>> uniqueCode;
	std::vector<std::vector<LocalGroup::type>> uniqueComplementCode;

	int skipped = 0;
	int localSkip = 0;
	int inverseSkip = 0;
	int boundsSkip = 0;
	int popSkip = 0;

	int prev = 0;
	int k = 0;
	for (auto& p: previous) {
		if (!(++k % 100)) {
			std::cout << "n = " << i << ": " << (float) k / previous.size() << " with " << unique.size() - prev
					  << " new unique shapes" << std::endl;
			prev = unique.size();
		}

		auto faces = p.getFreeSpaces();

		for (auto& f: faces) {
			Tet build(p.insert(f));
			auto buildCode = build.encodeLocal();

			Tet buildComplement = build.getComplement();
			auto buildComplementCode = buildComplement.encodeLocal();

			bool newShape = true;

			int isLocalEqual = false;
			int z = -1;

			for (int j = 0; j < unique.size(); ++j) {
				const auto& u = unique[j];
				const auto& uCode = uniqueCode[j];
				const auto& uComplementCode = uniqueComplementCode[j];

				z++;

				//if there's a new group, keep true, check next u
				if (!compareLocalEncodings(uCode, buildCode)) {
					skipped++;
					localSkip++;
					continue;
				}

				isLocalEqual = z;

				if (!comparePopulations(u.population, build.population)){
					skipped++;
					popSkip++;
					continue;
				}

				if (!compareBounds(u, build)) {
					skipped++;
					boundsSkip++;
					continue;
				}

				if (uComplementCode.size() == buildComplementCode.size() &&
					!compareLocalEncodings(uComplementCode, buildComplementCode)) {
					skipped++;
					inverseSkip++;
					continue;
				}

				//if all groups match, spin
				//if same at some rotation, set false, break
				if (fullCompare(u, build)) {
					newShape = false;
					break;
				}
			}

			if (newShape) {
				unique.push_back(build);
				uniqueCode.push_back(buildCode);
				uniqueComplementCode.push_back(buildComplementCode);
				z++;
			}
		}
	}

	std::cout << "n = " << i << "\n";
	std::cout << unique.size() << " unique shapes\n";
	std::cout << "Full comparisons skipped: " << skipped << "\n";
	std::cout
			<< "local: " << (float) localSkip / skipped * 100
			<< "\ninverse: " << (float) inverseSkip / skipped * 100
			<< "\nbounds: " << (float) boundsSkip / skipped * 100
			<< "\npopulation: " << (float) popSkip / skipped * 100
			<< std::endl;

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
