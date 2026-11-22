#pragma once
#include <chrono>
#include "maze.hpp"

class Player {
public:
    void placeAt(const Coord& pos) {
        currentPos = pos;
        movesMade = 0;
        startTime = std::chrono::steady_clock::now();
    }
    bool move(const Maze& maze, Direction dir) {
        if (!maze.canMove(currentPos, dir)) return false;
        currentPos = maze.neighbour(currentPos, dir);
        ++movesMade;
        return true;
    }
    bool hasReached(const Coord& target) const { return currentPos == target; }
    Coord position() const { return currentPos; }
    int moveCount() const { return movesMade; }
    double secondsElapsed() const {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
    }
private:
    Coord currentPos;
    int movesMade = 0;
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
};
