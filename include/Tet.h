//
// Created by Chris on 12/08/2023.
//

#ifndef TETRIS_TET_H
#define TETRIS_TET_H

#include "groups.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>
#include <array>
#include <set>
#include <vector>
#include "poly.h"

struct Tet {

	unsigned int n;

	std::vector<uint32_t> pieces;
	std::vector<Pos> coords;
	std::vector<int> neighbours;
	std::vector<uint8_t> population;

	Tet(unsigned int n, const std::vector<Pos>& coordinates);

	/**
	 * DOES NOT MAINTAIN CONSISTENCY
	 */
	Tet(unsigned int n, const std::vector<uint32_t>& pieces, const std::vector<Pos>& coords,
		const std::vector<int>& neighbours, const std::vector<uint8_t>& population);

	Tet& rotX(unsigned i = 1);

	Tet& rotY(unsigned i = 1);

	Tet& rotZ(unsigned i = 1);

	std::vector<Pos> getFreeSpaces() const;

	Tet insert(const Pos& block) const;

	std::vector<unsigned> encodeLocal() const;

	std::vector<unsigned> encodeSelf() const;

	std::array<uint64_t, 64> boundEncode() const;

	void print() const;

	std::array<int, 6> getBounds() const;

	Tet getComplement() const;
};


#endif //TETRIS_TET_H
