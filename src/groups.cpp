//
// Created by croissant on 25/07/2023.
//
#include "../include/groups.h"


namespace SelfGroup {

	uint8_t X(uint8_t b, unsigned int i) {
		std::bitset<6> bits(b);
		for (int j = 0; j < i % 4; ++j) {
			bool t = bits[0];
			bits[0] = bits[4];
			bits[4] = bits[5];
			bits[5] = bits[2];
			bits[2] = t;
		}
		return bits.to_ulong();
	}

	uint8_t Y(uint8_t b, unsigned int i) {
		std::bitset<6> bits(b);
		for (int j = 0; j < i % 4; ++j) {
			bool t = bits[4];
			bits[4] = bits[1];
			bits[1] = bits[2];
			bits[2] = bits[3];
			bits[3] = t;
		}
		return bits.to_ulong();
	}

	uint8_t Z(uint8_t b, unsigned int i) {
		std::bitset<6> bits(b);
		for (int j = 0; j < i % 4; ++j) {
			bool t = bits[0];
			bits[0] = bits[3];
			bits[3] = bits[5];
			bits[5] = bits[1];
			bits[1] = t;
		}
		return bits.to_ulong();
	}

	unsigned* generateSelfGroups() {
		unsigned n = 1 << 6;
		auto* groupOf = new unsigned[n](); //groups start at 1, write to file from 0

		std::vector<std::vector<uint8_t>> groups;

		for (int i = 0; i < n; ++i) {

			if (groupOf[i])
				continue;

			uint8_t k = i;
			auto& group = groups.emplace_back();

			for (int j = 0; j < 4; ++j) {
				k = Y(k);
				if (!groupOf[k]) {
					group.push_back(k);
					groupOf[k] = groups.size() - 1;
				}
			}

			k = X(k);
			for (int j = 0; j < 4; ++j) {
				k = Z(k);
				if (!groupOf[k]) {
					group.push_back(k);
					groupOf[k] = groups.size() - 1;
				}
			}

			k = Y(k);
			for (int j = 0; j < 4; ++j) {
				k = X(k);
				if (!groupOf[k]) {
					group.push_back(k);
					groupOf[k] = groups.size() - 1;
				}
			}

			k = Y(k);
			for (int j = 0; j < 4; ++j) {
				k = Z(k);
				if (!groupOf[k]) {
					group.push_back(k);
					groupOf[k] = groups.size() - 1;
				}
			}

			k = Y(k);
			for (int j = 0; j < 4; ++j) {
				k = X(k);
				if (!groupOf[k]) {
					group.push_back(k);
					groupOf[k] = groups.size() - 1;
				}
			}

			k = Y(k);
			k = X(k);
			for (int j = 0; j < 4; ++j) {
				k = Y(k);
				if (!groupOf[k]) {
					group.push_back(k);
					groupOf[k] = groups.size() - 1;
				}
			}
		}

		return groupOf;
	}

}

namespace LocalGroup {

	unsigned toDen(type b) {
		unsigned d = 0;
		d += (b & 0xF);
		d += ((b & 0xF0) >> 1 * 4) * 10;
		d += ((b & 0xF00) >> 2 * 4) * 100;
		d += ((b & 0xF000) >> 3 * 4) * 1000;
		d += ((b & 0xF0000) >> 4 * 4) * 10000;
		d += ((b & 0xF00000) >> 5 * 4) * 100000;
		return d;
	}

	type toBCD(unsigned int d) {
		type b = 0;

		unsigned d6 = d / 100000;
		b |= d6 << 4 * 5;

		unsigned d5 = (d - d6 * 100000) / 10000;
		b |= d5 << 4 * 4;

		unsigned d4 = (d - d6 * 100000 - d5 * 10000) / 1000;
		b |= d4 << 4 * 3;

		unsigned d3 = (d - d6 * 100000 - d5 * 10000 - d4 * 1000) / 100;
		b |= d3 << 4 * 2;

		unsigned d2 = (d - d6 * 100000 - d5 * 10000 - d4 * 1000 - d3 * 100) / 10;
		b |= d2 << 4;

		unsigned d1 = d - d6 * 100000 - d5 * 10000 - d4 * 1000 - d3 * 100 - d2 * 10;
		b |= d1;

		return b;
	}

	type X(type b, unsigned int i) {
		for (int j = 0; j < i % 4; ++j) {
			type m0 = 0xF;
			type m4 = 0xF0000;
			type m5 = 0xF00000;
			type m2 = 0xF00;

			type a0 = (b & m4) >> 4 * 4;
			type a4 = (b & m5) >> 1 * 4;
			type a5 = (b & m2) << 3 * 4;
			type a2 = (b & m0) << 2 * 4;

			b = (b & ~(m0 | m2 | m4 | m5)) | (a0 | a2 | a4 | a5);
		}
		return b;
	}

	type Y(type b, unsigned int i) {
		for (int j = 0; j < i % 4; ++j) {
			type m2 = 0xF00;
			type m1 = 0xF0;
			type m4 = 0xF0000;
			type m3 = 0xF000;

			type a1 = (b & m2) >> 1 * 4;
			type a2 = (b & m3) >> 1 * 4;
			type a3 = (b & m4) >> 1 * 4;
			type a4 = (b & m1) << 3 * 4;

			b = (b & ~(m1 | m2 | m3 | m4)) | (a1 | a2 | a3 | a4);
		}
		return b;
	}

	type Z(type b, unsigned int i) {
		for (int j = 0; j < i % 4; ++j) {
			type m0 = 0xF;
			type m1 = 0xF0;
			type m5 = 0xF00000;
			type m3 = 0xF000;

			type a0 = (b & m3) >> 3 * 4;
			type a1 = (b & m0) << 1 * 4;
			type a5 = (b & m1) << 4 * 4;
			type a3 = (b & m5) >> 2 * 4;

			b = (b & ~(m0 | m1 | m3 | m5)) | (a0 | a1 | a3 | a5);
		}
		return b;
	}

	unsigned* generateLocalGroups() {
		unsigned n = 1000000;
		auto* groupOf = new unsigned[n]();

		std::vector<std::vector<type>> groups;

		for (int i = 0; i < n; ++i) {

			if (groupOf[i])
				continue;

			type k = toBCD(i);
			auto& group = groups.emplace_back();

			for (int j = 0; j < 4; ++j) {
				k = Y(k);
				if (!groupOf[toDen(k)]) {
					group.push_back(k);
					groupOf[toDen(k)] = groups.size() - 1;
				}
			}

			k = X(k);
			for (int j = 0; j < 4; ++j) {
				k = Z(k);
				if (!groupOf[toDen(k)]) {
					group.push_back(k);
					groupOf[toDen(k)] = groups.size() - 1;
				}
			}

			k = Y(k);
			for (int j = 0; j < 4; ++j) {
				k = X(k);
				if (!groupOf[toDen(k)]) {
					group.push_back(k);
					groupOf[toDen(k)] = groups.size() - 1;
				}
			}

			k = Y(k);
			for (int j = 0; j < 4; ++j) {
				k = Z(k);
				if (!groupOf[toDen(k)]) {
					group.push_back(k);
					groupOf[toDen(k)] = groups.size() - 1;
				}
			}

			k = Y(k);
			for (int j = 0; j < 4; ++j) {
				k = X(k);
				if (!groupOf[toDen(k)]) {
					group.push_back(k);
					groupOf[toDen(k)] = groups.size() - 1;
				}
			}

			k = Y(k);
			k = X(k);
			for (int j = 0; j < 4; ++j) {
				k = Y(k);
				if (!groupOf[toDen(k)]) {
					group.push_back(k);
					groupOf[toDen(k)] = groups.size() - 1;
				}
			}
		}

		return groupOf;
	}
}