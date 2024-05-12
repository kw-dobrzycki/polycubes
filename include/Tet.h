//
// Created by Chris on 12/08/2023.
//

#ifndef TETRIS_TET_H
#define TETRIS_TET_H

#include <vector>
#include <cstring>

constexpr unsigned maxUnits = 16;

struct Pos {
	int x{}, y{}, z{};

	Pos operator+(const Pos& v) const {
		return {x + v.x, y + v.y, z + v.z};
	}

	Pos operator-(const Pos& v) const {
		return {x - v.x, y - v.y, z - v.z};
	}

	Pos& operator+=(const Pos& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	Pos& operator-=(const Pos& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	Pos operator~() const {
		return {-x, -y, -z};
	}

	bool operator==(const Pos& v) const {
		return x == v.x && y == v.y && z == v.z;
	}

	bool isUnit() const {
		return x * x + y * y + z * z == 1;
	}
};

Pos offsets[6]{
		{0,  1,  0},
		{0,  0,  1},
		{1,  0,  0},
		{0,  0,  -1},
		{-1, 0,  0},
		{0,  -1, 0}
};

void transAdd(Pos* tet, unsigned n, const Pos& p) {
	for (int i = 0; i < n; ++i) {
		tet[i] += p;
	}
}

void transSub(Pos* tet, unsigned n, const Pos& p) {
	for (int i = 0; i < n; ++i) {
		tet[i] -= p;
	}
}

void rotX(Pos* tet, unsigned n, unsigned i = 1) {
	for (int k = 0; k < i % 4; k++) {
		for (int j = 0; j < n; ++j) {
			auto t = tet[j].z;
			tet[j].z = -tet[j].y;
			tet[j].y = t;
		}
	}
}

void rotY(Pos* tet, unsigned n, unsigned i = 1) {
	for (int k = 0; k < i % 4; k++) {
		for (int j = 0; j < n; ++j) {
			auto t = tet[j].x;
			tet[j].x = -tet[j].z;
			tet[j].z = t;
		}
	}
}

void rotZ(Pos* tet, unsigned n, unsigned i = 1) {
	for (int k = 0; k < i % 4; k++) {
		for (int j = 0; j < n; ++j) {
			auto t = tet[j].y;
			tet[j].y = -tet[j].x;
			tet[j].x = t;
		}
	}
}


template<unsigned n>
struct Tet {

	Pos units[n]{};

	Tet() = default;

	explicit Tet(Pos* points) {
		memcpy(units, points, sizeof(Pos) * n);
	}

	std::vector<Pos> getFreeSpaces() const {
		std::vector<Pos> spaces;
		for (int i = 0; i < n; ++i) {
			for (const auto& opp: offsets) {
				Pos space = units[i] + opp;
				bool taken = false;

				for (const auto& unit: units) {
					if (space == unit) {
						taken = true;
						break;
					}
				}

				if (taken) continue;

				for (const auto& k: spaces) {
					if (space == k) {
						taken = true;
						break;
					}
				}

				if (!taken) spaces.push_back(space);
			}
		}
		return spaces;
	}

	bool operator!=(const Tet& other) const {
		for (int i = 0; i < n; ++i) {
			if (units[i] != other.units[i]) return true;
		}
		return false;
	}

	Tet<n + 1> insert(const Pos& unit) const {
		Pos points[n + 1]{};
		memcpy(points, units, sizeof(Pos) * n);
		points[n] = unit;
		return Tet<n + 1>(points);
	}

	Tet<n - 1> remove(size_t i) const {
		Pos points[n - 1]{};
		memcpy(points, units, sizeof(Pos) * i);
		memcpy(points + i, units + i + 1, sizeof(Pos) * (n - i - 1));
		return Tet<n - 1>(points);
	}
};

template<>
struct Tet<0> {
};

#endif //TETRIS_TET_H
