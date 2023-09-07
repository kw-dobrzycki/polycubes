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
	c.push_back(block);
	return Tet{n + 1, c};
}

template<class T>
T max3(const T& t1, const T& t2, const T& t3) {
	return std::max(std::max(t1, t2), t3);
}

std::vector<uint64_t> Tet::fullEncode() const {

	auto bounds = getBounds();
	unsigned last = ((bounds[3] - bounds[2]) * n * n +
					 (bounds[5] - bounds[4]) * n +
					 (bounds[1] - bounds[0]) + 63 + 1) / 64;

	std::vector<uint64_t> bits(last);

	for (int i = 0; i < n; ++i) {
		unsigned linear = (coords[i].y - bounds[2]) * n * n +
						  (coords[i].z - bounds[4]) * n +
						  (coords[i].x - bounds[0]);
		auto& bit = bits[linear / 64];
		bit ^= uint64_t(1) << (64 - 1 - linear % 64);
	}

	return bits;
}

std::vector<uint64_t> Tet::volumeEncode() const {
	auto bounds = getBounds();
	auto X = bounds[1] - bounds[0] + 1;
	auto Y = bounds[3] - bounds[2] + 1;
	auto Z = bounds[5] - bounds[4] + 1;
	Pos::boundType layers = max3(X, Y, Z); //in range

	std::vector<uint64_t> ints((layers * layers * layers + 63) / 64);

	for (int i = 0; i < coords.size(); ++i) {
		const Pos& c = coords[i];
		auto x = c.x - bounds[0];
		auto y = c.y - bounds[2];
		auto z = c.z - bounds[4];
		auto layer = max3(x, y, z) + 1;
		bool hat = x < layer - 1 && z < layer - 1;
		unsigned bit = !hat * (x + z * (x == layer - 1) + y * (2 * layer - 1)) +
					   hat * (x + z * (layer - 1) + (y + 1) * (2 * layer - 1));
		bit += (layer - 1) * (layer - 1) * (layer - 1);
		ints[bit / 64] |= uint64_t(0b1) << (64 - bit % 64 - 1);
	}

	for (unsigned i = ints.size() - 1; true; --i) {
		if (ints[i] == 0)
			ints.pop_back();
		else break;
	}
	return ints;
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

Tet::Tet(unsigned int n, const std::vector<Pos>& coords) : n(n), coords(coords) {}
