//
// Created by croissant on 24/07/2023.
//

#ifndef TETRIS_POLY_H
#define TETRIS_POLY_H

#include <cstdint>

struct Pos {
	using type = int8_t;
	using boundType = uint8_t;
	static constexpr unsigned width = 6;
	type x : width;
	type y : width;
	type z : width;

	Pos operator+(const Pos& v) const {
		return {static_cast<type>(x + v.x), static_cast<type>(y + v.y), static_cast<type>(z + v.z)};
	}

	Pos operator-(const Pos& v) const {
		return {static_cast<type>(x - v.x), static_cast<type>(y - v.y), static_cast<type>(z - v.z)};
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


#endif //TETRIS_POLY_H
