//
// Created by Chris on 12/08/2023.
//
#include "Tet.h"
#include "relative.h"

Tet& Tet::rotX(unsigned i) {
	for (int j = 0; j < n; ++j) {
		for (int k = 0; k < i; ++k) {
			Pos::type t = coords[j].z;
			coords[j].z = -coords[j].y;
			coords[j].y = t;
		}
	}
	for (int j = 0; j < spaces.size(); ++j){
		for (int k = 0; k < i; ++k){
			Pos::type t = spaces[j].z;
			spaces[j].z = -spaces[j].y;
			spaces[j].y = t;
		}
	}
	return *this;
}

Tet& Tet::rotY(unsigned i) {
	for (int j = 0; j < n; ++j) {
		for (int k = 0; k < i; ++k) {
			Pos::type t = coords[j].x;
			coords[j].x = -coords[j].z;
			coords[j].z = t;
		}
	}
	for (int j = 0; j < spaces.size(); ++j){
		for (int k = 0; k < i; ++k){
			Pos::type t = spaces[j].x;
			spaces[j].x = -spaces[j].z;
			spaces[j].z = t;
		}
	}
	return *this;
}

Tet& Tet::rotZ(unsigned i) {
	for (int j = 0; j < n; ++j) {
		for (int k = 0; k < i; ++k) {
			Pos::type t = coords[j].y;
			coords[j].y = -coords[j].x;
			coords[j].x = t;
		}
	}
	for (int j = 0; j < spaces.size(); ++j){
		for (int k = 0; k < i; ++k){
			Pos::type t = spaces[j].y;
			spaces[j].y = -spaces[j].x;
			spaces[j].x = t;
		}
	}
	return *this;
}

std::vector<Pos> Tet::getFreeSpaces() const {
	std::set<int> unique;
	std::vector<Pos> spaces;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < 6; ++j) {
			bool taken = false;
			for (int k = 0; k < n; ++k) {
				if (coords[i] + offsets[j] == coords[k] && i != k) {
					taken = true;
					break;
				}
			}
			if (!taken) {
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
	auto c = coords;
	auto s = spaces;

	c.push_back(block);

	//remove newly occupied space
	for (int i = 0; i < s.size(); ++i) {
		if (s[i] == block) {
			s[i] = s.back();
			s.pop_back();
			break;
		}
	}

	//add spaces around new block
	for (int i = 0; i < 6; ++i) {
		bool occupied = false;
		for (int j = 0; j < n; ++j) {
			if (block + offsets[i] == coords[j]) {
				occupied = true;
				break;
			}
		}

		if (!occupied) {
			bool seen = false;
			for (int j = 0; j < s.size(); ++j) {
				if (s[j] == block + offsets[i]) {
					seen = true;
					break;
				}
			}
			if (!seen)
				s.push_back(block + offsets[i]);
		}

	}

	return Tet{n + 1, c, s};
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

std::array<Pos::type, 6> Tet::getBounds() const {
	Pos::type ax = 0, ay = 0, az = 0, aX = 0, aY = 0, aZ = 0;

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

Tet::Tet(unsigned int n, const std::vector<Pos>& coords, const std::vector<Pos>& spaces)
		: n(n), coords(coords), spaces(spaces) {}
