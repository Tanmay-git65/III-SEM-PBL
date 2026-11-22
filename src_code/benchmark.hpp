#pragma once
#include <cstdio>
#include <ostream>
#include <random>
#include "generators.hpp"
#include "solvers.hpp"

inline void runBenchmark(std::ostream& out) {
    const int rows = 50;
    const int cols = 50;
    const int runs = 200;
    DFSGenerator dfs;
    KruskalGenerator kruskal;
    MazeGenerator* generators[] = {&dfs, &kruskal};
    BFSSolver bfs;
    AStarSolver aStar;
    out << "Benchmark: " << runs << " mazes of " << rows << " x " << cols << " per generator, seeds 0 to " << runs - 1 << "\n";
    for (MazeGenerator* generator : generators) {
        double pathTotal = 0, bfsExpanded = 0, aStarExpanded = 0, bfsTime = 0, aStarTime = 0;
        int mismatches = 0;
        for (int seed = 0; seed < runs; ++seed) {
            Maze maze;
            maze.init(rows, cols);
            std::mt19937 rng(seed);
            generator->generate(maze, rng);
            SolveResult bfsResult = bfs.solve(maze);
            SolveResult aStarResult = aStar.solve(maze);
            if (bfsResult.steps() != aStarResult.steps()) ++mismatches;
            pathTotal += bfsResult.steps();
            bfsExpanded += bfsResult.cellsExpanded();
            aStarExpanded += aStarResult.cellsExpanded();
            bfsTime += bfsResult.elapsedMicroseconds;
            aStarTime += aStarResult.elapsedMicroseconds;
        }
        char line[256];
        std::snprintf(line, sizeof(line), "%-20s path %.1f | expanded BFS %.1f, A* %.1f (%.1f%% fewer) | time BFS %.1f us, A* %.1f us | length mismatches %d\n",
            generator->name().c_str(), pathTotal / runs, bfsExpanded / runs, aStarExpanded / runs, 100.0 * (bfsExpanded - aStarExpanded) / bfsExpanded, bfsTime / runs, aStarTime / runs, mismatches);
        out << line;
    }
}
