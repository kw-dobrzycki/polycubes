//
// Created by croissant on 24/07/2023.
//

#ifndef TETRIS_POLY_H
#define TETRIS_POLY_H

#include "Tet.h"
#include <vector>
#include <set>
#include <array>
#include <algorithm>

extern const unsigned* const selfGroupOf;
extern const unsigned* const localGroupOf;

template<class T>
T ceildiv(T a, T b) {
	return ((a + b - 1) / b);
}

template<class T>
struct BasicEncoding {
	std::vector<T> encoding;
	unsigned n;

	struct Index {
		size_t i, j;
	};

	static Index indexOf(const Pos& p, unsigned n) {
		size_t ints = ceildiv((size_t) n * n * n, sizeof(T) * 8);
		//x, z, y
		size_t index = p.y * n * n + p.z * n + p.x;
		return {index / (sizeof(T) * 8), index % (sizeof(T) * 8)};
	}

	explicit BasicEncoding(unsigned n)
			: n(n) {
		for (unsigned i = 0; i < ceildiv((size_t) n * n * n, sizeof(T) * 8); ++i) {
			encoding.push_back(0);
		}
	};

	BasicEncoding(const Pos* tet, unsigned n)
			: BasicEncoding(n) {
		for (int i = 0; i < n; ++i) {
			set(tet[i]);
		}
	}

	bool operator<(const BasicEncoding& other) const {
		for (int i = 0; i < encoding.size; ++i) {
			if (encoding[i] < other.encoding[i]) return true;
		}
		return false;
	}

	bool operator>(const BasicEncoding& other) const {
		for (int i = 0; i < encoding.size(); ++i) {
			if (encoding[i] > other.encoding[i]) return true;
		}
		return false;
	}

	bool operator==(const BasicEncoding& other) const {
		return encoding == other.encoding;
	}

	void set(const Pos& p) {
		Index i = indexOf(p, n);
		encoding[i.i] |= 0b1 << i.j;
	}

	void unset(const Pos& p) {
		Index i = indexOf(p, n);
		encoding[i.i] &= ~(0b1 << i.j);
	}
};

struct Bound {
	Pos min, max;

	operator Pos*() {
		return &min;
	}
};

Bound find_bound(const Pos* tet, unsigned n) {
	Bound bound{*tet, *tet};
	//find the bound
	for (int i = 0; i < n; ++i) {
		bound.min.x = std::min(bound.min.x, tet[i].x);
		bound.min.y = std::min(bound.min.y, tet[i].y);
		bound.min.z = std::min(bound.min.z, tet[i].z);
		bound.max.x = std::max(bound.max.x, tet[i].x);
		bound.max.y = std::max(bound.max.y, tet[i].y);
		bound.max.z = std::max(bound.max.z, tet[i].z);
	}
	return bound;
}

//returns E(A) > E(B)
bool orient_compare(const Pos* tetA,
					const Pos* tetB,
					unsigned n) {

	Bound boundA(find_bound(tetA, n));
	Bound boundB(find_bound(tetB, n));

	Pos rangeA = boundA.max - boundA.min;
	Pos rangeB = boundB.max - boundB.min;

	if (rangeA.y > rangeB.y) return true;
	if (rangeA.y < rangeB.y) return false;
	if (rangeA.z > rangeB.z) return true;
	if (rangeA.z < rangeB.z) return false;
	if (rangeA.x > rangeB.x) return true;
	if (rangeA.x < rangeB.x) return false;

	std::vector<Pos> sortedA(n);
	std::vector<Pos> sortedB(n);
	for (int i = 0; i < n; ++i) {
		sortedA[i] = tetA[i] - boundA.min;
		sortedB[i] = tetB[i] - boundB.min;
	}

	auto flatten = [i = n](const Pos& p) {
		return p.y * i * i + p.z * i + p.x;
	};

	auto compare = [&](const Pos& a, const Pos& b) {
		return flatten(a) < flatten(b);
	};

	std::sort(sortedA.begin(), sortedA.end(), compare);
	std::sort(sortedB.begin(), sortedB.end(), compare);

	for (int i = 0; i < n; ++i) {
		auto f1 = flatten(sortedA[i]);
		auto f2 = flatten(sortedB[i]);
		if (f1 > f2) return true;
		if (f1 < f2) return false;
	}

	return false;
}

void orient(Pos* tet, unsigned n) {
	//rotate the tet 24 times and keep a current max
	//at each rotation, compare the bounds to the current max (y > z > x).
	//if bounds are equal, translate to the unit with min x,y,z to put into positive octant
	//then sort all units within both tets
	//iterate and compare each unit's x,y,z.

	Pos* max = new Pos[n]{};
	memcpy(max, tet, sizeof(Pos) * n);

	auto spinY = [&]() {
		rotY(tet, n);
		if (orient_compare(tet, max, n)) {
			memcpy(max, tet, sizeof(Pos) * n);
		}
	};

	//top on top
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//back on top
	rotX(tet, n);
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//left on top
	rotZ(tet, n);
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//front on top
	rotZ(tet, n);
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//right on top
	rotZ(tet, n);
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//bottom on top
	rotX(tet, n);
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	Bound maxBound = find_bound(max, n);

	translate(max, n, ~maxBound.min); //ensure that the final result is in positive octant
	memcpy(tet, max, sizeof(Pos) * n);
	delete[] max;
}

#endif //TETRIS_POLY_H
