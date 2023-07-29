//
// Created by croissant on 24/07/2023.
//
#include <iostream>
#include <fstream>
#include "../include/relative.h"
#include "../include/groups.h"

const unsigned* const selfGroupOf{SelfGroup::generateSelfGroups()};
const unsigned* const localGroupOf{LocalGroup::generateLocalGroups()};

unsigned opposite[]{
		5, 3, 4, 1, 2, 0
};

const Pos offsets[]{
		{0,  1,  0},
		{0,  0,  -1},
		{1,  0,  0},
		{0,  0,  1},
		{-1, 0,  0},
		{0,  -1, 0},
};

Tet::Tet(unsigned int n, const std::vector<Pos>& coordinates)
		: n(n),
		  pieces(n),
		  coords(coordinates),
		  neighbours(6 * n) {

	//decode connectivity (name the pieces for now)
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {
			auto offset = coordinates[i] + offsets[j];
			//go through each face and check for a block at that coordinate
			for (int k = 0; k < n; ++k) {
				if (offset == coordinates[k]) {
					pieces[i] += (k + 1) << (5 - j) * 4; //names start from 1
					neighbours[i * 6 + j] = k + 1; //just like names but these don't change
				}
			}
		}
	}

	//decode neighbour types
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {
			int neighbour = i * 6 + j;
			if (neighbours[neighbour]) {
				int neighbourName = neighbours[neighbour] - 1;
				//calculate the group of the neighbour
				int sum = 0;
				for (int k = 0; k < 6; ++k) {
					sum += (neighbours[neighbourName * 6 + k] > 0) << (5 - k);
				}
				pieces[i] = (pieces[i] & ~(0xF << (5 - j) * 4)) | (selfGroupOf[sum] << (5 - j) * 4);
			}
		}
	}
}

Tet::Tet(unsigned int n, const std::vector<uint32_t>& pieces, const std::vector<Pos>& coords,
		 const std::vector<int>& neighbours)
		: n(n),
		  pieces(pieces),
		  coords(coords),
		  neighbours(neighbours) {}

Tet& Tet::rotX() {
	for (int j = 0; j < n; ++j) {
		pieces[j] = LocalGroup::X(pieces[j]);
	}
	return *this;
}

Tet& Tet::rotY() {
	for (int j = 0; j < n; ++j) {
		pieces[j] = LocalGroup::Y(pieces[j]);
	}
	return *this;
}

Tet& Tet::rotZ() {
	for (int j = 0; j < n; ++j) {
		pieces[j] = LocalGroup::Z(pieces[j]);
	}
	return *this;
}

std::vector<Pos> Tet::getFreeSpaces() const {
	std::set<int> unique;
	std::vector<Pos> spaces;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {
			if (!neighbours[i * 6 + j]) {
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


	//change the 0 bits of direct neighbours to the self group of new block.
	for (int i = 0; i < 6; ++i) {
		int dirNeighbour = c_neighbours[n * 6 + i];
		if (dirNeighbour) {
			c_pieces[(dirNeighbour - 1)] += selfGroupOf[selfType] << (5 - opposite[i]) * 4;

			//compute new self type of the neighbour for indirect neighbours to update (this includes block)
			int neighbourType = 0;
			for (int k = 0; k < 6; ++k) {
				neighbourType += (c_neighbours[(dirNeighbour - 1) * 6 + k] > 0) << (5 - k);
			}


			//change the non-0 bits of indirect neighbours
			for (int j = 0; j < 6; ++j) {
				int indirNeighbour = c_neighbours[(dirNeighbour - 1) * 6 + j];
				if (indirNeighbour) {
					c_pieces[(indirNeighbour - 1)] =
							(c_pieces[(indirNeighbour - 1)] & ~(0xF << (5 - opposite[j]) * 4)) |
							(selfGroupOf[neighbourType] << (5 - opposite[j]) * 4);
				}
			}
		}
	}

	return Tet{n + 1, c_pieces, c_coords, c_neighbours};
}

std::vector<unsigned> Tet::groupEncode() const {
	std::vector<unsigned> code(n);
	for (int i = 0; i < n; ++i) {
		code[i] = localGroupOf[LocalGroup::toDen(pieces[i])];
	}
	return code;
}

std::vector<unsigned> Tet::encode() const {
	std::vector<unsigned> code(n);
	for (int i = 0; i < n; ++i) {
		code[i] = LocalGroup::toDen(pieces[i]);
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

bool compareGroupEncodings(const std::vector<unsigned>& A, const std::vector<unsigned>& B) {
	thread_local int* counters = new int[43450]();
	thread_local unsigned* indices = new unsigned[50]();
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

bool compareEncodings(const std::vector<unsigned>& A, const std::vector<unsigned>& B) {
	thread_local int* counters = new int[1000000]();
	thread_local unsigned* indices = new unsigned[50]();
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
		b.rotY();
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on back
	b.rotX();
	for (int i = 0; i < 4; ++i) {
		b.rotZ();
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on right
	b.rotY();
	for (int i = 0; i < 4; ++i) {
		b.rotX();
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on front
	b.rotY();
	for (int i = 0; i < 4; ++i) {
		b.rotZ();
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on left
	b.rotY();
	for (int i = 0; i < 4; ++i) {
		b.rotX();
		if (compareEncodings(Acode, b.encode()))
			return true;
	}

	//spin on bottom
	b.rotY();
	b.rotX();
	for (int i = 0; i < 4; ++i) {
		b.rotY();
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
				if (!compareGroupEncodings(uCode, buildCode))
					continue;

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
