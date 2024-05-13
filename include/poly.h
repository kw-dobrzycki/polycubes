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
#include <cstdint>

using _encoding_type = std::uint32_t; //todo 64 broken. big bug somewhere

template<class T>
constexpr T ceildiv(T a, T b) {
	return ((a + b - 1) / b);
}

template<class T, unsigned n>
struct BasicEncoding {

	static constexpr size_t size{ceildiv((size_t) n * n * n, sizeof(T) * 8)};
	T encoding[size]{};

	struct Index {
		size_t i, j;
	};

	static Index indexOf(const Pos& p) {
		size_t index = p.y * n * n + p.z * n + p.x;
		return {index / (sizeof(T) * 8), index % (sizeof(T) * 8)};
	}

	BasicEncoding(const Tet<n>& tet) {
		for (int i = 0; i < n; ++i) {
			set(tet.units[i]);
		}
	}

	BasicEncoding() = default;

	bool operator<(const BasicEncoding& other) const {
//		if (size < other.size) return true;
//		if (size > other.size) return false;
		for (int i = size - 1; i >= 0; --i) {
			if (encoding[i] < other.encoding[i]) return true;
			if (encoding[i] > other.encoding[i]) return false;
		}
		return false;
	}

	bool operator>(const BasicEncoding& other) const {
//		if (size > other.size) return true;
//		if (size < other.size) return false;
		for (int i = size - 1; i >= 0; --i) {
			if (encoding[i] > other.encoding[i]) return true;
			if (encoding[i] < other.encoding[i]) return false;
		}
		return false;
	}

	bool operator==(const BasicEncoding& other) const {
		for (int i = 0; i < size; ++i) {
			if (encoding[i] != other.encoding[i]) return false;
		}
		return true;
	}

	void encode(const Tet<n>& tet) {
		for (int i = 0; i < n; ++i) {
			set(tet.units[i]);
		}
	}

	void set(const Pos& p) {
		Index i = indexOf(p);
		encoding[i.i] |= 0b1 << i.j;
	}

	void unset(const Pos& p) {
		Index i = indexOf(p);
		encoding[i.i] &= ~(0b1 << i.j);
	}

	void reset() {
		memset(encoding, 0, sizeof(T) * size);
	}
};

template<unsigned n>
using EncodeType = BasicEncoding<_encoding_type, n>;

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

Pos findMax(const Pos* tet, unsigned n) {
	Pos corner{*tet};
	//find the bound
	for (int i = 0; i < n; ++i) {
		corner.x = std::max(corner.x, tet[i].x);
		corner.y = std::max(corner.y, tet[i].y);
		corner.z = std::max(corner.z, tet[i].z);
	}
	return corner;
}

template<unsigned n>
void fixTet(Tet<n>& tet) {
	Pos min = findMin(tet.units, n);
	transSub(tet.units, n, min);
}

template<unsigned n>
bool volumeCompare(Tet<n>& A, Tet<n>& B) {
	//returns A is definitely > B
	Pos topA = findMax(A.units, n) - findMin(A.units, n);
	Pos topB = findMax(B.units, n) - findMin(B.units, n);
	auto sort = [](Pos& p) {
		if (p.x > p.y) {
			auto t = p.y;
			p.y = p.x;
			p.x = t;
		}
		if (p.z > p.y) {
			auto t = p.y;
			p.y = p.z;
			p.z = t;
		}
		if (p.x > p.z) {
			auto t = p.z;
			p.z = p.x;
			p.x = t;
		}
	};

	auto max = [](Pos& p) {
		return p.x > p.y ?
			   p.x > p.z ? p.x : p.z
						 : p.z > p.y ? p.z : p.y;
	};
//	sort(topA);
//	sort(topB);

	auto mA = max(topA);
	auto mB = max(topB);
	if (mA > mB) return true;
	if (mA < mB) return false;
	return false;
}

template<unsigned n>
bool equalVolume(const Tet<n>& A, const Tet<n>& B) {
	Pos topA = findMax(A.units, n) - findMin(A.units, n);
	Pos topB = findMax(B.units, n) - findMin(B.units, n);
	auto sort = [](Pos& p) {
		if (p.x > p.y) {
			auto t = p.y;
			p.y = p.x;
			p.x = t;
		}
		if (p.z > p.y) {
			auto t = p.y;
			p.y = p.z;
			p.z = t;
		}
		if (p.x > p.z) {
			auto t = p.z;
			p.z = p.x;
			p.x = t;
		}
	};

	auto max = [](Pos& p) {
		return p.x > p.y ?
			   p.x > p.z ? p.x : p.z
						 : p.z > p.y ? p.z : p.y;
	};
	sort(topA);
	sort(topB);
	return topA == topB;
}

template<unsigned n>
void orient(Tet<n>& tet) {
	//rotate the tet 24 times and keep a current max
	//at each rotation, compare the bounds to the current max (y > z > x).
	//if bounds are equal, translate to the unit with min x,y,z to put into positive octant
	//then sort all units within both tets
	//iterate and compare each unit's x,y,z.

	Tet<n> max = tet;
	fixTet(max);
	EncodeType<n> maxEncoding(max);
	EncodeType<n> tetEncoding;

	auto spinY = [&]() {
		rotY(tet.units, n);
		fixTet(tet);
		tetEncoding.reset();
		tetEncoding.encode(tet);
		if (tetEncoding > maxEncoding) {
			max = tet;
			maxEncoding = tetEncoding;
		}
	};

	Pos bound = findMax(max.units, n) - findMin(max.units, n);

	auto rbx = [](Pos& p) {
		rotX(&p, 1);
		p.z *= -1;
	};

	auto rby = [](Pos& p) {
		rotY(&p, 1);
		p.x *= -1;
	};

	auto rbz = [](Pos& p) {
		rotZ(&p, 1);
		p.y *= -1;
	};

	//top on top
	if (bound.y >= bound.x && bound.y >= bound.z)
		for (int j = 0; j < 4; ++j) {
			spinY();
		}

	//back on top
	rotX(tet.units, n);
	rbx(bound);
	if (bound.y >= bound.x && bound.y >= bound.z)
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//left on top
	rotZ(tet.units, n);
	rbz(bound);
	if (bound.y >= bound.x && bound.y >= bound.z)
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//front on top
	rotZ(tet.units, n);
	rbz(bound);
	if (bound.y >= bound.x && bound.y >= bound.z)
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//right on top
	rotZ(tet.units, n);
	rbz(bound);
	if (bound.y >= bound.x && bound.y >= bound.z)
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	//bottom on top
	rotZ(tet.units, n);
	rotX(tet.units, n);
//	rbz(bound);
//	rbx(bound);
//	if (bound.y >= bound.x && bound.y >= bound.z)
	for (int j = 0; j < 4; ++j) {
		spinY();
	}

	tet = max;
}

#endif //TETRIS_POLY_H
