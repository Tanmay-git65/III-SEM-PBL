#pragma once
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <queue>
#include <string>
#include <vector>
#include "maze.hpp"

struct SolveResult {
    bool found = false;
    std::vector<Coord> path;
    std::vector<Coord> expandedCells;
    double elapsedMicroseconds = 0.0;
    int steps() const { return path.empty() ? 0 : (int)path.size() - 1; }
    int cellsExpanded() const { return (int)expandedCells.size(); }
};

class MazeSolver {
public:
    virtual ~MazeSolver() = default;
    SolveResult solve(const Maze& maze) {
        auto startTime = std::chrono::steady_clock::now();
        SolveResult result = search(maze);
        auto endTime = std::chrono::steady_clock::now();
        result.elapsedMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }
    virtual std::string name() const = 0;
protected:
    virtual SolveResult search(const Maze& maze) = 0;

    static void buildPath(const std::vector<std::vector<Coord>>& parentOf, const Coord& start, const Coord& goal, SolveResult& result) {
        if (parentOf[goal.row][goal.col].isNone()) return;
        for (Coord step = goal; step != start; step = parentOf[step.row][step.col])
            result.path.push_back(step);
        result.path.push_back(start);
        std::reverse(result.path.begin(), result.path.end());
        result.found = true;
    }
};

class BFSSolver : public MazeSolver {
public:
    std::string name() const override { return "BFS"; }
protected:
    SolveResult search(const Maze& maze) override {
        Coord start = maze.start();
        Coord goal = maze.end();
        std::vector<std::vector<Coord>> parentOf(maze.rows(), std::vector<Coord>(maze.cols(), Coord::none()));
        std::queue<Coord> toVisit;
        SolveResult result;
        parentOf[start.row][start.col] = start;
        toVisit.push(start);
        while (!toVisit.empty()) {
            Coord current = toVisit.front();
            toVisit.pop();
            result.expandedCells.push_back(current);
            if (current == goal) break;
            for (Direction dir : allDirections) {
                if (!maze.canMove(current, dir)) continue;
                Coord next = maze.neighbour(current, dir);
                if (!parentOf[next.row][next.col].isNone()) continue;
                parentOf[next.row][next.col] = current;
                toVisit.push(next);
            }
        }
        buildPath(parentOf, start, goal, result);
        return result;
    }
};

struct OpenEntry {
    int estimatedTotal;
    int costSoFar;
    Coord pos;
    bool operator>(const OpenEntry& other) const {
        if (estimatedTotal != other.estimatedTotal) return estimatedTotal > other.estimatedTotal;
        return costSoFar < other.costSoFar;
    }
};

class AStarSolver : public MazeSolver {
public:
    std::string name() const override { return "A*"; }
protected:
    SolveResult search(const Maze& maze) override {
        Coord start = maze.start();
        Coord goal = maze.end();
        const int unreached = maze.cellCount() + 1;
        std::vector<std::vector<int>> bestCost(maze.rows(), std::vector<int>(maze.cols(), unreached));
        std::vector<std::vector<bool>> finalised(maze.rows(), std::vector<bool>(maze.cols(), false));
        std::vector<std::vector<Coord>> parentOf(maze.rows(), std::vector<Coord>(maze.cols(), Coord::none()));
        std::priority_queue<OpenEntry, std::vector<OpenEntry>, std::greater<OpenEntry>> openSet;
        SolveResult result;
        bestCost[start.row][start.col] = 0;
        parentOf[start.row][start.col] = start;
        openSet.push({manhattan(start, goal), 0, start});
        while (!openSet.empty()) {
            OpenEntry entry = openSet.top();
            openSet.pop();
            Coord current = entry.pos;
            if (finalised[current.row][current.col]) continue;
            finalised[current.row][current.col] = true;
            result.expandedCells.push_back(current);
            if (current == goal) break;
            for (Direction dir : allDirections) {
                if (!maze.canMove(current, dir)) continue;
                Coord next = maze.neighbour(current, dir);
                int newCost = entry.costSoFar + 1;
                if (finalised[next.row][next.col] || newCost >= bestCost[next.row][next.col]) continue;
                bestCost[next.row][next.col] = newCost;
                parentOf[next.row][next.col] = current;
                openSet.push({newCost + manhattan(next, goal), newCost, next});
            }
        }
        buildPath(parentOf, start, goal, result);
        return result;
    }
private:
    static int manhattan(const Coord& a, const Coord& b) { return std::abs(a.row - b.row) + std::abs(a.col - b.col); }
};
