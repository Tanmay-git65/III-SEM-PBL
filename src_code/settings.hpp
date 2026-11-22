#pragma once
#include <string>

struct Color {
    unsigned char r, g, b;
};

const std::string WINDOW_TITLE = "Maze Lab";
const int WINDOW_WIDTH = 1120;
const int WINDOW_HEIGHT = 720;
const int SIDEBAR_WIDTH = 280;
const std::string FONT_NAME = "Segoe UI";
const int FONT_SIZE = 16;

const Color BACKGROUND_COLOR    = {230, 233, 238};
const Color SIDEBAR_COLOR       = {245, 246, 248};
const Color BORDER_COLOR        = {200, 204, 210};
const Color CELL_COLOR          = {255, 255, 255};
const Color WALL_COLOR          = { 33,  37,  41};
const Color START_COLOR         = { 46, 160,  67};
const Color EXIT_COLOR          = {207,  34,  46};
const Color PATH_COLOR          = {255, 160,  40};
const Color EXPANDED_COLOR      = {198, 224, 255};
const Color BFS_ONLY_COLOR      = {255, 230, 150};
const Color PLAYER_COLOR        = { 20,  55, 105};
const Color BUTTON_COLOR        = { 20,  55, 105};
const Color BUTTON_ACTIVE_COLOR = {196, 120,   0};
const Color BUTTON_TEXT_COLOR   = {255, 255, 255};
const Color TITLE_COLOR         = { 20,  55, 105};
const Color TEXT_COLOR          = { 33,  37,  41};
const Color MUTED_TEXT_COLOR    = { 90,  90,  90};

const std::string GUI_TITLE            = "MAZE LAB";
const std::string GUI_GENERATOR_LABEL  = "Generator: ";
const std::string BUTTON_DFS           = "1. Generate maze (DFS)";
const std::string BUTTON_KRUSKAL       = "2. Generate maze (Kruskal)";
const std::string BUTTON_PLAY          = "3. Play maze";
const std::string BUTTON_STOP          = "3. Stop playing";
const std::string BUTTON_BFS           = "4. Solve with BFS";
const std::string BUTTON_ASTAR         = "5. Solve with A*";
const std::string BUTTON_COMPARE       = "6. Compare BFS vs A*";
const std::string BUTTON_TESTS         = "7. Run validation tests";
const std::string ROWS_LABEL           = " Rows";
const std::string COLUMNS_LABEL        = " Columns";

const std::string LEGEND_START    = "Start";
const std::string LEGEND_EXIT     = "Exit";
const std::string LEGEND_PATH     = "Path";
const std::string LEGEND_EXPANDED = "Expanded";
const std::string LEGEND_BFS_ONLY = "BFS only";
const std::string LEGEND_PLAYER   = "Player";

const int CONSOLE_CELL_WIDTH = 2;
const std::string START_MARK  = "SS";
const std::string EXIT_MARK   = "EE";
const std::string PLAYER_MARK = "PP";
const std::string PATH_MARK   = "**";
const std::string EMPTY_MARK  = "  ";

const std::string CONSOLE_TITLE = "=============== MAZE LAB ===============";
const std::string MENU_DFS      = "1. Generate maze (Randomized DFS)";
const std::string MENU_KRUSKAL  = "2. Generate maze (Kruskal's Algorithm)";
const std::string MENU_PLAY     = "3. Play maze";
const std::string MENU_BFS      = "4. Solve with BFS";
const std::string MENU_ASTAR    = "5. Solve with A*";
const std::string MENU_COMPARE  = "6. Compare BFS vs A*";
const std::string MENU_SIZE     = "7. Change maze size";
const std::string MENU_TESTS    = "8. Run validation tests";
const std::string MENU_GUI      = "9. Open graphical window";
const std::string MENU_EXIT     = "0. Exit";

const char KEY_UP    = 'W';
const char KEY_LEFT  = 'A';
const char KEY_DOWN  = 'S';
const char KEY_RIGHT = 'D';
const char KEY_QUIT  = 'Q';

const int DEFAULT_ROWS = 10;
const int DEFAULT_COLS = 10;
const int MIN_SIZE = 2;
const int MAX_SIZE = 50;
const int SIZE_STEP = 5;
