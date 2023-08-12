#include <iostream>
#include "relative.h"

int main() {

	auto x = Tet{17, {
			{-1, 0, -1},
			{-1, 0, 1},
			{-1, 1, -1},
			{-1, 1, 0},
			{-1, 1, 1},
			{0, 1, -1},
			{0, 1, 1},
			{1, 0, -1},
			{1, 0, 1},
			{1, 1, -1},
			{1, 1, 0},
			{1, 1, 1},
			{2, 0, -1},
			{2, 0, 1},
			{2, 1, -1},
			{2, 1, 0},
			{2, 1, 1},
	}}.encodeSelf();


	auto u = generateCompl(7);
	auto y = generate(7);

	int i = 0, j = 0;
	for (auto p : y){
		bool in = false;
		for (auto x : u){
			if (fullCompare(p, x)){
				in = true;
			}
			j++;
		}
		if (! in){
			auto l = 0;
		}
		i++;
	}
}
