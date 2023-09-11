//
// Created by Chris on 12/08/2023.
//

#ifndef TETRIS_TET_H
#define TETRIS_TET_H

#include "groups.h"
#include <vector>
#include <array>
#include "poly.h"

struct Tet {

	unsigned int n;
	std::vector<Pos> coords;

	Tet() : n(0), coords({}){};

	Tet(unsigned int n, const std::vector<Pos>& coords);

	Tet& rotX(unsigned i = 1);

	Tet& rotY(unsigned i = 1);

	Tet& rotZ(unsigned i = 1);

	std::vector<Pos> getFreeSpaces() const;

	Tet insert(const Pos& block) const;

	std::vector<uint64_t> fullEncode() const;

	std::vector<uint64_t> volumeEncode() const;

	std::array<Pos::type, 6> getBounds() const;
};


#endif //TETRIS_TET_H
