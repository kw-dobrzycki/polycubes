#include <iostream>
#include "Tet.h"
#include "poly.h"

using EncodeType = BasicEncoding<unsigned>;

bool connected(const Pos* tet, unsigned n) {
	std::vector<bool> seen(n);
	std::vector<size_t> q({0});

	while (!q.empty()) {
		size_t i = q.back();
		q.pop_back();
		Pos current = tet[i];
		if (seen[i]) continue;
		seen[i] = true;

		for (int j = 0; j < n; ++j) {
			Pos d = current - tet[j];
			bool touch = false;
			for (const auto& k: offsets) {
				if (d == k) {
					touch = true;
					break;
				}
			}
			if (touch && !seen[j])
				q.push_back(j);
		}
	}

	for (auto&& i: seen)
		if (!i) return false;

	return true;
}

template<unsigned n>
std::vector<Tet<n>> generate() {
	std::vector<Tet<n - 1>> previous = generate<n - 1>();

	std::vector<Tet<n>> global;

	for (auto& parent: previous) {
		EncodeType parent_encoding(parent.units, n - 1);

		std::vector<Tet<n>> local;

		for (auto& space: parent.getFreeSpaces()) {

			Tet<n> child = parent.insert(space);
			orient(child.units, n);
			EncodeType child_encoding(child.units, n);

			Tet<n - 1> max_stem;

			for (unsigned i = 0; i < n; ++i) {

				Tet<n - 1> stem = child.remove(i);
				if (!connected(stem.units, n - 1)) continue;

				orient(stem.units, n - 1);

				if (orient_compare(stem.units, max_stem.units, n - 1)) {
					max_stem = stem;
				}
			}

			if (parent_encoding == EncodeType(max_stem.units, n - 1)) {
				bool seen = false;
				for (int i = 0; i < local.size(); ++i) {
					auto e = EncodeType (local[i].units, n);
					if (child_encoding == EncodeType(local[i].units, n)) {
						seen = true;
						break;
					}
				}
				if (!seen) local.push_back(child);
			}
		}

		global.insert(global.end(), local.begin(), local.end());
	}
	std::cout << "N=" << n << ": " << global.size() << std::endl;
	return global;
}

template<>
std::vector<Tet<1>> generate() {
	Pos unit{0, 0, 0};
	return {Tet<1>{&unit}};
}

template<>
std::vector<Tet<0>> generate() {
	return {};
};

int main() {
	auto results = generate<10>();
	std::cout << results.size() << std::endl;
	return 0;
}
