#include <iostream>
#include <set>
#include <vector>
#include <fstream>

struct Vec {
	int x, y, z;

	Vec operator+(const Vec& v) const {
		return {x + v.x, y + v.y, z + v.z};
	}

	Vec& operator+=(const Vec& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	bool operator==(const Vec& v) const {
		return x == v.x && y == v.y && z == v.z;
	}
};

int opposite(int i) {
	switch (i % 6) {
		case 0:
			return 5;
		case 1:
			return 3;
		case 2:
			return 4;
		case 3:
			return 1;
		case 4:
			return 2;
		case 5:
			return 0;
	}
	return 0; // never happens
}

const Vec offsets[]{
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

const int groupOf[]{0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 4, 5, 2, 3, 5, 6, 1, 2, 2, 3, 4, 5, 5, 6, 2, 3, 5, 6, 5, 6, 7, 8, 1,
					4, 2, 5, 2, 5, 3, 6, 2, 5, 5, 7, 3, 6, 6, 8, 2, 5, 3, 6, 5, 7, 6, 8, 3, 6, 6, 8, 6, 8, 8, 9};

struct Tet {

	unsigned int n;

	/**
	 * Connectivity encodings of each block - 6 ints per block
	 */
	std::vector<int> pieces;

	/**
	 * Coordinates of each block. THIS FIELD IS NOT MAINTAINED FOR EXTENDED FUNCTIONALITY.
	 */
	std::vector<Vec> coords;

	Tet(unsigned int n, const std::vector<Vec>& coordinates) : n(n), pieces(6 * n), coords(coordinates) {
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < 6; ++j) {
				//go through each face and check for a block at that coordinate
				for (int k = 0; k < n; ++k) {
					if (coordinates[i] + offsets[j] == coordinates[k]) {
						pieces[i * 6 + j] = k + 1; //names start from 1
					}
				}
			}
		}
	}

	Tet(unsigned int n, const std::vector<int>& pieces, const std::vector<Vec>& coords)
			: n(n),
			  pieces(pieces),
			  coords(coords) {}

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

	std::vector<Vec> getFreeSpaces() const {
		std::set<int> unique;
		std::vector<Vec> spaces;

		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < 6; ++j) {
				if (!pieces[i * 6 + j]) {
					Vec space = coords[i] + offsets[j];
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

	Tet insert(const Vec& block) const {
		auto c_pieces = pieces;
		auto c_coords = coords;

		c_pieces.push_back(0);
		c_pieces.push_back(0);
		c_pieces.push_back(0);
		c_pieces.push_back(0);
		c_pieces.push_back(0);
		c_pieces.push_back(0);
		c_coords.push_back(block);

		for (int i = 0; i < 6; ++i) {
			for (int j = 0; j < n; ++j) {
				if (coords[j] == block + offsets[i]) {
					c_pieces[j * 6 + opposite(i)] = n + 1;
					c_pieces[n * 6 + i] = j + 1;
				}
			}
		}

		return Tet{n + 1, c_pieces, c_coords};
	}

	int shortestGroup() const {
		int i = 2;
		for (int j = 0; j < n; ++j) {
			int x = 0;
			for (int k = 0; j < 6; ++j) {
				x += (pieces[j * 6 + k] > 0) * (1 << (5 - k));
			}
			int group = groupOf[x];
			i = std::min(lengths[i], lengths[group]);
		}
		return i;
	}

	std::vector<int> groupEncode() const {
		std::vector<int> code(n);
		for (int i = 0; i < n; ++i) {
			int x = 0;
			for (int j = 0; j < 6; ++j) {
				x += (pieces[i * 6 + j] > 0) * (1 << (5 - j));
			}
			code[i] = groupOf[x];
		}
		return code;
	}

	std::vector<int> encode() const {
		std::vector<int> code(n);
		for (int i = 0; i < n; ++i) {
			int x = 0;
			for (int j = 0; j < 6; ++j) {
				x += (pieces[i * 6 + j] > 0) * (1 << (5 - j));
			}
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

inline bool groupCompareV(const std::vector<int>& A, const std::vector<int>& B) {
	int counters[10]{};

	for (int i = 0; i < A.size(); ++i) {
		counters[A[i]]++;
		counters[B[i]]--;
	}

	int sum = 0;
	for (int i = 0; i < A.size(); ++i) {
		sum += counters[i] * counters[i];
	}

	return sum == 0;
}


bool spinCompare(Tet& a, Tet& b) {
	auto Acode = a.groupEncode();
	//spin on top
	for (int i = 0; i < 4; ++i) {
		b.rotY(1);
		auto Bcode = b.groupEncode();
		if (groupCompareV(Acode, Bcode))
			return true;
	}

	//spin on back
	b.rotX(1);
	for (int i = 0; i < 4; ++i) {
		b.rotZ(1);
		auto Bcode = b.groupEncode();
		if (groupCompareV(Acode, Bcode))
			return true;
	}

	//spin on right
	b.rotY(1);
	for (int i = 0; i < 4; ++i) {
		b.rotX(1);
		auto Bcode = b.groupEncode();
		if (groupCompareV(Acode, Bcode))
			return true;
	}

	//spin on front
	b.rotY(1);
	for (int i = 0; i < 4; ++i) {
		b.rotZ(1);
		auto Bcode = b.groupEncode();
		if (groupCompareV(Acode, Bcode))
			return true;
	}

	//spin on left
	b.rotY(1);
	for (int i = 0; i < 4; ++i) {
		b.rotX(1);
		auto Bcode = b.groupEncode();
		if (groupCompareV(Acode, Bcode))
			return true;
	}

	//spin on bottom
	b.rotY(1);
	b.rotX(1);
	for (int i = 0; i < 4; ++i) {
		b.rotY(1);
		auto Bcode = b.groupEncode();
		if (groupCompareV(Acode, Bcode))
			return true;
	}

	return false;
}

std::vector<Tet> generate(unsigned int i) {
	if (i <= 1) {
		return std::vector{Tet(i, std::vector<Vec>(1, {0, 0, 0}))};
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
				//if there's a new group, keep true, check next u
				if (!groupCompareV(u.groupEncode(), buildCode))
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

	/*
	write(generate(5));

	Tet A{6, {
			{0, 0, 0},
			{1, 0, 0},
			{2, 0, 0},
			{2, 1, 0},
			{2, 2, 0},
			{2, -1, 0}
	}};

	Tet B{6, {
			{0, 0, 0},
			{0, 1, 0},
			{0, -1, 0},
			{1, 0, 0},
			{2, 0, 0},
			{3, 0, 0}
	}};

	std::cout << shiftCompare(A, B);
	return 0;
	*/

	Tet a{4, {
			{0, 0, 0},
			{0, 1, 0},
			{0, -1, 0},
			{1, 0, 0},
	}};

#define positive
//#define test

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
		if (groupCompareV(A, B))
#endif
		j += spinCompare(a, b);
	}

	std::cout << j;

	return 0;
}
