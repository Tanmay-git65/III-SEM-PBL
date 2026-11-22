#pragma once
#include <queue>
#include <random>
#include <string>
#include <vector>
#include "maze.hpp"
#include "disjoint_set.hpp"
#include "generators.hpp"
#include "solvers.hpp"
#include "player.hpp"

struct TestOutcome {
    std::string name;
    bool passed;
};

class ValidationSuite {
public:
    std::vector<TestOutcome> runAll() {
        std::vector<TestOutcome> outcomes;
        outcomes.push_back({"T1 DFS maze is complete and connected", generatorBuildsValidMaze(dfs)});
        outcomes.push_back({"T2 Kruskal maze is complete and connected", generatorBuildsValidMaze(kruskal)});
        outcomes.push_back({"T3 Perfect maze has cells - 1 passages", bothGeneratorsMakePerfectMazes()});
        outcomes.push_back({"T4 Shared walls agree on both sides", bothGeneratorsKeepWallsConsistent()});
        outcomes.push_back({"T5 BFS path is valid", solverReturnsValidPaths(bfs)});
        outcomes.push_back({"T6 A* path is valid", solverReturnsValidPaths(aStar)});
        outcomes.push_back({"T7 BFS and A* path lengths match", solversAgreeOnLength()});
        outcomes.push_back({"T8 Unreachable exit is reported", solversReportNoPath()});
        outcomes.push_back({"T9 Player is blocked by walls", playerRespectsWalls()});
        outcomes.push_back({"T10 Smallest and narrow sizes work", edgeSizesWork()});
        outcomes.push_back({"T11 Outer boundary stays closed", outerBoundaryStaysClosed()});
        outcomes.push_back({"T12 Disjoint set find and unite", disjointSetWorks()});
        outcomes.push_back({"T13 Solvers do not change the maze", solversAreReadOnly()});
        return outcomes;
    }
private:
    DFSGenerator dfs;
    KruskalGenerator kruskal;
    BFSSolver bfs;
    AStarSolver aStar;
    const std::vector<std::pair<int, int>> sizes = {{2, 2}, {5, 8}, {10, 10}, {25, 40}};
    const std::vector<unsigned> seeds = {1, 7, 42, 2026};

    static bool everyCellReachable(const Maze& maze) {
        std::vector<std::vector<bool>> seen(maze.rows(), std::vector<bool>(maze.cols(), false));
        std::queue<Coord> toVisit;
        toVisit.push(maze.start());
        seen[0][0] = true;
        int reached = 1;
        while (!toVisit.empty()) {
            Coord current = toVisit.front();
            toVisit.pop();
            for (Direction dir : allDirections) {
                if (!maze.canMove(current, dir)) continue;
                Coord next = maze.neighbour(current, dir);
                if (seen[next.row][next.col]) continue;
                seen[next.row][next.col] = true;
                ++reached;
                toVisit.push(next);
            }
        }
        return reached == maze.cellCount();
    }
    static bool wallsConsistent(const Maze& maze) {
        for (int row = 0; row < maze.rows(); ++row)
            for (int col = 0; col < maze.cols(); ++col) {
                Coord here{row, col};
                if (col + 1 < maze.cols() && maze.hasWall(here, Right) != maze.hasWall({row, col + 1}, Left)) return false;
                if (row + 1 < maze.rows() && maze.hasWall(here, Down) != maze.hasWall({row + 1, col}, Up)) return false;
            }
        return true;
    }
    static bool outerBoundaryClosed(const Maze& maze) {
        for (int col = 0; col < maze.cols(); ++col)
            if (!maze.hasWall({0, col}, Up) || !maze.hasWall({maze.rows() - 1, col}, Down)) return false;
        for (int row = 0; row < maze.rows(); ++row)
            if (!maze.hasWall({row, 0}, Left) || !maze.hasWall({row, maze.cols() - 1}, Right)) return false;
        return true;
    }
    static int openPassageCount(const Maze& maze) {
        int count = 0;
        for (int row = 0; row < maze.rows(); ++row)
            for (int col = 0; col < maze.cols(); ++col) {
                if (maze.canMove({row, col}, Right)) ++count;
                if (maze.canMove({row, col}, Down)) ++count;
            }
        return count;
    }
    static bool pathIsValid(const Maze& maze, const SolveResult& result) {
        if (!result.found || result.path.empty()) return false;
        if (result.path.front() != maze.start() || result.path.back() != maze.end()) return false;
        for (size_t i = 1; i < result.path.size(); ++i) {
            bool connected = false;
            for (Direction dir : allDirections)
                if (maze.canMove(result.path[i - 1], dir) && maze.neighbour(result.path[i - 1], dir) == result.path[i]) connected = true;
            if (!connected) return false;
        }
        return true;
    }
    template <typename Check>
    bool forEveryMaze(MazeGenerator& generator, Check check) {
        for (auto size : sizes)
            for (unsigned seed : seeds) {
                Maze maze;
                maze.init(size.first, size.second);
                std::mt19937 rng(seed);
                generator.generate(maze, rng);
                if (!check(maze)) return false;
            }
        return true;
    }
    bool generatorBuildsValidMaze(MazeGenerator& generator) {
        return forEveryMaze(generator, [](const Maze& maze) {
            return maze.isGenerated() && everyCellReachable(maze) && wallsConsistent(maze);
        });
    }
    bool bothGeneratorsMakePerfectMazes() {
        auto perfect = [](const Maze& maze) { return openPassageCount(maze) == maze.cellCount() - 1; };
        return forEveryMaze(dfs, perfect) && forEveryMaze(kruskal, perfect);
    }
    bool bothGeneratorsKeepWallsConsistent() {
        return forEveryMaze(dfs, wallsConsistent) && forEveryMaze(kruskal, wallsConsistent);
    }
    bool solverReturnsValidPaths(MazeSolver& solver) {
        auto check = [&solver](const Maze& maze) { return pathIsValid(maze, solver.solve(maze)); };
        return forEveryMaze(dfs, check) && forEveryMaze(kruskal, check);
    }
    bool solversAgreeOnLength() {
        auto check = [this](const Maze& maze) { return bfs.solve(maze).steps() == aStar.solve(maze).steps(); };
        return forEveryMaze(dfs, check) && forEveryMaze(kruskal, check);
    }
    bool solversReportNoPath() {
        Maze maze;
        maze.init(6, 6);
        return !bfs.solve(maze).found && !aStar.solve(maze).found;
    }
    bool playerRespectsWalls() {
        Maze maze;
        maze.init(3, 3);
        maze.openPassage({0, 0}, Right);
        Player player;
        player.placeAt(maze.start());
        bool blockedDown = !player.move(maze, Down);
        bool blockedOutside = !player.move(maze, Up) && !player.move(maze, Left);
        bool movedRight = player.move(maze, Right) && player.position() == Coord{0, 1};
        return blockedDown && blockedOutside && movedRight && player.moveCount() == 1;
    }
    bool edgeSizesWork() {
        const std::vector<std::pair<int, int>> edgeSizes = {{2, 2}, {2, 10}, {10, 2}, {2, 3}, {3, 2}};
        for (auto size : edgeSizes) {
            Maze maze;
            maze.init(size.first, size.second);
            std::mt19937 rng(5);
            dfs.generate(maze, rng);
            if (!everyCellReachable(maze) || !pathIsValid(maze, bfs.solve(maze))) return false;
            kruskal.generate(maze, rng);
            if (!everyCellReachable(maze) || !pathIsValid(maze, aStar.solve(maze))) return false;
        }
        return true;
    }
    bool outerBoundaryStaysClosed() {
        return forEveryMaze(dfs, outerBoundaryClosed) && forEveryMaze(kruskal, outerBoundaryClosed);
    }
    bool disjointSetWorks() {
        DisjointSet sets(6);
        bool firstUnion = sets.unite(0, 1) && sets.unite(1, 2);
        bool repeatedUnionRejected = !sets.unite(0, 2);
        bool separateSetsStaySeparate = !sets.connected(0, 3) && sets.connected(0, 2);
        sets.unite(3, 4);
        sets.unite(2, 4);
        return firstUnion && repeatedUnionRejected && separateSetsStaySeparate && sets.connected(0, 3) && !sets.connected(0, 5);
    }
    bool solversAreReadOnly() {
        return forEveryMaze(kruskal, [this](const Maze& maze) {
            Maze copy = maze;
            bfs.solve(copy);
            aStar.solve(copy);
            for (int row = 0; row < maze.rows(); ++row)
                for (int col = 0; col < maze.cols(); ++col)
                    for (Direction dir : allDirections)
                        if (copy.hasWall({row, col}, dir) != maze.hasWall({row, col}, dir)) return false;
            return true;
        });
    }
};
