//
// Created by croissant on 24/07/2023.
//

#ifndef TETRIS_GROUPGENERATORS_H
#define TETRIS_GROUPGENERATORS_H

#include <bitset>
#include <vector>
#include <iostream>
#include <fstream>
#include "poly.h"
#include "relative.h"

namespace SelfGeneratorUI8 {

	inline uint8_t X(uint8_t b, unsigned i = 1);

	inline uint8_t Y(uint8_t b, unsigned i = 1);

	inline uint8_t Z(uint8_t b, unsigned i = 1);

	std::map<uint32_t, uint8_t> generateSelfGroups();
}

namespace LocalGeneratorUI32 {

	using type = uint32_t;

	unsigned toDen(type b);

	type toBCD(unsigned d);

	inline type X(type b, unsigned i = 1);

	inline type Y(type b, unsigned i = 1);

	inline type Z(type b, unsigned i = 1);

	std::map<uint32_t, uint16_t> generateLocalGroups();
}

#endif //TETRIS_GROUPGENERATORS_H
