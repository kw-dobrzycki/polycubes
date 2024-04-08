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

template<class T>
T ceildiv(T a, T b) {
	return ((a + b - 1) / b);
}

template<class T>
struct BasicEncoding {

	unsigned n;
	std::vector<T> encoding;

	struct Index {
		size_t i, j;
	};

	static Index indexOf(const Pos& p, unsigned n) {
		size_t index = p.y * n * n + p.z * n + p.x;
		return {index / (sizeof(T) * 8), index % (sizeof(T) * 8)};
	}

	explicit BasicEncoding(unsigned n)
			: n(n), encoding((n * n * n + (sizeof(T) * 8 - 1)) / (sizeof(T) * 8)) {
	}

	BasicEncoding(const Pos* tet, unsigned n)
			: BasicEncoding(n) {
		for (int i = 0; i < n; ++i) {
			set(tet[i]);
		}
	}

	bool operator<(const BasicEncoding& other) const {
		for (int i = encoding.size() - 1; i >= 0; --i) {
			if (encoding[i] < other.encoding[i]) return true;
			if (encoding[i] > other.encoding[i]) return false;
		}
		return false;
	}

	bool operator>(const BasicEncoding& other) const {
		for (int i = encoding.size() - 1; i >= 0; --i) {
			if (encoding[i] > other.encoding[i]) return true;
			if (encoding[i] < other.encoding[i]) return false;
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

using EncodeType = BasicEncoding<unsigned>;

Pos findMin(const Pos* tet, unsigned n) {
	Pos corner{*tet};
	//find the bound
	for (int i = 0; i < n; ++i) {
		corner.x = std::min(corner.x, tet[i].x);
		corner.y = std::min(corner.y, tet[i].y);
		corner.z = std::min(corner.z, tet[i].z);
	}
	return corner;
}

//returns E(A) > E(B) = 1
int compareTet(const Pos* tetA,
			   const Pos* tetB,
			   unsigned n) {

	Pos boundA = findMin(tetA, n);
	Pos boundB = findMin(tetB, n);

	std::vector<Pos> originA(n);
	std::vector<Pos> originB(n);
	for (int i = 0; i < n; ++i) {
		originA[i] = tetA[i] - boundA;
		originB[i] = tetB[i] - boundB;
	}

	EncodeType e1(originA.data(), n);
	EncodeType e2(originB.data(), n);
	if (e1 > e2) return 1;
	if (e1 < e2) return -1;
	return 0;

	auto flatten = [i = n](const Pos& p) {
		return p.y * i * i + p.z * i + p.x;
	};

	auto compare = [&](const Pos& a, const Pos& b) {
		return flatten(a) > flatten(b);
	};

	std::sort(originA.begin(), originA.end(), compare);
	std::sort(originB.begin(), originB.end(), compare);

	for (int i = 0; i < n; ++i) {
		auto f1 = flatten(originA[i]);
		auto f2 = flatten(originB[i]);
		if (f1 > f2) return 1;
		if (f1 < f2) return -1;
	}
	return 0;
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
		if (compareTet(tet, max, n) == 1) {
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

	Pos min = findMin(max, n);

	translate(max, n, ~min); //ensure that the final result is in positive octant
	memcpy(tet, max, sizeof(Pos) * n);
	delete[] max;
}

#endif //TETRIS_POLY_H
