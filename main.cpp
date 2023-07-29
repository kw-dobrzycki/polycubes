#include <iostream>
#include "include/relative.h"
#include "include/groups.h"

int main() {

	auto n7 = read("../n7.txt");

	for (int i = 0; i < n7.size(); ++i) {
		for (int j = i + 1; j < n7.size(); ++j) {
			if (compareEncodings(n7[i].encode(), n7[j].encode())) {

				auto x = n7[i];
				auto y = n7[j];
			}
		}
	}


	return 0;
};
