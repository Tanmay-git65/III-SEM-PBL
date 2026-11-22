#pragma once
#include <vector>

enum Direction { Up, Right, Down, Left };
const Direction allDirections[] = {Up, Right, Down, Left};

struct Coord {
    int row = 0;
    int col = 0;
    bool operator==(const Coord& other) const { return row == other.row && col == other.col; }
    bool operator!=(const Coord& other) const { return !(*this == other); }
    static Coord none() { return {-1, -1}; }
    bool isNone() const { return row < 0 || col < 0; }
};

struct Cell {
    bool wallTop = true;
    bool wallRight = true;
    bool wallBottom = true;
    bool wallLeft = true;
    bool visited = false;
    void reset() {
        wallTop = wallRight = wallBottom = wallLeft = true;
        visited = false;
    }
};

class Maze {
public:
    void init(int rows, int cols) {
        rowCount = rows;
        colCount = cols;
        grid.assign(rows, std::vector<Cell>(cols));
        startPos = {0, 0};
        endPos = {rows - 1, cols - 1};
        generated = false;
    }
    void resetWalls() {
        for (auto& row : grid)
            for (auto& cell : row)
                cell.reset();
        generated = false;
    }
    int rows() const { return rowCount; }
    int cols() const { return colCount; }
    int cellCount() const { return rowCount * colCount; }
    Coord start() const { return startPos; }
    Coord end() const { return endPos; }
    bool isGenerated() const { return generated; }
    void markGenerated() { generated = true; }
    bool inBounds(const Coord& pos) const {
        return pos.row >= 0 && pos.row < rowCount && pos.col >= 0 && pos.col < colCount;
    }
    Cell& cell(const Coord& pos) { return grid[pos.row][pos.col]; }
    const Cell& cell(const Coord& pos) const { return grid[pos.row][pos.col]; }
    Coord neighbour(const Coord& from, Direction dir) const {
        switch (dir) {
            case Up: return {from.row - 1, from.col};
            case Right: return {from.row, from.col + 1};
            case Down: return {from.row + 1, from.col};
            case Left: return {from.row, from.col - 1};
        }
        return from;
    }
    bool hasWall(const Coord& pos, Direction dir) const {
        const Cell& current = cell(pos);
        switch (dir) {
            case Up: return current.wallTop;
            case Right: return current.wallRight;
            case Down: return current.wallBottom;
            case Left: return current.wallLeft;
        }
        return true;
    }
    void openPassage(const Coord& from, Direction dir) {
        Coord to = neighbour(from, dir);
        if (!inBounds(from) || !inBounds(to)) return;
        Cell& fromCell = cell(from);
        Cell& toCell = cell(to);
        switch (dir) {
            case Up: fromCell.wallTop = false; toCell.wallBottom = false; break;
            case Right: fromCell.wallRight = false; toCell.wallLeft = false; break;
            case Down: fromCell.wallBottom = false; toCell.wallTop = false; break;
            case Left: fromCell.wallLeft = false; toCell.wallRight = false; break;
        }
    }
    bool canMove(const Coord& from, Direction dir) const {
        return inBounds(from) && !hasWall(from, dir) && inBounds(neighbour(from, dir));
    }
private:
    int rowCount = 0;
    int colCount = 0;
    std::vector<std::vector<Cell>> grid;
    Coord startPos;
    Coord endPos;
    bool generated = false;
};
