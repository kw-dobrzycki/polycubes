#include <iostream>
#include "relative.h"

int main() {
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
