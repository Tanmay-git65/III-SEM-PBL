#pragma once
#include <random>
#include <string>
#include "maze.hpp"
#include "generators.hpp"
#include "solvers.hpp"
#include "settings.hpp"

class MazeLabSession {
public:
    MazeLabSession() : rng(std::random_device{}()) { maze.init(DEFAULT_ROWS, DEFAULT_COLS); }
    Maze maze;
    DFSGenerator dfsGenerator;
    KruskalGenerator kruskalGenerator;
    BFSSolver bfsSolver;
    AStarSolver aStarSolver;
    std::string lastGeneratorName = "None";
    void generateWith(MazeGenerator& generator) {
        generator.generate(maze, rng);
        lastGeneratorName = generator.name();
    }
    void resize(int rows, int cols) {
        maze.init(rows, cols);
        lastGeneratorName = "None";
    }
private:
    std::mt19937 rng;
};
