#include <iostream>
#include <vector>
#include <stack>
#include <queue>
#include <string>
#include <random>
#include <algorithm>
#include <cctype>
#include <stdexcept>

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

struct SolveResult {
    bool found = false;
    std::vector<Coord> path;
    int steps() const { return path.empty() ? 0 : (int)path.size() - 1; }
};

class MazeGenerator {
public:
    virtual ~MazeGenerator() = default;
    virtual void generate(Maze& maze, std::mt19937& rng) = 0;
    virtual std::string name() const = 0;
};

class MazeSolver {
public:
    virtual ~MazeSolver() = default;
    virtual SolveResult solve(const Maze& maze) = 0;
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

class BFSSolver : public MazeSolver {
public:
    std::string name() const override { return "BFS"; }
    SolveResult solve(const Maze& maze) override {
        Coord start = maze.start();
        Coord goal = maze.end();
        std::vector<std::vector<Coord>> parentOf(maze.rows(), std::vector<Coord>(maze.cols(), Coord::none()));
        std::queue<Coord> toVisit;
        parentOf[start.row][start.col] = start;
        toVisit.push(start);
        while (!toVisit.empty()) {
            Coord current = toVisit.front();
            toVisit.pop();
            if (current == goal) break;
            for (Direction dir : allDirections) {
                if (!maze.canMove(current, dir)) continue;
                Coord next = maze.neighbour(current, dir);
                if (!parentOf[next.row][next.col].isNone()) continue;
                parentOf[next.row][next.col] = current;
                toVisit.push(next);
            }
        }
        SolveResult result;
        if (parentOf[goal.row][goal.col].isNone()) return result;
        for (Coord step = goal; step != start; step = parentOf[step.row][step.col])
            result.path.push_back(step);
        result.path.push_back(start);
        std::reverse(result.path.begin(), result.path.end());
        result.found = true;
        return result;
    }
};

class Player {
public:
    void placeAt(const Coord& pos) {
        currentPos = pos;
        movesMade = 0;
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
private:
    Coord currentPos;
    int movesMade = 0;
};

class ConsoleRenderer {
public:
    void draw(const Maze& maze, const std::vector<Coord>& path = {}, const Coord& playerPos = Coord::none()) const {
        std::vector<std::vector<bool>> isOnPath(maze.rows(), std::vector<bool>(maze.cols(), false));
        for (const Coord& step : path)
            isOnPath[step.row][step.col] = true;
        std::cout << '+';
        for (int col = 0; col < maze.cols(); ++col)
            std::cout << "--+";
        std::cout << '\n';
        for (int row = 0; row < maze.rows(); ++row) {
            std::cout << '|';
            for (int col = 0; col < maze.cols(); ++col) {
                Coord here{row, col};
                char symbol = symbolFor(maze, here, isOnPath[row][col], playerPos);
                std::cout << symbol << symbol << (maze.hasWall(here, Right) ? '|' : ' ');
            }
            std::cout << "\n+";
            for (int col = 0; col < maze.cols(); ++col)
                std::cout << (maze.hasWall({row, col}, Down) ? "--" : "  ") << '+';
            std::cout << '\n';
        }
        std::cout << "S = Start, E = End, P = Player, * = Path\n";
    }
private:
    char symbolFor(const Maze& maze, const Coord& here, bool onPath, const Coord& playerPos) const {
        if (here == playerPos) return 'P';
        if (here == maze.end()) return 'E';
        if (here == maze.start()) return 'S';
        if (onPath) return '*';
        return ' ';
    }
};

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

int readNumber(const std::string& prompt, int min, int max) {
    while (true) {
        std::string line = readLine(prompt);
        if (!std::cin) return 0;
        try {
            std::size_t charsRead = 0;
            int value = std::stoi(line, &charsRead);
            if (charsRead == line.size() && value >= min && value <= max) return value;
        } catch (const std::exception&) {
        }
        std::cout << "Please enter a number from " << min << " to " << max << ".\n";
    }
}

void waitForEnter() {
    readLine("Press Enter to continue...");
}

bool keyToDirection(char key, Direction& dir) {
    switch (key) {
        case 'W': dir = Up; return true;
        case 'D': dir = Right; return true;
        case 'S': dir = Down; return true;
        case 'A': dir = Left; return true;
    }
    return false;
}

bool mazeIsReady(const Maze& maze) {
    if (maze.isGenerated()) return true;
    std::cout << "\nNo maze yet. Generate one first (option 1).\n";
    waitForEnter();
    return false;
}

void generateAndShow(MazeGenerator& generator, Maze& maze, std::mt19937& rng, const ConsoleRenderer& renderer) {
    generator.generate(maze, rng);
    std::cout << "\nMaze generated using " << generator.name() << ".\n";
    renderer.draw(maze);
    waitForEnter();
}

void solveAndShow(MazeSolver& solver, const Maze& maze, const ConsoleRenderer& renderer) {
    SolveResult result = solver.solve(maze);
    if (!result.found) {
        std::cout << "\n" << solver.name() << " found no path to the exit.\n";
    } else {
        std::cout << "\n" << solver.name() << " found a path of " << result.steps() << " steps.\n";
        renderer.draw(maze, result.path);
    }
    waitForEnter();
}

void playMaze(const Maze& maze, const ConsoleRenderer& renderer) {
    Player player;
    player.placeAt(maze.start());
    std::cout << "\nControls: W = Up, A = Left, S = Down, D = Right, Q = Quit\n";
    bool quit = false;
    while (!quit && !player.hasReached(maze.end())) {
        renderer.draw(maze, {}, player.position());
        std::cout << "Moves: " << player.moveCount() << '\n';
        std::string input = readLine("Your move: ");
        if (!std::cin) return;
        if (input.empty()) continue;
        char key = (char)std::toupper((unsigned char)input[0]);
        Direction dir = Up;
        if (key == 'Q')
            quit = true;
        else if (!keyToDirection(key, dir))
            std::cout << "Unknown key. Use W, A, S, D or Q.\n";
        else if (!player.move(maze, dir))
            std::cout << "There is a wall in that direction.\n";
    }
    if (player.hasReached(maze.end())) {
        renderer.draw(maze, {}, player.position());
        std::cout << "You reached the exit in " << player.moveCount() << " moves.\n";
    } else {
        std::cout << "Game ended after " << player.moveCount() << " moves.\n";
    }
    waitForEnter();
}

void showMenu(const Maze& maze) {
    std::cout << "\n========== MAZE LAB (Phase II) ==========\n";
    std::cout << "Maze size: " << maze.rows() << " x " << maze.cols() << '\n';
    std::cout << "Maze status: " << (maze.isGenerated() ? "Generated" : "Not generated") << '\n';
    std::cout << "1. Generate maze (Randomized DFS)\n";
    std::cout << "2. Play maze\n";
    std::cout << "3. Solve maze with BFS\n";
    std::cout << "4. Change maze size\n";
    std::cout << "0. Exit\n";
}

void runMenu() {
    Maze maze;
    maze.init(10, 10);
    DFSGenerator dfsGenerator;
    BFSSolver bfsSolver;
    ConsoleRenderer renderer;
    std::mt19937 rng(std::random_device{}());
    while (true) {
        showMenu(maze);
        int choice = readNumber("Enter your choice: ", 0, 4);
        if (!std::cin || choice == 0) {
            std::cout << "Goodbye!\n";
            return;
        }
        switch (choice) {
            case 1:
                generateAndShow(dfsGenerator, maze, rng, renderer);
                break;
            case 2:
                if (mazeIsReady(maze)) playMaze(maze, renderer);
                break;
            case 3:
                if (mazeIsReady(maze)) solveAndShow(bfsSolver, maze, renderer);
                break;
            case 4: {
                int rows = readNumber("Rows (2-50): ", 2, 50);
                int cols = readNumber("Columns (2-50): ", 2, 50);
                maze.init(rows, cols);
                std::cout << "Size set to " << rows << " x " << cols << ". Generate a new maze.\n";
                waitForEnter();
                break;
            }
        }
    }
}

int main() {
    runMenu();
    return 0;
}
