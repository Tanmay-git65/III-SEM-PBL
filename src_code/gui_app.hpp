#pragma once
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>
#include "settings.hpp"
#include "session.hpp"
#include "player.hpp"
#include "validation.hpp"

class Canvas {
public:
    virtual ~Canvas() = default;
    virtual void fillRect(int x, int y, int width, int height, Color color) = 0;
    virtual void fillCircle(int centerX, int centerY, int radius, Color color) = 0;
    virtual void drawText(int x, int y, const std::string& text, Color color) = 0;
    virtual int textWidth(const std::string& text) = 0;
    virtual int textHeight() = 0;

    void outlineRect(int x, int y, int width, int height, int thickness, Color color) {
        fillRect(x, y, width, thickness, color);
        fillRect(x, y + height - thickness, width, thickness, color);
        fillRect(x, y, thickness, height, color);
        fillRect(x + width - thickness, y, thickness, height, color);
    }
};

enum class ButtonId { GenerateDFS, GenerateKruskal, Play, SolveBFS, SolveAStar, Compare, RowsDown, RowsUp, ColsDown, ColsUp, RunTests };

struct GuiButton {
    ButtonId id;
    int x, y, width, height;
    std::string label;

    bool contains(int px, int py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

enum class Overlay { Plain, SingleSolution, Comparison };

class GuiApp {
public:
    explicit GuiApp(MazeLabSession& session) : session(session) {
        const int left = sidebarX + 16;
        const int width = SIDEBAR_WIDTH - 32;
        const int gap = 40;

        int y = 78;
        addButton(ButtonId::GenerateDFS,     left, y,         width, BUTTON_DFS);
        addButton(ButtonId::GenerateKruskal, left, y += gap,  width, BUTTON_KRUSKAL);
        addButton(ButtonId::Play,            left, y += gap,  width, BUTTON_PLAY);
        addButton(ButtonId::SolveBFS,        left, y += gap,  width, BUTTON_BFS);
        addButton(ButtonId::SolveAStar,      left, y += gap,  width, BUTTON_ASTAR);
        addButton(ButtonId::Compare,         left, y += gap,  width, BUTTON_COMPARE);

        rowsY = y += 44;
        colsY = y += 34;
        sizeButtonsX = left + width - 112;
        addSmallButton(ButtonId::RowsDown, sizeButtonsX,      rowsY, "-");
        addSmallButton(ButtonId::RowsUp,   sizeButtonsX + 84, rowsY, "+");
        addSmallButton(ButtonId::ColsDown, sizeButtonsX,      colsY, "-");
        addSmallButton(ButtonId::ColsUp,   sizeButtonsX + 84, colsY, "+");

        addButton(ButtonId::RunTests, left, y += gap, width, BUTTON_TESTS);
        infoTop = y + 44;

        if (session.maze.isGenerated())
            info = {"Showing the current maze.", "Choose an option on the right."};
        else
            info = {"No maze yet.", "Press 1 or 2 to generate one."};
    }

    void draw(Canvas& canvas) const {
        canvas.fillRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, BACKGROUND_COLOR);
        drawMaze(canvas);
        drawSidebar(canvas);
    }

    void onClick(int x, int y) {
        for (const GuiButton& button : buttons) {
            if (button.contains(x, y)) {
                runAction(button.id);
                return;
            }
        }
    }

    void onArrow(Direction dir) {
        if (playing) movePlayer(dir);
    }

    void onKeyChar(char typed) {
        char key = (char)std::toupper((unsigned char)typed);
        const char escape = 27;

        if (playing) {
            if (key == KEY_UP)    { movePlayer(Up);    return; }
            if (key == KEY_LEFT)  { movePlayer(Left);  return; }
            if (key == KEY_DOWN)  { movePlayer(Down);  return; }
            if (key == KEY_RIGHT) { movePlayer(Right); return; }
            if (key == KEY_QUIT || key == escape) { runAction(ButtonId::Play); return; }
        }

        switch (key) {
            case '1': runAction(ButtonId::GenerateDFS); break;
            case '2': runAction(ButtonId::GenerateKruskal); break;
            case '3': runAction(ButtonId::Play); break;
            case '4': runAction(ButtonId::SolveBFS); break;
            case '5': runAction(ButtonId::SolveAStar); break;
            case '6': runAction(ButtonId::Compare); break;
            case '7': runAction(ButtonId::RunTests); break;
        }
    }

    void runAction(ButtonId id) {
        if (id != ButtonId::Play) playing = false;

        int rows = session.maze.rows();
        int cols = session.maze.cols();

        switch (id) {
            case ButtonId::GenerateDFS:     generate(session.dfsGenerator); break;
            case ButtonId::GenerateKruskal: generate(session.kruskalGenerator); break;
            case ButtonId::Play:            togglePlay(); break;
            case ButtonId::SolveBFS:        solve(session.bfsSolver); break;
            case ButtonId::SolveAStar:      solve(session.aStarSolver); break;
            case ButtonId::Compare:         compare(); break;
            case ButtonId::RowsDown:        resize(sizeDown(rows), cols); break;
            case ButtonId::RowsUp:          resize(sizeUp(rows), cols); break;
            case ButtonId::ColsDown:        resize(rows, sizeDown(cols)); break;
            case ButtonId::ColsUp:          resize(rows, sizeUp(cols)); break;
            case ButtonId::RunTests:        runTests(); break;
        }
    }

private:
    MazeLabSession& session;
    std::vector<GuiButton> buttons;
    std::vector<std::string> info;
    Overlay overlay = Overlay::Plain;
    SolveResult mainResult;
    SolveResult bfsResult;
    Player player;
    bool playing = false;

    const int sidebarX = WINDOW_WIDTH - SIDEBAR_WIDTH;
    const int mazeAreaX = 20;
    const int mazeAreaY = 20;
    const int mazeAreaWidth = sidebarX - 40;
    const int mazeAreaHeight = WINDOW_HEIGHT - 40;
    int rowsY = 0;
    int colsY = 0;
    int sizeButtonsX = 0;
    int infoTop = 0;

    void addButton(ButtonId id, int x, int y, int width, const std::string& label) {
        buttons.push_back({id, x, y, width, 32, label});
    }

    void addSmallButton(ButtonId id, int x, int y, const std::string& label) {
        buttons.push_back({id, x, y, 28, 26, label});
    }

    static int sizeUp(int value) {
        return std::min(MAX_SIZE, (value / SIZE_STEP + 1) * SIZE_STEP);
    }

    static int sizeDown(int value) {
        int smaller = (value % SIZE_STEP == 0) ? value - SIZE_STEP : value / SIZE_STEP * SIZE_STEP;
        return std::max(MIN_SIZE, smaller);
    }

    bool requireMaze() {
        if (session.maze.isGenerated()) return true;
        info = {"No maze yet.", "Press 1 or 2 to generate one."};
        return false;
    }

    void generate(MazeGenerator& generator) {
        session.generateWith(generator);
        overlay = Overlay::Plain;
        info = {
            "Generated a " + sizeText() + " maze",
            "with " + generator.name() + ".",
            "",
            "Next: play (3), solve (4, 5)",
            "or compare (6).",
        };
    }

    void togglePlay() {
        if (playing) {
            playing = false;
            info = {"Play stopped after " + std::to_string(player.moveCount()) + " moves."};
            return;
        }
        if (!requireMaze()) return;
        playing = true;
        overlay = Overlay::Plain;
        player.placeAt(session.maze.start());
        showPlayInfo("");
    }

    void movePlayer(Direction dir) {
        bool moved = player.move(session.maze, dir);

        if (player.hasReached(session.maze.end())) {
            playing = false;
            int moves = player.moveCount();
            int shortest = session.bfsSolver.solve(session.maze).steps();
            int efficiency = (int)(100.0 * shortest / moves + 0.5);
            info = {
                "You reached the exit!",
                "Moves: " + std::to_string(moves),
                "Time: " + oneDecimal(player.secondsElapsed()) + " s",
                "Shortest path: " + std::to_string(shortest),
                "Efficiency: " + std::to_string(efficiency) + "%",
            };
            return;
        }

        showPlayInfo(moved ? "" : "Blocked by a wall.");
    }

    void showPlayInfo(const std::string& notice) {
        std::string keys = std::string(1, KEY_UP) + " " + KEY_LEFT + " " + KEY_DOWN + " " + KEY_RIGHT;
        info = {
            "Play mode",
            "Move with the arrow keys or " + keys + ".",
            "Press 3 or Esc to stop.",
            "",
            "Moves: " + std::to_string(player.moveCount()),
        };
        if (!notice.empty()) info.push_back(notice);
    }

    void solve(MazeSolver& solver) {
        if (!requireMaze()) return;
        mainResult = solver.solve(session.maze);
        overlay = Overlay::SingleSolution;

        if (!mainResult.found) {
            info = {solver.name() + " found no path."};
            return;
        }
        info = {
            solver.name() + " solution",
            "Path length\t" + std::to_string(mainResult.steps()) + " steps",
            "Cells expanded\t" + std::to_string(mainResult.cellsExpanded()),
            "Time\t" + microseconds(mainResult.elapsedMicroseconds),
        };
    }

    void compare() {
        if (!requireMaze()) return;
        bfsResult = session.bfsSolver.solve(session.maze);
        mainResult = session.aStarSolver.solve(session.maze);
        overlay = Overlay::Comparison;

        int saved = bfsResult.cellsExpanded() - mainResult.cellsExpanded();
        info = {
            "BFS vs A* on this maze",
            "\tBFS\tA*",
            "Path length\t" + std::to_string(bfsResult.steps()) + "\t" + std::to_string(mainResult.steps()),
            "Expanded\t" + std::to_string(bfsResult.cellsExpanded()) + "\t" + std::to_string(mainResult.cellsExpanded()),
            "Time\t" + microseconds(bfsResult.elapsedMicroseconds) + "\t" + microseconds(mainResult.elapsedMicroseconds),
            "A* expanded " + std::to_string(saved) + " fewer cells.",
        };
    }

    void resize(int rows, int cols) {
        session.resize(rows, cols);
        overlay = Overlay::Plain;
        info = {"Size set to " + sizeText() + ".", "Generate a new maze (1 or 2)."};
    }

    void runTests() {
        ValidationSuite suite;
        std::vector<TestOutcome> outcomes = suite.runAll();

        int passed = 0;
        std::vector<std::string> failed;
        for (const TestOutcome& outcome : outcomes) {
            if (outcome.passed) passed++;
            else failed.push_back(outcome.name);
        }

        info = {"Validation tests", "Passed: " + std::to_string(passed) + " of " + std::to_string(outcomes.size())};
        if (failed.empty()) info.push_back("All tests passed.");
        for (const std::string& name : failed) info.push_back("Failed: " + name);
    }

    std::string sizeText() const {
        return std::to_string(session.maze.rows()) + " x " + std::to_string(session.maze.cols());
    }

    static std::string oneDecimal(double value) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.1f", value);
        return buffer;
    }

    static std::string microseconds(double value) {
        return oneDecimal(value) + " us";
    }

    int cellSize() const {
        return std::min(mazeAreaWidth / session.maze.cols(), mazeAreaHeight / session.maze.rows());
    }

    int originX() const { return mazeAreaX + (mazeAreaWidth - cellSize() * session.maze.cols()) / 2; }
    int originY() const { return mazeAreaY + (mazeAreaHeight - cellSize() * session.maze.rows()) / 2; }

    std::vector<std::vector<Color>> cellColors() const {
        const Maze& maze = session.maze;
        std::vector<std::vector<Color>> colors(maze.rows(), std::vector<Color>(maze.cols(), CELL_COLOR));

        if (overlay == Overlay::Comparison) {
            for (const Coord& pos : bfsResult.expandedCells) colors[pos.row][pos.col] = BFS_ONLY_COLOR;
        }
        if (overlay != Overlay::Plain) {
            for (const Coord& pos : mainResult.expandedCells) colors[pos.row][pos.col] = EXPANDED_COLOR;
            for (const Coord& pos : mainResult.path)          colors[pos.row][pos.col] = PATH_COLOR;
        }
        colors[maze.start().row][maze.start().col] = START_COLOR;
        colors[maze.end().row][maze.end().col] = EXIT_COLOR;
        return colors;
    }

    void drawMaze(Canvas& canvas) const {
        const Maze& maze = session.maze;
        const int size = cellSize();
        const int x0 = originX();
        const int y0 = originY();
        const int wall = std::max(2, size / 10);
        const int half = wall / 2;

        std::vector<std::vector<Color>> colors = cellColors();
        for (int row = 0; row < maze.rows(); row++)
            for (int col = 0; col < maze.cols(); col++)
                canvas.fillRect(x0 + col * size, y0 + row * size, size, size, colors[row][col]);

        if (playing) {
            Coord pos = player.position();
            int centerX = x0 + pos.col * size + size / 2;
            int centerY = y0 + pos.row * size + size / 2;
            canvas.fillCircle(centerX, centerY, std::max(3, size * 3 / 10), PLAYER_COLOR);
        }

        for (int row = 0; row < maze.rows(); row++) {
            for (int col = 0; col < maze.cols(); col++) {
                Coord here{row, col};
                int x = x0 + col * size;
                int y = y0 + row * size;
                bool lastRow = (row == maze.rows() - 1);
                bool lastCol = (col == maze.cols() - 1);

                if (maze.hasWall(here, Up))
                    canvas.fillRect(x - half, y - half, size + wall, wall, WALL_COLOR);
                if (maze.hasWall(here, Left))
                    canvas.fillRect(x - half, y - half, wall, size + wall, WALL_COLOR);
                if (lastRow && maze.hasWall(here, Down))
                    canvas.fillRect(x - half, y + size - half, size + wall, wall, WALL_COLOR);
                if (lastCol && maze.hasWall(here, Right))
                    canvas.fillRect(x + size - half, y - half, wall, size + wall, WALL_COLOR);
            }
        }
    }

    void drawSidebar(Canvas& canvas) const {
        canvas.fillRect(sidebarX, 0, SIDEBAR_WIDTH, WINDOW_HEIGHT, SIDEBAR_COLOR);
        canvas.fillRect(sidebarX, 0, 1, WINDOW_HEIGHT, BORDER_COLOR);

        const int left = sidebarX + 16;
        canvas.drawText(left, 18, GUI_TITLE, TITLE_COLOR);
        canvas.drawText(left, 40, GUI_GENERATOR_LABEL + session.lastGeneratorName, MUTED_TEXT_COLOR);

        for (const GuiButton& button : buttons) drawButton(canvas, button);

        const int textY = std::max(0, (26 - canvas.textHeight()) / 2);
        canvas.drawText(left, rowsY + textY, ROWS_LABEL, TEXT_COLOR);
        canvas.drawText(left, colsY + textY, COLUMNS_LABEL, TEXT_COLOR);
        drawCentered(canvas, std::to_string(session.maze.rows()), sizeButtonsX + 28, rowsY + textY, 56);
        drawCentered(canvas, std::to_string(session.maze.cols()), sizeButtonsX + 28, colsY + textY, 56);

        drawInfoPanel(canvas, left, infoTop, SIDEBAR_WIDTH - 32);
        drawLegend(canvas, left, WINDOW_HEIGHT - 92);
    }

    void drawButton(Canvas& canvas, const GuiButton& button) const {
        bool active = (button.id == ButtonId::Play && playing);
        std::string label = active ? BUTTON_STOP : button.label;

        canvas.fillRect(button.x, button.y, button.width, button.height, active ? BUTTON_ACTIVE_COLOR : BUTTON_COLOR);
        int textX = button.x + (button.width - canvas.textWidth(label)) / 2;
        int textY = button.y + (button.height - canvas.textHeight()) / 2;
        canvas.drawText(textX, textY, label, BUTTON_TEXT_COLOR);
    }

    void drawCentered(Canvas& canvas, const std::string& text, int x, int y, int width) const {
        canvas.drawText(x + (width - canvas.textWidth(text)) / 2, y, text, TEXT_COLOR);
    }

    void drawInfoPanel(Canvas& canvas, int x, int y, int width) const {
        const int height = WINDOW_HEIGHT - 110 - y;
        canvas.fillRect(x, y, width, height, CELL_COLOR);
        canvas.outlineRect(x, y, width, height, 1, BORDER_COLOR);

        const int lineHeight = canvas.textHeight() + 5;
        const int columnX[] = {x + 10, x + 120, x + 185};
        int lineY = y + 8;

        for (const std::string& line : info) {
            int column = 0;
            std::string part;
            for (char c : line) {
                if (c == '\t' && column < 2) {
                    canvas.drawText(columnX[column], lineY, part, TEXT_COLOR);
                    part.clear();
                    column++;
                } else {
                    part += c;
                }
            }
            canvas.drawText(columnX[column], lineY, part, TEXT_COLOR);
            lineY += lineHeight;
        }
    }

    void drawLegend(Canvas& canvas, int x, int y) const {
        struct Entry { Color color; std::string label; };
        const std::vector<Entry> entries = {
            {START_COLOR,    LEGEND_START},
            {EXIT_COLOR,     LEGEND_EXIT},
            {PATH_COLOR,     LEGEND_PATH},
            {EXPANDED_COLOR, LEGEND_EXPANDED},
            {BFS_ONLY_COLOR, LEGEND_BFS_ONLY},
            {PLAYER_COLOR,   LEGEND_PLAYER},
        };
        const int lineHeight = std::max(20, canvas.textHeight() + 6);

        for (int i = 0; i < (int)entries.size(); i++) {
            int entryX = x + (i % 2) * 125;
            int entryY = y + (i / 2) * lineHeight;
            canvas.fillRect(entryX, entryY + 2, 14, 14, entries[i].color);
            canvas.outlineRect(entryX, entryY + 2, 14, 14, 1, BORDER_COLOR);
            canvas.drawText(entryX + 22, entryY + (18 - canvas.textHeight()) / 2, entries[i].label, TEXT_COLOR);
        }
    }
};
