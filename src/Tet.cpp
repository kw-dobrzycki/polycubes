//
// Created by Chris on 12/08/2023.
//
#include "Tet.h"
#include "relative.h"

Tet::Tet(unsigned int n, const std::vector<Pos>& coordinates)
		: n(n),
		  pieces(n),
		  coords(coordinates),
		  population(n),
		  neighbours(6 * n) {

	//decode connectivity (name the pieces for now)
	for (int i = 0; i < n; ++i) {

		for (int k = 0; k < n; ++k) {
			//get distance
			Pos dist = coords[k] - coords[i];

			//if neighbour, j decides face
			auto d = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
			if (d == 1) {
				int j = 0;
				if (dist.z < 0) {
					j = 1;
				} else if (dist.x > 0) {
					j = 2;
				} else if (dist.z > 0) {
					j = 3;
				} else if (dist.x < 0) {
					j = 4;
				} else if (dist.y < 0) {
					j = 5;
				}

				pieces[i] |= 1 << (5 - j) * 4; //mark as having a neighbour
				neighbours[i * 6 + j] = k + 1; //just like names but these don't change
			}

			//check if in 3x3x3 area
			if (d <= 3) {
				population[i] += 1;
			}
		}
	}

	//get self type and update neighbours
	for (int i = 0; i < n; ++i) {
		int self = 0;

		for (int j = 0; j < 6; ++j) {
			if (pieces[i] & 0xF << j * 4) {
				self += 1 << j;
			}
		}
		auto group = selfGroupOf[self];

		//update neighbours
		for (int j = 0; j < 6; ++j) {
			const auto ni = neighbours[i * 6 + j];
			if (ni) {
				auto& neighbour = pieces[ni - 1];
				neighbour = neighbour & ~(0xF << (5 - opposite[j]) * 4) | group << (5 - opposite[j]) * 4;
			}
		}
	}
}

Tet::Tet(unsigned int n, const std::vector<uint32_t>& pieces, const std::vector<Pos>& coords,
		 const std::vector<int>& neighbours, const std::vector<uint8_t>& population)
		: n(n),
		  pieces(pieces),
		  coords(coords),
		  population(population),
		  neighbours(neighbours) {}

Tet& Tet::rotX(unsigned i) {
	for (int j = 0; j < n; ++j) {
		for (int k = 0; k < i; ++k) {
			auto t = coords[j].z;
			coords[j].z = -coords[j].y;
			coords[j].y = t;
		}
	}
	return *this;
}

Tet& Tet::rotY(unsigned i) {
	for (int j = 0; j < n; ++j) {
		for (int k = 0; k < i; ++k) {
			auto t = coords[j].x;
			coords[j].x = -coords[j].z;
			coords[j].z = t;
		}
	}
	return *this;
}

Tet& Tet::rotZ(unsigned i) {
	for (int j = 0; j < n; ++j) {
		for (int k = 0; k < i; ++k) {
			auto t = coords[j].y;
			coords[j].y = -coords[j].x;
			coords[j].x = t;
		}
	}
	return *this;
}

Tet Tet::getComplement() const {

	auto bounds = getBounds();
	int ax = bounds[0], aX = bounds[1],
			ay = bounds[2], aY = bounds[3],
			az = bounds[4], aZ = bounds[5];

	for (int i = 0; i < n; ++i) {
		ax = std::min(ax, coords[i].x);
		ay = std::min(ay, coords[i].y);
		az = std::min(az, coords[i].z);
		aX = std::max(aX, coords[i].x);
		aY = std::max(aY, coords[i].y);
		aZ = std::max(aZ, coords[i].z);
	}

	std::vector<Pos> complement;

	for (int i = ax; i <= aX; ++i) {
		for (int j = ay; j <= aY; ++j) {
			for (int k = az; k <= aZ; ++k) {
				bool taken = false;
				for (const auto& p: coords) {
					if (p.x == i && p.y == j && p.z == k) {
						taken = true;
						break;
					}
				}
				if (!taken) {
					complement.push_back({i, j, k});
				}
			}
		}
	}

	return {static_cast<unsigned int>(complement.size()), complement};
}

std::vector<Pos> Tet::getFreeSpaces() const {
	std::set<int> unique;
	std::vector<Pos> spaces;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {
			if (!neighbours[i * 6 + j]) {
				Pos space = coords[i] + offsets[j];
				bool seen = unique.count(space.y * n * n + space.z * n + space.x) > 0;
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
	decltype(population) c_population(n + 1);

	c_pieces.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_neighbours.push_back(0);
	c_coords.push_back(block);

	//set update the population (naive for now)
	for (int i = 0; i < n + 1; ++i) {
		for (int k = 0; k < n + 1; ++k) {
			//get distance
			Pos dist = c_coords[k] - c_coords[i];
			auto d = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
			//check if in 3x3x3 area
			if (d <= 3) {
				c_population[i] += 1;
			}
		}
	}

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

	return Tet{n + 1, c_pieces, c_coords, c_neighbours, c_population};
}

std::vector<unsigned> Tet::encodeLocal() const {
	std::vector<unsigned> code(n);
	for (int i = 0; i < n; ++i) {
		code[i] = localGroupOf[LocalGroup::toDen(pieces[i])];
	}
	return code;
}

std::vector<unsigned> Tet::encodeSelf() const {
	std::vector<unsigned> code(n);
	for (int i = 0; i < n; ++i) {
		int self = 0;

		for (int j = 0; j < 6; ++j) {
			if (pieces[i] & (0xF << j * 4)) {
				self += 1 << j;
			}
		}

		code[i] = selfGroupOf[self];
	}
	return code;
}

std::array<uint32_t, 128> Tet::boundEncode() const {
	unsigned bitsize = 32;
	auto bounds = getBounds();
	auto volume = (bounds[1] - bounds[0] + 1) * (bounds[3] - bounds[2] + 1) * (bounds[5] - bounds[4] + 1);
	auto X = bounds[1] - bounds[0] + 1;
	auto Z = bounds[5] - bounds[4] + 1;
	std::array<uint32_t, 128> bits{};

	for (int i = 0; i < n; ++i) {
		//set (x, y, z) as the encoding origin
		unsigned linear = (coords[i].y - bounds[2]) * X * Z +
						  (coords[i].z - bounds[4]) * X +
						  (coords[i].x - bounds[0]);
		uint32_t& bit = bits[linear / bitsize];
		bit ^= uint32_t(1) << (bitsize - 1 - linear % bitsize);
	}

	return bits;
}

std::vector<uint64_t> Tet::fullEncode() const {
	std::vector<uint64_t> bits((n * n * n + 64 - 1) / 64);

	auto bounds = getBounds();

	for (int i = 0; i < n; ++i) {
		unsigned linear = (coords[i].y - bounds[2]) * n * n +
						  (coords[i].z - bounds[4]) * n +
						  (coords[i].x - bounds[0]);
		auto& bit = bits[linear / 64];
		bit ^= uint64_t(1) << (64 - 1 - linear % 64);
	}

	return bits;
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

std::array<int, 6> Tet::getBounds() const {
	int ax = 0, ay = 0, az = 0, aX = 0, aY = 0, aZ = 0;

	for (int i = 0; i < n; ++i) {
		ax = std::min(ax, coords[i].x);
		ay = std::min(ay, coords[i].y);
		az = std::min(az, coords[i].z);
		aX = std::max(aX, coords[i].x);
		aY = std::max(aY, coords[i].y);
		aZ = std::max(aZ, coords[i].z);
	}

	return {ax, aX, ay, aY, az, aZ};
}