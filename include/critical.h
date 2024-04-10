//
// Created by Chris on 10/04/2024.
//

#ifndef TETRIS_CRITICAL_H
#define TETRIS_CRITICAL_H

#include "Tet.h"

template<unsigned n>
void APUtil(const Tet<n>& tet, int u, bool visited[],
			int disc[], int low[], int& time, int parent,
			bool isAP[]) {
	// Count of children in DFS Tree
	int children = 0;

	// Mark the current node as visited
	visited[u] = true;

	// Initialize discovery time and low value
	disc[u] = low[u] = ++time;

	// Go through all vertices adjacent to this
	for (int i = 0; i < n; ++i) {
		Pos d = tet.units[u] - tet.units[i];
		if (!d.isUnit()) continue;

		// If v is not visited yet, then make it a child of u
		// in DFS tree and recur for it
		if (!visited[i]) {
			children++;
			APUtil(tet, i, visited, disc, low, time, u, isAP);

			// Check if the subtree rooted with i has
			// a connection to one of the ancestors of u
			low[u] = std::min(low[u], low[i]);

			// If u is not root and low value of one of
			// its child is more than discovery value of u.
			if (parent != -1 && low[i] >= disc[u])
				isAP[u] = true;
		}
			// Update low value of u for parent function calls.
		else if (i != parent)
			low[u] = std::min(low[u], disc[i]);

	}

	// If u is root of DFS tree and has two or more children.
	if (parent == -1 && children > 1)
		isAP[u] = true;
}

template<unsigned n>
std::vector<size_t> AP(const Tet<n>& tet) {
	int disc[n]{};
	int low[n]{};
	bool isAP[n]{};
	bool visited[n]{};
	int time = 0, par = -1;

	// Adding this loop so that the
	// code works even if we are given
	// disconnected graph
	for (int u = 0; u < n; u++)
		if (!visited[u])
			APUtil(tet, u, visited, disc, low,
				   time, par, isAP);

	std::vector<size_t> r;
	r.reserve(n);
	for (int i = 0; i < n; ++i) {
		if (!isAP[i]) r.push_back(i);
	}
	return r;
}

#endif //TETRIS_CRITICAL_H
