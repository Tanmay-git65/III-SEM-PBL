#pragma once
#include <random>
#include <stack>
#include <string>
#include <vector>
#include <algorithm>
#include "maze.hpp"
#include "disjoint_set.hpp"

class MazeGenerator {
public:
    virtual ~MazeGenerator() = default;
    virtual void generate(Maze& maze, std::mt19937& rng) = 0;
    virtual std::string name() const = 0;
};

class DFSGenerator : public MazeGenerator {
public:
    std::string name() const override { return "Randomized DFS"; }
    void generate(Maze& maze, std::mt19937& rng) override {
        maze.resetWalls();
        std::stack<Coord> backtrackStack;
        maze.cell(maze.start()).visited = true;
        backtrackStack.push(maze.start());
        while (!backtrackStack.empty()) {
            Coord current = backtrackStack.top();
            std::vector<Direction> unvisitedDirections;
            for (Direction dir : allDirections) {
                Coord next = maze.neighbour(current, dir);
                if (maze.inBounds(next) && !maze.cell(next).visited)
                    unvisitedDirections.push_back(dir);
            }
            if (unvisitedDirections.empty()) {
                backtrackStack.pop();
                continue;
            }
            std::uniform_int_distribution<int> pickIndex(0, (int)unvisitedDirections.size() - 1);
            Direction chosen = unvisitedDirections[pickIndex(rng)];
            Coord next = maze.neighbour(current, chosen);
            maze.openPassage(current, chosen);
            maze.cell(next).visited = true;
            backtrackStack.push(next);
        }
        maze.markGenerated();
    }
};

struct WallEdge {
    Coord from;
    Direction dir;
};

class KruskalGenerator : public MazeGenerator {
public:
    std::string name() const override { return "Kruskal's Algorithm"; }
    void generate(Maze& maze, std::mt19937& rng) override {
        maze.resetWalls();

        std::vector<WallEdge> walls;
        for (int row = 0; row < maze.rows(); ++row) {
            for (int col = 0; col < maze.cols(); ++col) {
                if (col + 1 < maze.cols()) walls.push_back({{row, col}, Right});
                if (row + 1 < maze.rows()) walls.push_back({{row, col}, Down});
            }
        }
        std::shuffle(walls.begin(), walls.end(), rng);
        DisjointSet regions(maze.cellCount());
        for (const WallEdge& wall : walls) {
            Coord to = maze.neighbour(wall.from, wall.dir);
            if (regions.unite(indexOf(maze, wall.from), indexOf(maze, to)))
                maze.openPassage(wall.from, wall.dir);
        }
        maze.markGenerated();
    }
private:
    static int indexOf(const Maze& maze, const Coord& pos) { return pos.row * maze.cols() + pos.col; }
};
