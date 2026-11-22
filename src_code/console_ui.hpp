#pragma once
#include <cctype>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "settings.hpp"
#include "session.hpp"
#include "player.hpp"
#include "validation.hpp"

class ConsoleRenderer {
public:
    void draw(const Maze& maze, const std::vector<Coord>& path = {}, const Coord& playerPos = Coord::none()) const {
        std::vector<std::vector<bool>> isOnPath(maze.rows(), std::vector<bool>(maze.cols(), false));
        for (const Coord& step : path)
            isOnPath[step.row][step.col] = true;

        const std::string wall(CONSOLE_CELL_WIDTH, '-');
        const std::string gap(CONSOLE_CELL_WIDTH, ' ');

        std::cout << '+';
        for (int col = 0; col < maze.cols(); col++)
            std::cout << wall << '+';
        std::cout << '\n';

        for (int row = 0; row < maze.rows(); row++) {
            std::cout << '|';
            for (int col = 0; col < maze.cols(); col++) {
                Coord here{row, col};
                std::cout << markFor(maze, here, isOnPath[row][col], playerPos);
                std::cout << (maze.hasWall(here, Right) ? '|' : ' ');
            }
            std::cout << '\n';

            std::cout << '+';
            for (int col = 0; col < maze.cols(); col++)
                std::cout << (maze.hasWall({row, col}, Down) ? wall : gap) << '+';
            std::cout << '\n';
        }

        std::cout << START_MARK << " = Start, " << EXIT_MARK << " = End, "
                  << PLAYER_MARK << " = Player, " << PATH_MARK << " = Path\n";
    }

private:
    std::string markFor(const Maze& maze, const Coord& here, bool onPath, const Coord& playerPos) const {
        if (here == playerPos)    return fitToCell(PLAYER_MARK);
        if (here == maze.end())   return fitToCell(EXIT_MARK);
        if (here == maze.start()) return fitToCell(START_MARK);
        if (onPath)               return fitToCell(PATH_MARK);
        return fitToCell(EMPTY_MARK);
    }

    static std::string fitToCell(std::string text) {
        text.resize(CONSOLE_CELL_WIDTH, ' ');
        return text;
    }
};

inline std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

inline int readNumber(const std::string& prompt, int min, int max) {
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

inline void waitForEnter() {
    readLine("Press Enter to continue...");
}

inline bool keyToDirection(char key, Direction& dir) {
    if (key == KEY_UP)    { dir = Up;    return true; }
    if (key == KEY_RIGHT) { dir = Right; return true; }
    if (key == KEY_DOWN)  { dir = Down;  return true; }
    if (key == KEY_LEFT)  { dir = Left;  return true; }
    return false;
}

inline std::string oneDecimal(double value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.1f", value);
    return buffer;
}

inline std::string formatMicroseconds(double value) {
    return oneDecimal(value) + " us";
}

inline std::string tableRow(const std::string& label, const std::string& bfs, const std::string& aStar) {
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "| %-16s | %10s | %10s |\n", label.c_str(), bfs.c_str(), aStar.c_str());
    return buffer;
}

class ConsoleApp {
public:
    explicit ConsoleApp(MazeLabSession& session) : session(session) {}
    void generateAndShow(MazeGenerator& generator) {
        session.generateWith(generator);
        std::cout << "\nMaze generated using " << generator.name() << ".\n";
        renderer.draw(session.maze);
        waitForEnter();
    }
    void solveAndShow(MazeSolver& solver) {
        if (!mazeIsReady()) return;
        SolveResult result = solver.solve(session.maze);
        if (!result.found) {
            std::cout << "\n" << solver.name() << " found no path to the exit.\n";
        } else {
            std::cout << "\n" << solver.name() << " found a path of " << result.steps() << " steps.\n";
            std::cout << "Cells expanded: " << result.cellsExpanded() << ", time: " << formatMicroseconds(result.elapsedMicroseconds) << "\n";
            renderer.draw(session.maze, result.path);
        }
        waitForEnter();
    }
    void compareSolvers() {
        if (!mazeIsReady()) return;
        SolveResult bfs = session.bfsSolver.solve(session.maze);
        SolveResult aStar = session.aStarSolver.solve(session.maze);
        const std::string line = "+------------------+------------+------------+\n";
        std::cout << '\n' << line;
        std::cout << tableRow("Metric", "BFS", "A*");
        std::cout << line;
        std::cout << tableRow("Path found", bfs.found ? "Yes" : "No", aStar.found ? "Yes" : "No");
        std::cout << tableRow("Path length", std::to_string(bfs.steps()), std::to_string(aStar.steps()));
        std::cout << tableRow("Cells expanded", std::to_string(bfs.cellsExpanded()), std::to_string(aStar.cellsExpanded()));
        std::cout << tableRow("Time", formatMicroseconds(bfs.elapsedMicroseconds), formatMicroseconds(aStar.elapsedMicroseconds));
        std::cout << line;
        if (bfs.steps() == aStar.steps())
            std::cout << "Both found a shortest path of the same length.\n";
        int saved = bfs.cellsExpanded() - aStar.cellsExpanded();
        std::cout << "A* expanded " << saved << " fewer cells than BFS.\n";
        waitForEnter();
    }
    void playMaze() {
        if (!mazeIsReady()) return;
        const Maze& maze = session.maze;
        Player player;
        player.placeAt(maze.start());
        std::cout << "\nControls: " << KEY_UP << " = Up, " << KEY_LEFT << " = Left, "
                  << KEY_DOWN << " = Down, " << KEY_RIGHT << " = Right, " << KEY_QUIT << " = Quit\n";
        bool quit = false;
        while (!quit && !player.hasReached(maze.end())) {
            renderer.draw(maze, {}, player.position());
            std::cout << "Moves: " << player.moveCount() << '\n';
            std::string input = readLine("Your move: ");
            if (!std::cin) return;
            if (input.empty()) continue;
            char key = (char)std::toupper((unsigned char)input[0]);
            Direction dir = Up;
            if (key == KEY_QUIT)
                quit = true;
            else if (!keyToDirection(key, dir))
                std::cout << "Unknown key. Use the keys listed above.\n";
            else if (!player.move(maze, dir))
                std::cout << "There is a wall in that direction.\n";
        }
        if (player.hasReached(maze.end())) {
            renderer.draw(maze, {}, player.position());
            int shortest = session.bfsSolver.solve(maze).steps();
            int efficiency = (int)(100.0 * shortest / player.moveCount() + 0.5);
            std::cout << "You reached the exit in " << player.moveCount() << " moves and "
                      << oneDecimal(player.secondsElapsed()) << " seconds.\n";
            std::cout << "Shortest path: " << shortest << " moves. Efficiency: " << efficiency << "%\n";
        } else {
            std::cout << "Game ended after " << player.moveCount() << " moves.\n";
        }
        waitForEnter();
    }
    void changeSize() {
        std::string range = "(" + std::to_string(MIN_SIZE) + "-" + std::to_string(MAX_SIZE) + "): ";
        int rows = readNumber("Rows " + range, MIN_SIZE, MAX_SIZE);
        int cols = readNumber("Columns " + range, MIN_SIZE, MAX_SIZE);
        session.resize(rows, cols);
        std::cout << "Size set to " << rows << " x " << cols << ". Generate a new maze.\n";
        waitForEnter();
    }
    void runTests() {
        ValidationSuite suite;
        int passed = 0;
        std::vector<TestOutcome> outcomes = suite.runAll();
        std::cout << '\n';
        for (const TestOutcome& outcome : outcomes) {
            std::cout << (outcome.passed ? "[PASS] " : "[FAIL] ") << outcome.name << '\n';
            if (outcome.passed) ++passed;
        }
        std::cout << "Total: " << passed << " passed, " << outcomes.size() - passed << " failed\n";
        waitForEnter();
    }
    void showMenu() const {
        std::cout << '\n' << CONSOLE_TITLE << '\n';
        std::cout << "Maze size: " << session.maze.rows() << " x " << session.maze.cols() << '\n';
        std::cout << "Generator: " << session.lastGeneratorName << '\n';
        std::cout << "Maze status: " << (session.maze.isGenerated() ? "Generated" : "Not generated") << '\n';
        std::cout << MENU_DFS << '\n';
        std::cout << MENU_KRUSKAL << '\n';
        std::cout << MENU_PLAY << '\n';
        std::cout << MENU_BFS << '\n';
        std::cout << MENU_ASTAR << '\n';
        std::cout << MENU_COMPARE << '\n';
        std::cout << MENU_SIZE << '\n';
        std::cout << MENU_TESTS << '\n';
        std::cout << MENU_GUI << '\n';
        std::cout << MENU_EXIT << '\n';
    }
private:
    MazeLabSession& session;
    ConsoleRenderer renderer;
    bool mazeIsReady() {
        if (session.maze.isGenerated()) return true;
        std::cout << "\nNo maze yet. Generate one first (option 1 or 2).\n";
        waitForEnter();
        return false;
    }
};
