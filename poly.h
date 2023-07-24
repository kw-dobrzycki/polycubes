//
// Created by croissant on 24/07/2023.
//

#ifndef TETRIS_POLY_H
#define TETRIS_POLY_H

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



#endif //TETRIS_POLY_H
