#pragma once
#include <vector>
#include <utility>

class DisjointSet {
public:
    explicit DisjointSet(int size) : parent(size), rank(size, 0) {
        for (int i = 0; i < size; ++i) parent[i] = i;
    }

    int find(int element) {
        int root = element;
        while (parent[root] != root) root = parent[root];

        while (parent[element] != root) {
            int next = parent[element];
            parent[element] = root;
            element = next;
        }
        return root;
    }
    bool unite(int first, int second) {
        int rootA = find(first);
        int rootB = find(second);
        if (rootA == rootB) return false;

        if (rank[rootA] < rank[rootB]) std::swap(rootA, rootB);
        parent[rootB] = rootA;
        if (rank[rootA] == rank[rootB]) ++rank[rootA];
        return true;
    }
    bool connected(int first, int second) { return find(first) == find(second); }
private:
    std::vector<int> parent;
    std::vector<int> rank;
};
