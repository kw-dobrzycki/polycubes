//
// Created by croissant on 24/07/2023.
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "relative.h"
#include "groups.h"

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

Tet::Tet(unsigned int n, const std::vector<Pos>& coordinates)
		: n(n),
		  pieces(n),
		  coords(coordinates),
		  neighbours(6 * n) {

	//decode connectivity (name the pieces for now)
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {

			//go through each face and check for a block at that coordinate
			for (int k = 0; k < n; ++k) {
				if (coordinates[i] + offsets[j] == coordinates[k]) {
					pieces[i] |= 1 << (5 - j) * 4; //mark as having a neighbour
					neighbours[i * 6 + j] = k + 1; //just like names but these don't change
					break; //no other neighbour on that face
				}
			}
		}
	}

	//get self type and update neighbours
	for (int i = 0; i < n; ++i) {
		int self = 0;

		for (int j = 0; j < 6; ++j) {
			if (pieces[i] & 0xF << j * 4) {
				self += 1 << j;
			}
		}
		auto group = selfGroupOf[self];

		//update neighbours
		for (int j = 0; j < 6; ++j) {
			const auto ni = neighbours[i * 6 + j];
			if (ni) {
				auto& neighbour = pieces[ni - 1];
				neighbour = neighbour & ~(0xF << (5 - opposite[j]) * 4) | group << (5 - opposite[j]) * 4;
			}
		}
	}
}

Tet::Tet(unsigned int n, const std::vector<uint32_t>& pieces, const std::vector<Pos>& coords,
		 const std::vector<int>& neighbours)
		: n(n),
		  pieces(pieces),
		  coords(coords),
		  neighbours(neighbours) {}

Tet& Tet::rotX() {
	for (int j = 0; j < n; ++j) {
		auto t = coords[j].z;
		coords[j].z = -coords[j].y;
		coords[j].y = t;
	}
	return *this;
}

Tet& Tet::rotY() {
	for (int j = 0; j < n; ++j) {
		auto t = coords[j].x;
		coords[j].x = -coords[j].z;
		coords[j].z = t;
	}
	return *this;
}

Tet& Tet::rotZ() {
	for (int j = 0; j < n; ++j) {
		auto t = coords[j].y;
		coords[j].y = -coords[j].x;
		coords[j].x = t;
	}
	return *this;
}

Tet Tet::getComplement() const {
	int ax = 0, ay = 0, az = 0, aX = 0, aY = 0, aZ = 0;

	for (int i = 0; i < n; ++i) {
		ax = std::min(ax, coords[i].x);
		ay = std::min(ay, coords[i].y);
		az = std::min(az, coords[i].z);
		aX = std::max(aX, coords[i].x);
		aY = std::max(aY, coords[i].y);
		aZ = std::max(aZ, coords[i].z);
	}

	std::vector<Pos> complement;

	for (int i = ax; i <= aX; ++i) {
		for (int j = ay; j <= aY; ++j) {
			for (int k = az; k <= aZ; ++k) {
				bool taken = false;
				for (const auto& p: coords) {
					if (p.x == i && p.y == j && p.z == k) {
						taken = true;
						break;
					}
				}
				if (!taken) {
					complement.push_back({i, j, k});
				}
			}
		}
	}

	return {static_cast<unsigned int>(complement.size()), complement};
}

std::vector<Pos> Tet::getFreeSpaces() const {
	std::set<int> unique;
	std::vector<Pos> spaces;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {
			if (!neighbours[i * 6 + j]) {
				Pos space = coords[i] + offsets[j];
				bool seen = unique.contains(space.y * n * n + space.z * n + space.x);
				if (!seen) {
					unique.insert(seen);
					spaces.push_back(space);
				}
			}
		}
	}

	return spaces;
}

Tet Tet::insert(const Pos& block) const {
	auto c_pieces = pieces;
	auto c_coords = coords;
	auto c_neighbours = neighbours;

	c_pieces.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_coords.push_back(block);

	//find neighbours
	for (int i = 0; i < 6; ++i) {
		for (int j = 0; j < n; ++j) {
			if (block + offsets[i] == coords[j]) {
				c_neighbours[n * 6 + i] = j + 1; //names start at 1
				c_neighbours[j * 6 + opposite[i]] = n + 1;
			}
		}
	}

	int selfType = 0; //depends on the piece type of neighbours, not neighbourhood type
	for (int k = 0; k < 6; ++k) {
		selfType += (c_neighbours[n * 6 + k] > 0) << (5 - k);
	}


	//change the 0 bits of direct neighbours to the self group of new block.
	for (int i = 0; i < 6; ++i) {
		int dirNeighbour = c_neighbours[n * 6 + i];
		if (dirNeighbour) {
			c_pieces[(dirNeighbour - 1)] += selfGroupOf[selfType] << (5 - opposite[i]) * 4;

			//compute new self type of the neighbour for indirect neighbours to update (this includes block)
			int neighbourType = 0;
			for (int k = 0; k < 6; ++k) {
				neighbourType += (c_neighbours[(dirNeighbour - 1) * 6 + k] > 0) << (5 - k);
			}


			//change the non-0 bits of indirect neighbours
			for (int j = 0; j < 6; ++j) {
				int indirNeighbour = c_neighbours[(dirNeighbour - 1) * 6 + j];
				if (indirNeighbour) {
					c_pieces[(indirNeighbour - 1)] =
							(c_pieces[(indirNeighbour - 1)] & ~(0xF << (5 - opposite[j]) * 4)) |
							(selfGroupOf[neighbourType] << (5 - opposite[j]) * 4);
				}
			}
		}
	}

	return Tet{n + 1, c_pieces, c_coords, c_neighbours};
}

std::vector<unsigned> Tet::encodeLocal() const {
	std::vector<unsigned> code(n);
	for (int i = 0; i < n; ++i) {
		code[i] = localGroupOf[LocalGroup::toDen(pieces[i])];
	}
	return code;
}

std::vector<unsigned> Tet::encodeSelf() const {
	std::vector<unsigned> code(n);
	for (int i = 0; i < n; ++i) {
		int self = 0;

		for (int j = 0; j < 6; ++j) {
			if (pieces[i] & (0xF << j * 4)) {
				self += 1 << j;
			}
		}

		code[i] = selfGroupOf[self];
	}
	return code;
}

void Tet::print() const {
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {
			std::cout << pieces[i * 6 + j];
		}
		std::cout << "|";
	}
	std::cout << std::endl;
}

std::array<int, 3> Tet::getBounds() const {
	int ax = 0, ay = 0, az = 0, aX = 0, aY = 0, aZ = 0;

	for (int i = 0; i < n; ++i) {
		ax = std::min(ax, coords[i].x);
		ay = std::min(ay, coords[i].y);
		az = std::min(az, coords[i].z);
		aX = std::max(aX, coords[i].x);
		aY = std::max(aY, coords[i].y);
		aZ = std::max(aZ, coords[i].z);
	}

	aX -= ax;
	aY -= ay;
	aZ -= az;

	int r[]{aX, aY, aZ};
	std::sort(r, r + 3);
	return {r[0], r[1], r[2]};
}

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

bool compareLinearCoordinates(const std::vector<unsigned>& A, const std::vector<unsigned>& B) {
	thread_local int* counters = new int[265000]();
	thread_local unsigned* indices = new unsigned[50]();
	int items = 0;

	for (int i = 0; i < A.size(); ++i) {
		counters[A[i]]++;
		counters[B[i]]--;
		indices[items++] = A[i];
		indices[items++] = B[i];
	}

	bool equal = true;
	int tail = 0;
	for (int i = 0; i < items; ++i) {
		if (counters[indices[i]] != 0) {
			equal = false;
			tail = i;
			break;
		}

		//counter is 0 but index is not, clean up index
		indices[i] = 0;
	}

	//clean up the rest
	for (int i = tail; i < items; ++i) {
		counters[indices[i]] = 0;
		indices[i] = 0;
	}

	return equal;
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

bool compareLocalEncodings(const std::vector<unsigned>& A, const std::vector<unsigned>& B) {
	thread_local int* counters = new int[43450]();
	thread_local unsigned* indices = new unsigned[8000]();
	int items = 0;

	for (int i = 0; i < A.size(); ++i) {
		counters[A[i]]++;
		counters[B[i]]--;
		indices[items++] = A[i];
		indices[items++] = B[i];
	}

	bool equal = true;
	int tail = 0;
	for (int i = 0; i < items; ++i) {
		if (counters[indices[i]] != 0) {
			equal = false;
			tail = i;
			break;
		}

		//counter is 0 but index is not, clean up index
		indices[i] = 0;
	}

	//clean up the rest
	for (int i = tail; i < items; ++i) {
		counters[indices[i]] = 0;
		indices[i] = 0;
	}

	return equal;
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

std::vector<Tet> generateCompl(unsigned int i) {
	if (i <= 1) {
		return std::vector{Tet(i, std::vector<Pos>(1, {0, 0, 0}))};
	}

	auto previous = generateCompl(i - 1);
	std::vector<Tet> unique;
	std::vector<std::vector<LocalGroup::type>> uniqueCode;
	std::vector<std::vector<LocalGroup::type>> uniqueComplementCode;

	int skipped = 0;
	int prev = 0;
	int k = 0;
	for (auto& p: previous) {
		if (!(++k % 100)) {
			std::cout << "n = " << i << ": " << (float) k / previous.size() << " with " << unique.size() - prev
					  << " new unique shapes\n";
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
					continue;
				}

				isLocalEqual = z;

				//check bounding box sizes
				if (!compareBounds(u, build)) {
					skipped++;
					continue;
				}

				//do the same for the complements
				if (!compareLocalEncodings(uComplementCode, buildComplementCode)) {
					skipped++;
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

	std::cout << "n = " << i << " full comparisons skipped: " << skipped << std::endl;
	std::cout << "n = " << i << ": " << unique.size() << " unique shapes\n" << std::endl;

	return unique;

}

std::vector<Tet> generate(unsigned int i) {
	if (i <= 1) {
		return std::vector{Tet(i, std::vector<Pos>(1, {0, 0, 0}))};
	}

	auto previous = generateCompl(i - 1);
	std::vector<Tet> unique;
	std::vector<std::vector<LocalGroup::type>> uniqueCode;
	std::vector<std::vector<LocalGroup::type>> uniqueComplementCode;

	int skipped = 0;
	int prev = 0;
	int k = 0;
	for (auto& p: previous) {
		if (!(++k % 100)) {
			std::cout << "n = " << i << ": " << (float) k / previous.size() << " with " << unique.size() - prev
					  << " new unique shapes\n";
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
					continue;
				}

				isLocalEqual = z;

				//check bounding box sizes
				if (!compareBounds(u, build)) {
					skipped++;
					continue;
				}

				//if all groups match, spin
				//if same at some rotation, set false, break
				if (fullCompare(u, build)) {
					newShape = false;
					break;
				}
				if (!compareLocalEncodings(uComplementCode, buildComplementCode)) {
					auto x = 0;
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

	std::cout << "n = " << i << " full comparisons skipped: " << skipped << std::endl;
	std::cout << "n = " << i << ": " << unique.size() << " unique shapes\n" << std::endl;

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
