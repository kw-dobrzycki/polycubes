//
// Created by croissant on 24/07/2023.
//

#ifndef TETRIS_RELATIVE_H
#define TETRIS_RELATIVE_H

#include "poly.h"
#include <vector>
#include <set>

extern const unsigned* const selfGroupOf;
extern const unsigned* const localGroupOf;
extern unsigned opposite[6];
extern const Pos offsets[6];

struct Tet {

	unsigned int n;

	std::vector<uint32_t> pieces;

	/**
	 * Coordinates of each block. THIS FIELD IS NOT MAINTAINED FOR EXTENDED FUNCTIONALITY.
	 */
	std::vector<Pos> coords;

	std::vector<int> neighbours;

	Tet(unsigned int n, const std::vector<Pos>& coordinates);

	/**
	 * DO NOT USE THIS FOR EXPERIMENTATION
	 */
	Tet(unsigned int n, const std::vector<uint32_t>& pieces, const std::vector<Pos>& coords,
		const std::vector<int>& neighbours);

	/**
	 * THIS FUNCTION DOES NOT UPDATE COORDINATES.
	 */
	Tet& rotX();

	/**
	 * THIS FUNCTION DOES NOT UPDATE COORDINATES.
	 */
	Tet& rotY();

	/**
	 * THIS FUNCTION DOES NOT UPDATE COORDINATES.
	 */
	Tet& rotZ();

	std::vector<Pos> getFreeSpaces() const;

	Tet insert(const Pos& block) const;

	std::vector<unsigned> groupEncode() const;

	std::vector<unsigned> encode() const;

	void print() const;
};

inline bool compareGroupEncodings(const std::vector<unsigned>& A, const std::vector<unsigned>& B);

inline bool compareEncodings(const std::vector<unsigned>& A, const std::vector<unsigned>& B);


bool spinCompare(Tet a, Tet b);

std::vector<Tet> generate(unsigned int i);

void write(const std::vector<Tet>& v);


#endif //TETRIS_RELATIVE_H
