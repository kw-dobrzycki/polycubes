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

extern const unsigned* const selfGroupOf;
extern const unsigned* const localGroupOf;
extern unsigned opposite[6];
extern const Pos offsets[6];

/*
 * Assumes A and B have same length
 */
template<class T, int range, int max_items>
bool sparseCompare(const std::vector<T>& A, const std::vector<T>& B) {
	thread_local int* counters = new int[range]();
	thread_local unsigned* indices = new unsigned[max_items]();
	int items = 0;

	for (int i = 0; i < A.size(); ++i) {
		counters[A[i]]++;
		counters[B[i]]--;
		indices[items++] = A[i];
		indices[items++] = B[i];
	}

	bool equal = true;
	int tail = 0;
	for (int i = 0; i < items; ++i) {
		if (counters[indices[i]] != 0) {
			equal = false;
			tail = i;
			break;
		}

		//counter is 0 but index is not, clean up index
		indices[i] = 0;
	}

	//clean up the rest
	for (int i = tail; i < items; ++i) {
		counters[indices[i]] = 0;
		indices[i] = 0;
	}

	return equal;
}

inline auto compareLocalEncodings = sparseCompare<unsigned, 43450, 20 * 20 * 20>;

inline auto comparePopulations = sparseCompare<unsigned, 27, 20 * 20 * 20>;

/**
 * @return translational and rotational equivalence
 */
bool fullCompare(const Tet& A, const Tet& B);

std::pair<std::vector<unsigned int>, std::vector<unsigned int>>
getRareSeeds(const std::vector<unsigned int>& A, const std::vector<unsigned int>& B);

std::vector<Tet> generate(unsigned int i);

void write(const std::vector<Tet>& v);

std::vector<Tet> read(std::string_view path);


#endif //TETRIS_RELATIVE_H
