//
// Created by croissant on 24/07/2023.
//
#include <iostream>
#include <fstream>
#include "relative.h"

Tet::Tet(unsigned int n, const std::vector<Pos>& coordinates)
		: n(n),
		  pieces(6 * n),
		  coords(coordinates),
		  neighbours(6 * n) {

	//decode connectivity (name the pieces for now)
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {
			auto offset = coordinates[i] + offsets[j];
			//go through each face and check for a block at that coordinate
			for (int k = 0; k < n; ++k) {
				if (offset == coordinates[k]) {
					pieces[i * 6 + j] = k + 1; //names start from 1
					neighbours[i * 6 + j] = k + 1; //just like names but these don't change
				}
			}
		}
	}

	//decode neighbour types
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {
			int neighbour = i * 6 + j;
			if (pieces[neighbour]) {
				int neighbourName = pieces[neighbour] - 1;
				//calculate the group of the neighbour
				int sum = 0;
				for (int k = 0; k < 6; ++k) {
					sum += (pieces[neighbourName * 6 + k] > 0) << (5 - k);
				}
				pieces[neighbour] = selfGroupOf[sum];
			}
		}
	}
}

Tet::Tet(unsigned int n, const std::vector<int>& pieces, const std::vector<Pos>& coords,
		 const std::vector<int>& neighbours)
		: n(n),
		  pieces(pieces),
		  coords(coords),
		  neighbours(neighbours) {}

Tet& Tet::rot(unsigned int i, const int* shift) {
	for (int j = 0; j < i; ++j) {
		for (int k = 0; k < n; ++k) {
			int tmp = pieces[6 * k + shift[3]];
			pieces[6 * k + shift[3]] = pieces[6 * k + shift[2]];
			pieces[6 * k + shift[2]] = pieces[6 * k + shift[1]];
			pieces[6 * k + shift[1]] = pieces[6 * k + shift[0]];
			pieces[6 * k + shift[0]] = tmp;
		}
	}
	return *this;
}

Tet& Tet::rotX(unsigned int i) {
	int shift[]{0, 1, 5, 3};
	return rot(i, shift);
}

Tet& Tet::rotY(unsigned int i) {
	int shift[]{1, 2, 3, 4};
	return rot(i, shift);
}

Tet& Tet::rotZ(unsigned int i)  {
	int shift[]{0, 2, 5, 4};
	return rot(i, shift);
}

std::vector<Pos> Tet::getFreeSpaces() const {
	std::set<int> unique;
	std::vector<Pos> spaces;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {
			if (!pieces[i * 6 + j]) {
				Pos space = coords[i] + offsets[j];
				bool seen = unique.contains(space.y * n * n + space.z * n + space.x);
				if (!seen) {
					unique.insert(seen);
					spaces.push_back(space);
				}
			}
		}
	}

	return spaces;
}

Tet Tet::insert(const Pos& block) const {
	auto c_pieces = pieces;
	auto c_coords = coords;
	auto c_neighbours = neighbours;

	c_pieces.push_back(0);
	c_pieces.push_back(0);
	c_pieces.push_back(0);
	c_pieces.push_back(0);
	c_pieces.push_back(0);
	c_pieces.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_coords.push_back(block);

	//find neighbours
	for (int i = 0; i < 6; ++i) {
		for (int j = 0; j < n; ++j) {
			if (block + offsets[i] == coords[j]) {
				c_neighbours[n * 6 + i] = j + 1; //names start at 1
				c_neighbours[j * 6 + opposite[i]] = n + 1;
			}
		}
	}

	int selfType = 0; //depends on the piece type of neighbours, not neighbourhood type
	for (int k = 0; k < 6; ++k) {
		selfType += (c_neighbours[n * 6 + k] > 0) << (5 - k);
	}


	//change 0 bits of direct neighbours to the self group of new block.
	for (int i = 0; i < 6; ++i) {
		int dirNeighbour = c_neighbours[n * 6 + i];
		if (dirNeighbour) {
			c_pieces[(dirNeighbour - 1) * 6 + opposite[i]] = selfGroupOf[selfType];

			//compute new self type of the neighbour for indirect neighbours to update (this includes block)
			int neighbourType = 0;
			for (int k = 0; k < 6; ++k) {
				neighbourType += (c_neighbours[(dirNeighbour - 1) * 6 + k] > 0) << (5 - k);
			}


			//change non-0 bits of indirect neighbours
			for (int j = 0; j < 6; ++j) {
				int indirNeighbour = c_neighbours[(dirNeighbour - 1) * 6 + j];
				if (indirNeighbour) {
					c_pieces[(indirNeighbour - 1) * 6 + opposite[j]] = selfGroupOf[neighbourType];
				}
			}
		}
	}

	return Tet{n + 1, c_pieces, c_coords, c_neighbours};
}

std::vector<int> Tet::groupEncode() const {
	std::vector<int> code(n);
	for (int i = 0; i < n; ++i) {
		int x = pieces[i * 6 + 0] * 100000 +
				pieces[i * 6 + 1] * 10000 +
				pieces[i * 6 + 2] * 1000 +
				pieces[i * 6 + 3] * 100 +
				pieces[i * 6 + 4] * 10 +
				pieces[i * 6 + 5];

		code[i] = localGroupOf[x];
	}
	return code;
}

std::vector<int> Tet::encode() const {
	std::vector<int> code(n);
	for (int i = 0; i < n; ++i) {
		int x = pieces[i * 6 + 0] * 100000 +
				pieces[i * 6 + 1] * 10000 +
				pieces[i * 6 + 2] * 1000 +
				pieces[i * 6 + 3] * 100 +
				pieces[i * 6 + 4] * 10 +
				pieces[i * 6 + 5];

		code[i] = x;
	}
	return code;
}

void Tet::print() const {
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {
			std::cout << pieces[i * 6 + j];
		}
		std::cout << "|";
	}
	std::cout << std::endl;
}

bool compareGroupEncodings(const std::vector<int>& A, const std::vector<int>& B) {
	thread_local int* counters = new int[43450]();
	thread_local int* indices = new int[50](); //note this implementation supports maximum 25 blocks
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

bool compareEncodings(const std::vector<int>& A, const std::vector<int>& B) {
	thread_local int* counters = new int[1000000]();
	thread_local int* indices = new int[50](); //note this implementation supports maximum 25 blocks
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

bool spinCompare(Tet a, Tet b) {
	auto Acode = a.encode();
	//spin on top
	for (int i = 0; i < 4; ++i) {
		b.rotY(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on back
	b.rotX(1);
	for (int i = 0; i < 4; ++i) {
		b.rotZ(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on right
	b.rotY(1);
	for (int i = 0; i < 4; ++i) {
		b.rotX(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on front
	b.rotY(1);
	for (int i = 0; i < 4; ++i) {
		b.rotZ(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on left
	b.rotY(1);
	for (int i = 0; i < 4; ++i) {
		b.rotX(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on bottom
	b.rotY(1);
	b.rotX(1);
	for (int i = 0; i < 4; ++i) {
		b.rotY(1);
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	return false;
}

std::vector<Tet> generate(unsigned int i) {
	if (i <= 1) {
		return std::vector{Tet(i, std::vector<Pos>(1, {0, 0, 0}))};
	}

	auto previous = generate(i - 1);
	std::vector<Tet> unique;

	for (auto& p: previous) {
		auto faces = p.getFreeSpaces();

		for (auto& f: faces) {
			Tet build(p.insert(f));
			auto buildCode = build.groupEncode();
			bool newShape = true;

			for (auto& u: unique) {
				auto uCode = u.groupEncode();
				//if there's a new group, keep true, check next u
//				if (!compareGroupEncodings(uCode, buildCode))
//					continue;

				//if all groups match, spin
				//if same at some rotation, set false, break
				if (spinCompare(u, build)) {
					newShape = false;
					break;
				}
			}

			if (newShape)
				unique.push_back(build);
		}
	}

	return unique;
}

void write(const std::vector<Tet>& v) {
	std::cout << "saving " << v.size() << " results..." << std::endl;
	std::ofstream file;
	file.open("./blocks.txt");
	for (auto& t: v) {
		for (auto& k: t.coords) {
			file << k.x << " " << k.y << " " << k.z << std::endl;
		}
		file << std::endl;
	}
	file.close();
}
