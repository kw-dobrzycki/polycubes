//
// Created by croissant on 24/07/2023.
//

#ifndef TETRIS_GROUPS_H
#define TETRIS_GROUPS_H

#include <bitset>
#include <vector>
#include <iostream>
#include <fstream>
#include "poly.h"
#include "relative.h"

namespace SelfGroup {

	using type = uint8_t;

	uint8_t X(uint8_t b, unsigned i = 1);

	uint8_t Y(uint8_t b, unsigned i = 1);

	uint8_t Z(uint8_t b, unsigned i = 1);

	unsigned* generateSelfGroups();
}

namespace LocalGroup {

	using type = uint32_t;

	unsigned toDen(type b);

	type toBCD(unsigned d);

	type X(type b, unsigned i = 1);

	type Y(type b, unsigned i = 1);

	type Z(type b, unsigned i = 1);

	unsigned* generateLocalGroups();
}

#endif //TETRIS_GROUPS_H
