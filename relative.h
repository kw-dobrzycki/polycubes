//
// Created by croissant on 24/07/2023.
//

#ifndef TETRIS_RELATIVE_H
#define TETRIS_RELATIVE_H

#include "poly.h"
#include <vector>
#include <set>
#include <map>

extern const std::map<uint32_t, uint8_t> selfGroupOf;
extern const std::map<uint32_t, uint16_t> localGroupOf;
extern unsigned opposite[6];
extern const Pos offsets[6];

struct Tet {

	unsigned int n;

	/**
	 * Connectivity encodings of each block - 6 ints per block
	 */
	std::vector<int> pieces;

	/**
	 * Coordinates of each block. THIS FIELD IS NOT MAINTAINED FOR EXTENDED FUNCTIONALITY.
	 */
	std::vector<Pos> coords;

	std::vector<int> neighbours;

	Tet(unsigned int n, const std::vector<Pos>& coordinates);

	/**
	 * DO NOT USE THIS FOR EXPERIMENTATION
	 */
	Tet(unsigned int n, const std::vector<int>& pieces, const std::vector<Pos>& coords,
		const std::vector<int>& neighbours);

	/**
	 * THIS FUNCTION DOES NOT UPDATE COORDINATES.
	 */
	Tet& rot(unsigned int i, const int* shift);

	/**
	 * THIS FUNCTION DOES NOT UPDATE COORDINATES.
	 */
	Tet& rotX(unsigned int i);

	/**
	 * THIS FUNCTION DOES NOT UPDATE COORDINATES.
	 */
	Tet& rotY(unsigned int i);

	/**
	 * THIS FUNCTION DOES NOT UPDATE COORDINATES.
	 */
	Tet& rotZ(unsigned int i);

	std::vector<Pos> getFreeSpaces() const;

	Tet insert(const Pos& block) const;

	std::vector<int> groupEncode() const;

	std::vector<int> encode() const;

	void print() const;
};

inline bool compareGroupEncodings(const std::vector<int>& A, const std::vector<int>& B);

inline bool compareEncodings(const std::vector<int>& A, const std::vector<int>& B);


bool spinCompare(Tet a, Tet b);

std::vector<Tet> generate(unsigned int i);

void write(const std::vector<Tet>& v);


#endif //TETRIS_RELATIVE_H
