//
// Created by croissant on 24/07/2023.
//

#ifndef TETRIS_RELATIVE_H
#define TETRIS_RELATIVE_H

#include "poly.h"
#include "Tet.h"
#include <vector>
#include <set>
#include <array>

extern const Pos offsets[6];

std::vector<Tet> generate(unsigned int i);

void write(const std::vector<Tet>& v);

std::vector<Tet> read(std::string_view path);


#endif //TETRIS_RELATIVE_H
