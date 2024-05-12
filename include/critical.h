//
// Created by Chris on 10/04/2024.
//

#ifndef TETRIS_CRITICAL_H
#define TETRIS_CRITICAL_H

#include "Tet.h"

template<unsigned n>
bool labelCut(const Tet<n>& tet, size_t r, const unsigned endpoints[6], unsigned edges) {
	//remove a vertex, label all connected components
	auto cut = tet.remove(r);

	size_t q[n - 1]{};
	size_t p1 = 0;
	size_t p2 = 1;

	size_t labels[n - 1]{};

	bool seen[n - 1]{};

	int maxLabel = 0;
	for (int i = 0; i < n - 1; ++i) {
		if (seen[i]) continue;
		maxLabel++;
		seen[i] = true;
		labels[i] = maxLabel;
		p1 = 0;
		p2 = 1;
		q[0] = i;

		while (p1 < p2) {
			size_t j = q[p1++];
			for (int k = 0; k < n - 1; ++k) {
				if ((cut.units[j] - cut.units[k]).isUnit() && !seen[k]) {
					seen[k] = true;
					labels[k] = maxLabel;
					q[p2++] = k;
				}
			}
		}
	}

	//count the unique labels
	unsigned connectingComponents = 0;
	bool seenLabels[n]{};
	for (int i = 0; i < edges; ++i) {
		if (endpoints[i] == r) continue; //this endpoint doesn't connect any components
		int j = endpoints[i] - (endpoints[i] > r);
		//all will have been seen, seen is now used to count uniques
		if (!seenLabels[labels[j]]) {
			seenLabels[labels[j]] = true;
			connectingComponents++;
		}
	}

	//if the new vertex is incident to less than the number of components connected by the removed vertex,
	//some components will remain disconnected when the new vertex is added, so the removed vertex is
	//critical
	return connectingComponents < maxLabel;
}

template<unsigned n>
void findCriticalRecursive(const Tet<n>& tet, const Pos& p, bool critical[n + 1], bool recurse = true) {
	if (recurse) findCriticalRecursive(tet.remove(n - 1), tet.units[n - 1], critical); //todo removing is inefficient

	unsigned endpoints[6]{};
	unsigned neighbours = 0;
	for (int i = 0; i < n; ++i) {
		if ((tet.units[i] - p).isUnit()) {
			endpoints[neighbours++] = i;
		}
	}

	//check if p has only one neighbour, then it must be crit
	if (neighbours == 1) {
		critical[endpoints[0]] = true;
		critical[n] = false;
		return;
	}

	for (int i = 0; i < n; ++i) {
		if (!critical[i]) continue;
		critical[i] = labelCut(tet, i, endpoints, neighbours);
	}
}

template<>
void findCriticalRecursive(const Tet<1>&, const Pos&, bool critical[2], bool) {
	critical[0] = false;
	critical[1] = false;
}


#endif //TETRIS_CRITICAL_H
