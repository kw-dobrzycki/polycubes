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
#include <unordered_map>
#include <algorithm>

const Pos offsets[6]{
		{0,  1,  0},
		{0,  0,  -1},
		{1,  0,  0},
		{0,  0,  1},
		{-1, 0,  0},
		{0,  -1, 0},
};

extern const int numThreads;

Tet getMaxRotation(Tet t);

std::vector<Tet> generate(unsigned int i);

void write(const std::vector<Tet>& v);

std::vector<Tet> read(std::string_view path);

static unsigned long long collisions = 0;
static unsigned long long comparisons = 0;
static unsigned long long tables = 0;
static unsigned long long items = 0;

template<unsigned depth>
struct NestedHash {
	std::unordered_map<uint64_t, NestedHash<depth - 1>> map;
	std::vector<uint64_t> store;

	inline static uint64_t loadhi = 0;
	inline static uint64_t count = 0;
	inline static uint64_t loadtot = 0;

	static void print() {
		std::cout << "Depth " << depth << " total load " << loadtot << " highest load " << loadhi << " count " << count
				  << std::endl;
		NestedHash<depth - 1>::print();
	}

	NestedHash() {
		tables++;
		count++;
	}

	static void reset() {
		loadhi = 0;
		count = 0;
		tables = 0;
		items = 0;
		collisions = 0;
		comparisons = 0;
		loadtot = 0;
		NestedHash<depth - 1>::reset();
	}

	std::vector<std::vector<std::uint64_t>> collect(const std::vector<uint64_t>& parent = {}) const {
		std::vector<std::vector<uint64_t>> own;
		own.reserve(store.size());
		for (const auto& t: store) {
			std::vector<uint64_t> child;
			child.reserve(parent.size() + 1);
			child.insert(child.end(), parent.begin(), parent.end() - 1);
			child.push_back(t);
			own.push_back(std::move(child));
		}

		for (const auto& [k, v]: map) {
			auto p = parent;
			p.push_back(k);

			for (auto child: v.collect(p)) {
				own.push_back(child);
			}
		}
		return own;
	}

	bool contains(const std::vector<uint64_t>& t) const {
		return lookup(t);
	}

	bool lookup(const std::vector<uint64_t>& encoding, unsigned bit = 0) const {
		if (bit == encoding.size()) {
			for (unsigned long long i: store) {
				comparisons++;
				if (i == encoding[bit - 1]) return true;
			}
			return false;
		}

		if (map.count(encoding[bit]) <= 0)
			return false;

		return map.at(encoding[bit]).lookup(encoding, bit + 1);
	}

	void insert(const std::vector<uint64_t>& t) {
		add(t);
		items++;
	}

	void add(const std::vector<uint64_t>& encoding, unsigned bit = 0) {
		//if processed all ints
		if (bit == encoding.size()) {
			if (store.size() > 1)
				collisions++;
			store.push_back(encoding[bit - 1]);
			loadtot++;
			if (store.size() > loadhi)
				loadhi = store.size();
			return;
		}

		map[encoding[bit]].add(encoding, bit + 1);
	}
};

//stores the overflow of bits
template<>
struct NestedHash<0> {
	std::vector<std::vector<uint64_t>> store;

	inline static uint64_t loadhi = 0;
	inline static uint64_t count = 0;
	inline static uint64_t loadtot = 0;

	static void print() {
		std::cout << "Depth " << 0 << " total load " << loadtot << " highest load " << loadhi << " count " << count
				  << std::endl;
	}

	NestedHash() {
		tables++;
		count++;
	}

	static void reset() {
		loadhi = 0;
		count = 0;
		tables = 0;
		comparisons = 0;
		items = 0;
		collisions = 0;
		loadtot = 0;
	}

	std::vector<std::vector<uint64_t>> collect(std::vector<uint64_t> parent = {}) const {
		std::vector<std::vector<uint64_t>> res;
		res.reserve(store.size());
		for (const auto& t: store) {
			std::vector<uint64_t> r;
			r.insert(r.end(), parent.begin(), parent.end() - 1);
			r.insert(r.end(), t.begin(), t.end());
			res.push_back(r);
		}
		return res;
	}

	bool contains(const Tet& t) const {
		return lookup(t.volumeEncoding, 0);
	}

	bool lookup(std::vector<uint64_t> encoding, unsigned bit) const {
		std::vector<uint64_t> trailing(encoding.size() - bit + 1);
		for (unsigned i = bit - 1; i < encoding.size(); ++i) {
			trailing[i - bit + 1] = encoding[i];
		}
		for (const auto& i: store) {
			comparisons++;
			if (i == trailing) return true;
		}
		return false;
	}

	void insert(const Tet& t) {
		add(t.volumeEncoding, 0);
	}

	void add(const std::vector<uint64_t>& encoding, unsigned bit) {
		if (store.size() > 1)
			collisions++;
		std::vector<uint64_t> trailing(encoding.size() - bit + 1);
		for (unsigned i = bit - 1; i < encoding.size(); ++i) {
			trailing[i - bit + 1] = encoding[i];
		}
		store.push_back(std::move(trailing));
		loadtot++;
		if (store.size() > loadhi)
			loadhi = store.size();
	}
};


inline uint64_t encodeBound(Pos bound) {
	Pos::type v[3]{bound.x, bound.y, bound.z};
	std::sort(v, v + 3);
	bound.x = v[2];
	bound.z = v[1];
	bound.y = v[0];
	uint64_t e = 0;
	e |= bound.x << Pos::width * 2;
	e |= bound.y << Pos::width;
	e |= bound.z;
	return e;
}

//maps between vec3 (concat encoding) and NestedHash
//this is the global result
template<unsigned depth>
struct BoundCache {

	std::unordered_map<uint64_t, NestedHash<depth>> caches;
	std::vector<uint64_t> keys;

	explicit BoundCache(unsigned n) : caches(allocCaches(n)) {}

	std::vector<Tet> collect() {
		std::vector<Tet> res;
		for (const auto& [k, v]: caches) {
			const auto& r = v.collect();
			res.insert(res.end(), r.begin(), r.end());
		}
		return res;
	}

	auto& at(auto e) {
		return caches.at(e);
	}

	std::unordered_map<uint64_t, NestedHash<depth>> allocCaches(Pos::type n) { //todo assert range
		decltype(caches) cache;

		//todo brute forcing all configurations - n^3 - there is a pattern
		for (Pos::type i = 1; i < n + 1; ++i) {
			for (Pos::type j = 1; j < n + 1; ++j) {
				for (Pos::type k = 1; k < n + 1; ++k) {

					//if enough volume for n pieces
					if (i * j * k >= n) {

						//get canonical orientation
						auto encoding = encodeBound(Pos{i, j, k});
						if (!caches.contains(encoding)) {
							cache.insert({encoding, {}});
							keys.push_back(encoding);
						}
					}
				}
			}
		}
		return cache;
	}
};

//temporary local results from each Tet
struct LocalBoundCache {
	std::unordered_map<uint64_t, std::vector<Tet>> caches;

	LocalBoundCache(unsigned n) : caches(allocBounds(n)) {}

	void clear() {
		for (auto& [k, v]: caches) {
			v.clear();
		}
	}

	auto& at(auto e) {
		return caches.at(e);
	}

private:
	std::unordered_map<uint64_t, std::vector<Tet>> allocBounds(Pos::type n) { //assert range
		std::unordered_map<uint64_t, std::vector<Tet>> cache;
		for (Pos::type i = 1; i < n + 1; ++i) {
			for (Pos::type j = 1; j < n + 1; ++j) {
				for (Pos::type k = 1; k < n + 1; ++k) {

					//if enough volume for n pieces
					if (i * j * k >= n) {

						//get canonical orientation
						auto encoding = encodeBound(Pos{i, j, k});
						if (!caches.contains(encoding)) {
							cache.insert({encoding, {}});
						}
					}
				}
			}
		}
		return cache;
	}
};

#endif //TETRIS_RELATIVE_H
