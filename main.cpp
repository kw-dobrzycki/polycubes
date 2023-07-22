#include <iostream>
#include <set>
#include <vector>
#include <fstream>
#include "groupOf.h"
#include <cstring>

struct Pos {
	int x, y, z;

	Pos operator+(const Pos& v) const {
		return {x + v.x, y + v.y, z + v.z};
	}

	Pos& operator+=(const Pos& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	bool operator==(const Pos& v) const {
		return x == v.x && y == v.y && z == v.z;
	}
};

int opposite[]{
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

const std::set<int> groups[]{
		{0},
		{1,  16, 8,  4,  32, 2},
		{9,  12, 18, 40, 6,  5,  24, 17, 34, 3,  36, 48},
		{44, 7,  19, 56, 38, 50, 13, 25},
		{10, 33, 20},
		{41, 42, 14, 21, 11, 52, 28, 49, 22, 35, 26, 37},
		{60, 27, 58, 39, 54, 57, 23, 46, 45, 51, 15, 29},
		{53, 30, 43},
		{59, 31, 47, 55, 62, 61},
		{63}
};

const int next[]{0, 16, 1, 36, 32, 24, 5, 19, 4, 12, 33, 52, 18, 25, 21, 29, 8, 34, 40, 56, 10, 11, 35, 46, 17, 44, 37,
				 58, 49, 60, 43, 47, 2, 20, 3, 26, 48, 41, 50, 54, 6, 42, 14, 53, 7, 51, 45, 55, 9, 22, 13, 15, 28, 30,
				 57, 62, 38, 23, 39, 31, 27, 59, 61, 63};

const int lengths[]{1, 6, 12, 8, 3, 12, 12, 3, 6, 1};

const int groupOfSelf[]{0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 4, 5, 2, 3, 5, 6, 1, 2, 2, 3, 4, 5, 5, 6, 2, 3, 5, 6, 5, 6, 7, 8,
						1, 4, 2, 5, 2, 5, 3, 6, 2, 5, 5, 7, 3, 6, 6, 8, 2, 5, 3, 6, 5, 7, 6, 8, 3, 6, 6, 8, 6, 8, 8, 9};

struct Tet {

	unsigned int n;

	/**
	 * Connectivity encodings of each block - 6 ints per block
	 */
	std::vector<int> pieces;

	/**
	 * Coordinates of each block. THIS FIELD IS NOT MAINTAINED FOR EXTENDED FUNCTIONALITY.
	 */
	std::vector<Pos> coords;

	std::vector<int> neighbours;

	Tet(unsigned int n, const std::vector<Pos>& coordinates)
			: n(n),
			  pieces(6 * n),
			  coords(coordinates),
			  neighbours(6 * n) {

		//decode connectivity (name the pieces for now)
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < 6; ++j) {
				auto offset = coordinates[i] + offsets[j];
				//go through each face and check for a block at that coordinate
				for (int k = 0; k < n; ++k) {
					if (offset == coordinates[k]) {
						pieces[i * 6 + j] = k + 1; //names start from 1
						neighbours[i * 6 + j] = k + 1; //just like names but these don't change
					}
				}
			}
		}

		//decode neighbour types
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < 6; ++j) {
				int neighbour = i * 6 + j;
				if (pieces[neighbour]) {
					int neighbourName = pieces[neighbour] - 1;
					//calculate the group of the neighbour
					int sum = 0;
					for (int k = 0; k < 6; ++k) {
						sum += (pieces[neighbourName * 6 + k] > 0) << (5 - k);
					}
					pieces[neighbour] = groupOfSelf[sum];
				}
			}
		}
	}

	/**
	 * DO NOT USE THIS FOR EXPERIMENTATION
	 */
	Tet(unsigned int n, const std::vector<int>& pieces, const std::vector<Pos>& coords,
		const std::vector<int>& neighbours)
			: n(n),
			  pieces(pieces),
			  coords(coords),
			  neighbours(neighbours) {}

	/**
	 * THIS FUNCTION DOES NOT UPDATE COORDINATES.
	 */
	Tet& rot(unsigned int i, const int* shift) {
		for (int j = 0; j < i; ++j) {
			for (int k = 0; k < n; ++k) {
				int tmp = pieces[6 * k + shift[3]];
				pieces[6 * k + shift[3]] = pieces[6 * k + shift[2]];
				pieces[6 * k + shift[2]] = pieces[6 * k + shift[1]];
				pieces[6 * k + shift[1]] = pieces[6 * k + shift[0]];
				pieces[6 * k + shift[0]] = tmp;
			}
		}
		return *this;
	}

	/**
	 * THIS FUNCTION DOES NOT UPDATE COORDINATES.
	 */
	Tet& rotX(unsigned int i) {
		int shift[]{0, 1, 5, 3};
		return rot(i, shift);
	}

	/**
	 * THIS FUNCTION DOES NOT UPDATE COORDINATES.
	 */
	Tet& rotY(unsigned int i) {
		int shift[]{1, 2, 3, 4};
		return rot(i, shift);
	}

	/**
	 * THIS FUNCTION DOES NOT UPDATE COORDINATES.
	 */
	Tet& rotZ(unsigned int i) {
		int shift[]{0, 2, 5, 4};
		return rot(i, shift);
	}

	std::vector<Pos> getFreeSpaces() const {
		std::set<int> unique;
		std::vector<Pos> spaces;

		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < 6; ++j) {
				if (!pieces[i * 6 + j]) {
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

	Tet insert(const Pos& block) const {
		auto c_pieces = pieces;
		auto c_coords = coords;
		auto c_neighbours = neighbours;

		c_pieces.push_back(0);
		c_pieces.push_back(0);
		c_pieces.push_back(0);
		c_pieces.push_back(0);
		c_pieces.push_back(0);
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


		//change 0 bits of direct neighbours to the self group of new block.
		for (int i = 0; i < 6; ++i) {
			int dirNeighbour = c_neighbours[n * 6 + i];
			if (dirNeighbour) {
				c_pieces[(dirNeighbour - 1) * 6 + opposite[i]] = groupOfSelf[selfType];

				//compute new self type of the neighbour for indirect neighbours to update (this includes block)
				int neighbourType = 0;
				for (int k = 0; k < 6; ++k) {
					neighbourType += (c_neighbours[(dirNeighbour - 1) * 6 + k] > 0) << (5 - k);
				}


				//change non-0 bits of indirect neighbours
				for (int j = 0; j < 6; ++j) {
					int indirNeighbour = c_neighbours[(dirNeighbour - 1) * 6 + j];
					if (indirNeighbour) {
						c_pieces[(indirNeighbour - 1) * 6 + opposite[j]] = groupOfSelf[neighbourType];
					}
				}
			}
		}

		return Tet{n + 1, c_pieces, c_coords, c_neighbours};
	}

	int shortestGroup() const {
		int i = 2;
		for (int j = 0; j < n; ++j) {
			int x = 0;
			for (int k = 0; j < 6; ++j) {
				x += (pieces[j * 6 + k] > 0) << (5 - k);
			}
			int group = groupOfSelf[x];
			i = std::min(lengths[i], lengths[group]);
		}
		return i;
	}

	std::vector<int> groupEncode() const {
		std::vector<int> code(n);
		for (int i = 0; i < n; ++i) {
			int x = pieces[i * 6 + 0] * 100000 +
					pieces[i * 6 + 1] * 10000 +
					pieces[i * 6 + 2] * 1000 +
					pieces[i * 6 + 3] * 100 +
					pieces[i * 6 + 4] * 10 +
					pieces[i * 6 + 5];

			code[i] = groupOfNeighbour[x];
		}
		return code;
	}

	std::vector<int> encode() const {
		std::vector<int> code(n);
		for (int i = 0; i < n; ++i) {
			int x = pieces[i * 6 + 0] * 100000 +
					pieces[i * 6 + 1] * 10000 +
					pieces[i * 6 + 2] * 1000 +
					pieces[i * 6 + 3] * 100 +
					pieces[i * 6 + 4] * 10 +
					pieces[i * 6 + 5];

			code[i] = x;
		}
		return code;
	}

	void print() const {
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < 6; ++j) {
				std::cout << pieces[i * 6 + j];
			}
			std::cout << "|";
		}
		std::cout << std::endl;
	}
};

inline bool compareGroupEncodings(const std::vector<int>& A, const std::vector<int>& B) {
	thread_local int* counters = new int[43450]();
	thread_local int* indices = new int[50](); //note this implementation supports maximum 25 blocks
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

inline bool compareEncodings(const std::vector<int>& A, const std::vector<int>& B) {
	thread_local int* counters = new int[1000000]();
	thread_local int* indices = new int[50](); //note this implementation supports maximum 25 blocks
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


bool spinCompare(Tet& a, Tet& b) {
	auto Acode = a.encode();
	//spin on top
	for (int i = 0; i < 4; ++i) {
		b.rotY(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on back
	b.rotX(1);
	for (int i = 0; i < 4; ++i) {
		b.rotZ(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on right
	b.rotY(1);
	for (int i = 0; i < 4; ++i) {
		b.rotX(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on front
	b.rotY(1);
	for (int i = 0; i < 4; ++i) {
		b.rotZ(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on left
	b.rotY(1);
	for (int i = 0; i < 4; ++i) {
		b.rotX(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on bottom
	b.rotY(1);
	b.rotX(1);
	for (int i = 0; i < 4; ++i) {
		b.rotY(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	return false;
}

std::vector<Tet> generate(unsigned int i) {
	if (i <= 1) {
		return std::vector{Tet(i, std::vector<Pos>(1, {0, 0, 0}))};
	}

	auto previous = generate(i - 1);
	std::vector<Tet> unique;

	for (auto& p: previous) {
		auto faces = p.getFreeSpaces();

		for (auto& f: faces) {
			Tet build(p.insert(f));
			auto buildCode = build.groupEncode();
			bool newShape = true;

			for (auto& u: unique) {
				auto uCode = u.groupEncode();
				//if there's a new group, keep true, check next u
				if (!compareGroupEncodings(uCode, buildCode))
					continue;

				//if all groups match, spin
				//if same at some rotation, set false, break
				if (spinCompare(u, build)) {
					newShape = false;
					break;
				}
			}

			if (newShape)
				unique.push_back(build);
		}
	}

	return unique;
}

void write(const std::vector<Tet>& v) {
	std::cout << "saving " << v.size() << " results..." << std::endl;
	std::ofstream file;
	file.open("./blocks.txt");
	for (auto& t: v) {
		for (auto& k: t.coords) {
			file << k.x << " " << k.y << " " << k.z << std::endl;
		}
		file << std::endl;
	}
	file.close();
}

int main() {

	write(generate(4));

//#define testing
#ifdef testing

	Tet a{4, {
			{0, 0, 0},
			{0, 1, 0},
			{0, -1, 0},
			{1, 0, 0},
	}};

//#define positive
#define test

#ifdef positive

	Tet b{a};
	b.rotY(1);

#else

	Tet b{4, {
			{0, 0, 0},
			{0, 1, 0},
			{0, -1, 0},
			{1, 1, 0}
	}};

#endif

	auto A = a.groupEncode();
	auto B = b.groupEncode();

	int j = 0;

	for (int i = 0; i < 1000000; ++i) {
#ifdef test
		if (compareGroupEncodings(A, B))
#endif
		j += spinCompare(a, b);
	}

	std::cout << j;
#endif

	return 0;
};
