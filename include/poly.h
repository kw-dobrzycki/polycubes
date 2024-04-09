//
// Created by croissant on 24/07/2023.
//

#ifndef TETRIS_POLY_H
#define TETRIS_POLY_H

#include "Tet.h"
#include <vector>
#include <set>
#include <array>
#include <cassert>

template<class T>
T ceildiv(T a, T b) {
	return ((a + b - 1) / b);
}

template<class T>
struct BasicEncoding {

	unsigned n;
	size_t size;
	T* encoding;

	struct Index {
		size_t i, j;
	};

	static Index indexOf(const Pos& p, unsigned n) {
		size_t index = p.y * n * n + p.z * n + p.x;
		return {index / (sizeof(T) * 8), index % (sizeof(T) * 8)};
	}

	explicit BasicEncoding(unsigned n)
			: n(n),
			  size(ceildiv((size_t) n * n * n, sizeof(T) * 8)),
			  encoding(new T[size]{}) {
	}

	BasicEncoding(const Pos* tet, unsigned n)
			: BasicEncoding(n) {
		for (int i = 0; i < n; ++i) {
			set(tet[i]);
		}
	}

	BasicEncoding(const BasicEncoding& other)
			: BasicEncoding(other.n) {
		memcpy(encoding, other.encoding, size * sizeof(T));
	}

	~BasicEncoding() {
		delete[] encoding;
	}

	bool operator<(const BasicEncoding& other) const {
		assert(size == other.size);
		for (int i = size - 1; i >= 0; --i) {
			if (encoding[i] < other.encoding[i]) return true;
			if (encoding[i] > other.encoding[i]) return false;
		}
		return false;
	}

	bool operator>(const BasicEncoding& other) const {
		assert(size == other.size);
		for (int i = size - 1; i >= 0; --i) {
			if (encoding[i] > other.encoding[i]) return true;
			if (encoding[i] < other.encoding[i]) return false;
		}
		return false;
	}

	bool operator==(const BasicEncoding& other) const {
		assert(size == other.size);
		return memcmp(encoding, other.encoding, sizeof(T) * size);
	}

	void operator()(const Pos* tet) {
		for (int i = 0; i < n; ++i) {
			set(tet[i]);
		}
	}

	void set(const Pos& p) {
		Index i = indexOf(p, n);
		encoding[i.i] |= 0b1 << i.j;
	}

	void unset(const Pos& p) {
		Index i = indexOf(p, n);
		encoding[i.i] &= ~(0b1 << i.j);
	}

	void reset() {
		for (int i = 0; i < size; ++i) {
			encoding[i] = (T) 0;
		}
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

template<unsigned n>
void fixTet(Tet<n>& tet) {
	Pos min = findMin(tet.units, n);
	transSub(tet.units, n, min);
}

//returns E(A) > E(B) = 1
template<unsigned n>
int compareFixed(Tet<n>& tetA, Tet<n>& tetB) {

	thread_local EncodeType e1(n);
	thread_local EncodeType e2(n);
	e1.reset();
	e2.reset();

	e1(tetA.units);
	e2(tetB.units);

	if (e1 > e2) return 1;
	if (e1 < e2) return -1;
	return 0;
}


template<unsigned n>
void orient(Tet<n>& tet) {
	//rotate the tet 24 times and keep a current max
	//at each rotation, compare the bounds to the current max (y > z > x).
	//if bounds are equal, translate to the unit with min x,y,z to put into positive octant
	//then sort all units within both tets
	//iterate and compare each unit's x,y,z.

	thread_local Tet<n> max{};
	thread_local EncodeType maxEncoding(n);
	thread_local EncodeType tetEncoding(n);
	max = tet;
	fixTet(max);
	maxEncoding(max.units);

	auto spinY = [&]() {
		rotY(tet.units, n);
		fixTet(tet);
		tetEncoding.reset();
		tetEncoding(tet.units);
		if (tetEncoding > maxEncoding) {
			max = tet;
			maxEncoding.reset();
			maxEncoding(max.units);
		}
	};

	//top on top
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//back on top
	rotX(tet.units, n);
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//left on top
	rotZ(tet.units, n);
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//front on top
	rotZ(tet.units, n);
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//right on top
	rotZ(tet.units, n);
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//bottom on top
	rotX(tet.units, n);
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	tet = max;

	tetEncoding.reset();
	maxEncoding.reset();
}

#endif //TETRIS_POLY_H
