/*
 * ============================================================
 *  MAZE LAB  —  Phase II
 *  Single-file C++17 console application
 *
 *  Architecture
 *  ─────────────────────────────────────────────────────────
 *  Coord            – row / column position
 *  Cell             – per-cell wall state and visited flag
 *  Maze             – owns the grid, centralises wall logic
 *  DSU              – Disjoint-Set Union (Kruskal helper)
 *  SolveResult      – common solver output structure
 *  MazeGenerator    – abstract generation interface
 *    DFSGenerator   – iterative randomised depth-first search
 *    KruskalGenerator – randomised Kruskal via DSU
 *  MazeSolver       – abstract solver interface
 *    BFSSolver      – breadth-first search (shortest path)
 *    AStarSolver    – A* with Manhattan heuristic
 *  Player           – position, move counter, elapsed timer
 *  ConsoleRenderer  – pure ASCII rendering, no algorithm logic
 *  runMenu()        – interactive application loop
 *  runTests()       – integrated validation suite
 * ============================================================
 */

#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <string>
#include <random>
#include <chrono>
#include <algorithm>
#include <functional>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <limits>
#include <memory>

// ============================================================
//  Coord  – a simple (row, col) position type
// ============================================================
struct Coord {
    int row{0};
    int col{0};

    bool operator==(const Coord& o) const { return row == o.row && col == o.col; }
    bool operator!=(const Coord& o) const { return !(*this == o); }
    bool operator<(const Coord& o) const {
        if (row != o.row) return row < o.row;
        return col < o.col;
    }

    static Coord none() { return {-1, -1}; }
    bool isNone() const { return row < 0 || col < 0; }
};

// ============================================================
//  Cell  – stores four wall flags and a visited flag
// ============================================================
struct Cell {
    bool wallTop    {true};
    bool wallBottom {true};
    bool wallLeft   {true};
    bool wallRight  {true};
    bool visited    {false};

    void reset() {
        wallTop = wallBottom = wallLeft = wallRight = true;
        visited = false;
    }
};

// ============================================================
//  Maze  – owns the 2-D grid, centralises wall management
// ============================================================
class Maze {
public:
    Maze() : rows_(0), cols_(0), start_{0,0}, end_{0,0} {}

    void init(int rows, int cols) {
        rows_ = rows;
        cols_ = cols;
        grid_.assign(rows_, std::vector<Cell>(cols_));
        start_ = {0, 0};
        end_   = {rows_ - 1, cols_ - 1};
        generated_ = false;
    }

    void resetWalls() {
        for (auto& row : grid_)
            for (auto& cell : row)
                cell.reset();
        generated_ = false;
    }

    int  rows()  const { return rows_; }
    int  cols()  const { return cols_; }
    bool valid() const { return rows_ > 0 && cols_ > 0; }
    bool isGenerated() const { return generated_; }
    void markGenerated()     { generated_ = true; }

    bool inBounds(int r, int c) const {
        return r >= 0 && r < rows_ && c >= 0 && c < cols_;
    }
    bool inBounds(const Coord& p) const { return inBounds(p.row, p.col); }

    Cell& cell(int r, int c)             { return grid_[r][c]; }
    const Cell& cell(int r, int c) const { return grid_[r][c]; }
    Cell& cell(const Coord& p)             { return grid_[p.row][p.col]; }
    const Cell& cell(const Coord& p) const { return grid_[p.row][p.col]; }

    Coord start() const { return start_; }
    Coord end()   const { return end_;   }
    void  setStart(const Coord& c) { start_ = c; }
    void  setEnd  (const Coord& c) { end_   = c; }

    // Open passage between (r,c) and its neighbour in direction dir.
    // Directions: 0=up, 1=right, 2=down, 3=left
    // Both cells are updated to stay consistent.
    bool openPassage(int r, int c, int dir) {
        int nr = r, nc = c;
        switch (dir) {
            case 0: nr = r - 1; break;
            case 1: nc = c + 1; break;
            case 2: nr = r + 1; break;
            case 3: nc = c - 1; break;
            default: return false;
        }
        if (!inBounds(nr, nc)) return false;

        switch (dir) {
            case 0: grid_[r][c].wallTop    = false; grid_[nr][nc].wallBottom = false; break;
            case 1: grid_[r][c].wallRight  = false; grid_[nr][nc].wallLeft   = false; break;
            case 2: grid_[r][c].wallBottom = false; grid_[nr][nc].wallTop    = false; break;
            case 3: grid_[r][c].wallLeft   = false; grid_[nr][nc].wallRight  = false; break;
        }
        return true;
    }

    bool openPassage(const Coord& a, const Coord& b) {
        int dr = b.row - a.row;
        int dc = b.col - a.col;
        if      (dr == -1 && dc == 0) return openPassage(a.row, a.col, 0);
        else if (dr ==  0 && dc == 1) return openPassage(a.row, a.col, 1);
        else if (dr ==  1 && dc == 0) return openPassage(a.row, a.col, 2);
        else if (dr ==  0 && dc ==-1) return openPassage(a.row, a.col, 3);
        return false;
    }

    bool canMove(int r, int c, int dir) const {
        if (!inBounds(r, c)) return false;
        const Cell& cl = grid_[r][c];
        int nr = r, nc = c;
        switch (dir) {
            case 0: if (cl.wallTop)    return false; nr = r - 1; break;
            case 1: if (cl.wallRight)  return false; nc = c + 1; break;
            case 2: if (cl.wallBottom) return false; nr = r + 1; break;
            case 3: if (cl.wallLeft)   return false; nc = c - 1; break;
            default: return false;
        }
        return inBounds(nr, nc);
    }

    bool canMove(const Coord& from, int dir) const {
        return canMove(from.row, from.col, dir);
    }

    Coord neighbour(const Coord& from, int dir) const {
        switch (dir) {
            case 0: return {from.row - 1, from.col};
            case 1: return {from.row,     from.col + 1};
            case 2: return {from.row + 1, from.col};
            case 3: return {from.row,     from.col - 1};
        }
        return from;
    }

    bool wallsConsistent() const {
        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                if (c + 1 < cols_) {
                    if (grid_[r][c].wallRight != grid_[r][c+1].wallLeft)
                        return false;
                }
                if (r + 1 < rows_) {
                    if (grid_[r][c].wallBottom != grid_[r+1][c].wallTop)
                        return false;
                }
            }
        }
        return true;
    }

    bool outerBoundaryIntact() const {
        for (int c = 0; c < cols_; ++c) {
            if (!grid_[0][c].wallTop)               return false;
            if (!grid_[rows_-1][c].wallBottom)       return false;
        }
        for (int r = 0; r < rows_; ++r) {
            if (!grid_[r][0].wallLeft)               return false;
            if (!grid_[r][cols_-1].wallRight)        return false;
        }
        return true;
    }

private:
    int rows_{0}, cols_{0};
    std::vector<std::vector<Cell>> grid_;
    Coord start_, end_;
    bool generated_{false};
};

// ============================================================
//  DSU  – Disjoint-Set Union with path compression & union-by-rank
// ============================================================
class DSU {
public:
    explicit DSU(int n) : parent_(n), rank_(n, 0) {
        for (int i = 0; i < n; ++i) parent_[i] = i;
    }

    int find(int x) {
        if (parent_[x] != x)
            parent_[x] = find(parent_[x]);
        return parent_[x];
    }

    bool unite(int x, int y) {
        int rx = find(x), ry = find(y);
        if (rx == ry) return false;
        if (rank_[rx] < rank_[ry]) std::swap(rx, ry);
        parent_[ry] = rx;
        if (rank_[rx] == rank_[ry]) ++rank_[rx];
        return true;
    }

    bool connected(int x, int y) { return find(x) == find(y); }

private:
    std::vector<int> parent_, rank_;
};

// ============================================================
//  SolveResult  – common result structure for all solvers
// ============================================================
struct SolveResult {
    bool            solved        {false};
    std::vector<Coord> path;
    int             nodesExpanded {0};
    int             pathLength    {0};   // edges = path.size() - 1
    double          elapsedUs     {0.0}; // microseconds

    static SolveResult failure() { return SolveResult{}; }
};

// ============================================================
//  MazeGenerator  – abstract strategy interface
// ============================================================
class MazeGenerator {
public:
    virtual ~MazeGenerator() = default;
    virtual void generate(Maze& maze, std::mt19937& rng) = 0;
    virtual std::string name() const = 0;
};

// ============================================================
//  DFSGenerator  – iterative randomised depth-first search
// ============================================================
class DFSGenerator : public MazeGenerator {
public:
    std::string name() const override { return "Randomized DFS"; }

    void generate(Maze& maze, std::mt19937& rng) override {
        maze.resetWalls();

        const int dr[4] = {-1, 0, 1,  0};
        const int dc[4] = { 0, 1, 0, -1};

        Coord start = maze.start();
        maze.cell(start).visited = true;

        std::stack<Coord> stk;
        stk.push(start);

        while (!stk.empty()) {
            Coord cur = stk.top();

            std::vector<int> dirs;
            for (int d = 0; d < 4; ++d) {
                int nr = cur.row + dr[d];
                int nc = cur.col + dc[d];
                if (maze.inBounds(nr, nc) && !maze.cell(nr, nc).visited)
                    dirs.push_back(d);
            }

            if (dirs.empty()) {
                stk.pop();
            } else {
                std::uniform_int_distribution<int> pick(0, (int)dirs.size() - 1);
                int d = dirs[pick(rng)];
                int nr = cur.row + dr[d];
                int nc = cur.col + dc[d];

                maze.openPassage(cur.row, cur.col, d);
                maze.cell(nr, nc).visited = true;
                stk.push({nr, nc});
            }
        }
        maze.markGenerated();
    }
};

// ============================================================
//  KruskalGenerator  – randomised Kruskal's algorithm
// ============================================================
struct Edge {
    int r1, c1, r2, c2;
};

class KruskalGenerator : public MazeGenerator {
public:
    std::string name() const override { return "Kruskal's Algorithm"; }

    void generate(Maze& maze, std::mt19937& rng) override {
        maze.resetWalls();
        const int rows = maze.rows();
        const int cols = maze.cols();

        DSU dsu(rows * cols);

        // Only rightward and downward edges to avoid duplicates
        std::vector<Edge> edges;
        edges.reserve(rows * (cols - 1) + (rows - 1) * cols);

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (c + 1 < cols) edges.push_back({r, c, r, c + 1});
                if (r + 1 < rows) edges.push_back({r, c, r + 1, c});
            }
        }

        std::shuffle(edges.begin(), edges.end(), rng);

        for (const Edge& e : edges) {
            int id1 = e.r1 * cols + e.c1;
            int id2 = e.r2 * cols + e.c2;
            if (dsu.unite(id1, id2)) {
                maze.openPassage({e.r1, e.c1}, {e.r2, e.c2});
            }
        }
        maze.markGenerated();
    }
};

// ============================================================
//  MazeSolver  – abstract solver interface
// ============================================================
class MazeSolver {
public:
    virtual ~MazeSolver() = default;
    virtual SolveResult solve(const Maze& maze,
                              const Coord& startPos,
                              const Coord& endPos) = 0;
    virtual std::string name() const = 0;
};

// ============================================================
//  BFSSolver  – breadth-first search, guaranteed shortest path
//
//  Node-expansion: a node is counted when it is dequeued and
//  its neighbours are examined.
// ============================================================
class BFSSolver : public MazeSolver {
public:
    std::string name() const override { return "BFS"; }

    SolveResult solve(const Maze& maze,
                      const Coord& startPos,
                      const Coord& endPos) override
    {
        auto t0 = std::chrono::steady_clock::now();

        const int rows = maze.rows();
        const int cols = maze.cols();

        if (!maze.inBounds(startPos) || !maze.inBounds(endPos)) {
            auto t1 = std::chrono::steady_clock::now();
            SolveResult res = SolveResult::failure();
            res.elapsedUs = std::chrono::duration<double, std::micro>(t1 - t0).count();
            return res;
        }

        // parent[r][c] = Coord::none() means not visited
        std::vector<std::vector<Coord>> parent(
            rows, std::vector<Coord>(cols, Coord::none()));

        parent[startPos.row][startPos.col] = startPos;

        std::queue<Coord> q;
        q.push(startPos);

        int expanded = 0;

        while (!q.empty()) {
            Coord cur = q.front(); q.pop();
            ++expanded;

            if (cur == endPos) break;

            for (int d = 0; d < 4; ++d) {
                if (!maze.canMove(cur, d)) continue;
                Coord nb = maze.neighbour(cur, d);
                if (!parent[nb.row][nb.col].isNone()) continue;
                parent[nb.row][nb.col] = cur;
                q.push(nb);
            }
        }

        auto t1 = std::chrono::steady_clock::now();
        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();

        if (parent[endPos.row][endPos.col].isNone()) {
            SolveResult res = SolveResult::failure();
            res.nodesExpanded = expanded;
            res.elapsedUs = us;
            return res;
        }

        std::vector<Coord> path;
        for (Coord c = endPos; !(c == startPos); c = parent[c.row][c.col])
            path.push_back(c);
        path.push_back(startPos);
        std::reverse(path.begin(), path.end());

        SolveResult res;
        res.solved        = true;
        res.path          = std::move(path);
        res.nodesExpanded = expanded;
        res.pathLength    = (int)res.path.size() - 1;
        res.elapsedUs     = us;
        return res;
    }
};

// ============================================================
//  AStarSolver  – A* with Manhattan heuristic
//
//  f(n) = g(n) + h(n)
//  h(n) = |row_n - row_goal| + |col_n - col_goal|
//  All edge costs = 1.
//
//  Node-expansion: a node is counted when it is first popped
//  from the priority queue (not yet finalized).
// ============================================================
class AStarSolver : public MazeSolver {
public:
    std::string name() const override { return "A*"; }

    SolveResult solve(const Maze& maze,
                      const Coord& startPos,
                      const Coord& endPos) override
    {
        auto t0 = std::chrono::steady_clock::now();

        const int rows = maze.rows();
        const int cols = maze.cols();

        if (!maze.inBounds(startPos) || !maze.inBounds(endPos)) {
            auto t1 = std::chrono::steady_clock::now();
            SolveResult res = SolveResult::failure();
            res.elapsedUs = std::chrono::duration<double, std::micro>(t1 - t0).count();
            return res;
        }

        const int INF = std::numeric_limits<int>::max();

        std::vector<std::vector<int>> g(rows, std::vector<int>(cols, INF));
        std::vector<std::vector<Coord>> parent(
            rows, std::vector<Coord>(cols, Coord::none()));
        std::vector<std::vector<bool>> closed(
            rows, std::vector<bool>(cols, false));

        auto heuristic = [&](const Coord& c) {
            return std::abs(c.row - endPos.row) + std::abs(c.col - endPos.col);
        };

        // Min-heap: (f, g, Coord)
        using Entry = std::tuple<int, int, Coord>;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pq;

        g[startPos.row][startPos.col] = 0;
        parent[startPos.row][startPos.col] = startPos;
        pq.push({heuristic(startPos), 0, startPos});

        int expandedCount = 0;

        while (!pq.empty()) {
            auto [f, gCur, cur] = pq.top(); pq.pop();

            if (closed[cur.row][cur.col]) continue;
            closed[cur.row][cur.col] = true;
            ++expandedCount;

            if (cur == endPos) break;

            for (int d = 0; d < 4; ++d) {
                if (!maze.canMove(cur, d)) continue;
                Coord nb = maze.neighbour(cur, d);
                if (closed[nb.row][nb.col]) continue;

                int newG = g[cur.row][cur.col] + 1;
                if (newG < g[nb.row][nb.col]) {
                    g[nb.row][nb.col] = newG;
                    parent[nb.row][nb.col] = cur;
                    pq.push({newG + heuristic(nb), newG, nb});
                }
            }
        }

        auto t1 = std::chrono::steady_clock::now();
        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();

        if (g[endPos.row][endPos.col] == INF) {
            SolveResult res = SolveResult::failure();
            res.nodesExpanded = expandedCount;
            res.elapsedUs = us;
            return res;
        }

        std::vector<Coord> path;
        Coord c = endPos;
        while (!(c == startPos)) {
            path.push_back(c);
            Coord p = parent[c.row][c.col];
            if (p == c) break;  // safety
            c = p;
        }
        path.push_back(startPos);
        std::reverse(path.begin(), path.end());

        SolveResult res;
        res.solved        = true;
        res.path          = std::move(path);
        res.nodesExpanded = expandedCount;
        res.pathLength    = (int)res.path.size() - 1;
        res.elapsedUs     = us;
        return res;
    }
};

// ============================================================
//  Player  – position, movement counting, timing, victory
// ============================================================
class Player {
public:
    Player() = default;

    void init(const Coord& startPos) {
        pos_         = startPos;
        moveCount_   = 0;
        reachedEnd_  = false;
        startTime_   = std::chrono::steady_clock::now();
    }

    bool move(const Maze& maze, int dir) {
        if (maze.inBounds(pos_) && maze.canMove(pos_, dir)) {
            pos_ = maze.neighbour(pos_, dir);
            ++moveCount_;
            return true;
        }
        return false;
    }

    void checkVictory(const Coord& endPos) {
        if (pos_ == endPos) reachedEnd_ = true;
    }

    Coord pos()         const { return pos_; }
    int   moveCount()   const { return moveCount_; }
    bool  reachedEnd()  const { return reachedEnd_; }

    double elapsedSeconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - startTime_).count();
    }

private:
    Coord pos_         {0, 0};
    int   moveCount_   {0};
    bool  reachedEnd_  {false};
    std::chrono::steady_clock::time_point startTime_;
};

// ============================================================
//  ConsoleRenderer  – pure ASCII rendering, no algorithm logic
//
//  Cell layout: each cell is 2 chars wide.
//
//    +--+--+
//    |SS|  |
//    +--+--+
//
//  Symbols:  S=Start  E=End  P=Player  *=Path  (space)=empty
// ============================================================
class ConsoleRenderer {
public:
    void render(const Maze& maze,
                const Player& player,
                const std::vector<Coord>& solutionPath = {}) const
    {
        const int rows = maze.rows();
        const int cols = maze.cols();

        std::vector<std::vector<bool>> onPath(rows, std::vector<bool>(cols, false));
        for (const Coord& c : solutionPath)
            if (maze.inBounds(c)) onPath[c.row][c.col] = true;

        const Coord startPos = maze.start();
        const Coord endPos   = maze.end();
        const Coord pPos     = player.pos();

        // Top border
        std::cout << '+';
        for (int c = 0; c < cols; ++c) std::cout << "--+";
        std::cout << '\n';

        for (int r = 0; r < rows; ++r) {
            // Cell content row
            std::cout << '|';
            for (int c = 0; c < cols; ++c) {
                char sym = ' ';
                Coord here{r, c};
                if      (pPos     == here) sym = 'P';
                else if (endPos   == here) sym = 'E';
                else if (startPos == here) sym = 'S';
                else if (onPath[r][c])     sym = '*';

                std::cout << sym << sym;
                std::cout << (maze.cell(r,c).wallRight ? '|' : ' ');
            }
            std::cout << '\n';

            // Bottom wall row
            std::cout << '+';
            for (int c = 0; c < cols; ++c) {
                std::cout << (maze.cell(r,c).wallBottom ? "--" : "  ");
                std::cout << '+';
            }
            std::cout << '\n';
        }

        std::cout << "\n  Legend: S=Start  E=End  P=Player";
        if (!solutionPath.empty()) std::cout << "  *=Solution Path";
        std::cout << '\n';
    }

    void renderNoPlayer(const Maze& maze,
                        const std::vector<Coord>& solutionPath = {}) const
    {
        // Use an off-grid player position so 'P' never renders
        Player dummy;
        dummy.init({-1, -1});
        render(maze, dummy, solutionPath);
    }
};

// ============================================================
//  Utility: print solver result
// ============================================================
static void printSolveResult(const SolveResult& res, const std::string& algoName) {
    std::cout << "\n--- " << algoName << " Result ---\n";
    if (!res.solved) {
        std::cout << "  Status       : No path found\n";
        std::cout << "  Nodes expnd  : " << res.nodesExpanded << '\n';
        std::cout << "  Time         : " << std::fixed << std::setprecision(2)
                  << res.elapsedUs << " us\n";
        return;
    }
    std::cout << "  Status       : Path found\n";
    std::cout << "  Path length  : " << res.pathLength << " edges\n";
    std::cout << "  Nodes expnd  : " << res.nodesExpanded << '\n';
    std::cout << "  Time         : " << std::fixed << std::setprecision(2)
              << res.elapsedUs << " us\n";
}

static void printComparisonTable(const SolveResult& bfsRes,
                                 const SolveResult& astarRes) {
    auto yesNo = [](bool b) -> const char* { return b ? "Yes" : "No"; };

    auto timeStr = [](double us) -> std::string {
        std::ostringstream oss;
        if (us < 1000.0)
            oss << std::fixed << std::setprecision(1) << us << "us";
        else
            oss << std::fixed << std::setprecision(2) << us / 1000.0 << "ms";
        return oss.str();
    };

    std::cout << "\n";
    std::cout << "+------------------+-----------+-----------+\n";
    std::cout << "| Metric           |       BFS |        A* |\n";
    std::cout << "+------------------+-----------+-----------+\n";
    std::cout << "| Solution found   |" << std::setw(10) << yesNo(bfsRes.solved)
              << " |" << std::setw(10) << yesNo(astarRes.solved) << " |\n";

    auto pathStr = [](const SolveResult& r) -> std::string {
        return r.solved ? std::to_string(r.pathLength) : "N/A";
    };
    std::cout << "| Path length      |" << std::setw(10) << pathStr(bfsRes)
              << " |" << std::setw(10) << pathStr(astarRes) << " |\n";
    std::cout << "| Nodes expanded   |" << std::setw(10) << bfsRes.nodesExpanded
              << " |" << std::setw(10) << astarRes.nodesExpanded << " |\n";
    std::cout << "| Execution time   |" << std::setw(10) << timeStr(bfsRes.elapsedUs)
              << " |" << std::setw(10) << timeStr(astarRes.elapsedUs) << " |\n";
    std::cout << "+------------------+-----------+-----------+\n";

    if (bfsRes.solved && astarRes.solved) {
        if (bfsRes.pathLength == astarRes.pathLength)
            std::cout << "  Both algorithms found the same shortest-path length.\n";
        else
            std::cout << "  Path lengths differ (BFS=" << bfsRes.pathLength
                      << ", A*=" << astarRes.pathLength << ").\n";
    }
}

// ============================================================
//  Input helpers
// ============================================================
static int readInt(const std::string& prompt, int lo, int hi) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cin.clear();
            return lo;
        }
        if (line.empty()) continue;
        try {
            std::size_t idx;
            int val = std::stoi(line, &idx);
            if (idx == line.size() && val >= lo && val <= hi) return val;
        } catch (...) {}
        std::cout << "  Invalid input. Enter a number between "
                  << lo << " and " << hi << ".\n";
    }
}

static std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// ============================================================
//  Built-in Validation Tests
// ============================================================
static void runTests() {
    std::cout << "\n========== MAZE LAB VALIDATION ==========\n\n";

    int passed = 0, failed = 0;

    auto pass = [&](const std::string& label) {
        std::cout << "[PASS] " << label << '\n';
        ++passed;
    };
    auto fail = [&](const std::string& label, const std::string& reason = "") {
        std::cout << "[FAIL] " << label;
        if (!reason.empty()) std::cout << " -- " << reason;
        std::cout << '\n';
        ++failed;
    };

    DFSGenerator     dfsGen;
    KruskalGenerator kruskalGen;
    BFSSolver        bfsSolver;
    AStarSolver      astarSolver;

    // BFS reachability count helper
    auto countReachable = [](const Maze& maze, const Coord& from) -> int {
        const int rows = maze.rows();
        const int cols = maze.cols();
        std::vector<std::vector<bool>> vis(rows, std::vector<bool>(cols, false));
        std::queue<Coord> q;
        q.push(from);
        vis[from.row][from.col] = true;
        int cnt = 0;
        while (!q.empty()) {
            Coord cur = q.front(); q.pop();
            ++cnt;
            for (int d = 0; d < 4; ++d) {
                if (!maze.canMove(cur, d)) continue;
                Coord nb = maze.neighbour(cur, d);
                if (!vis[nb.row][nb.col]) {
                    vis[nb.row][nb.col] = true;
                    q.push(nb);
                }
            }
        }
        return cnt;
    };

    // Count open inter-cell passages (rightward + downward only, no duplicates)
    auto countOpenPassages = [](const Maze& maze) -> int {
        int cnt = 0;
        const int rows = maze.rows();
        const int cols = maze.cols();
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c) {
                if (c + 1 < cols && !maze.cell(r, c).wallRight)  ++cnt;
                if (r + 1 < rows && !maze.cell(r, c).wallBottom) ++cnt;
            }
        return cnt;
    };

    struct TC { int rows, cols; unsigned seed; };
    std::vector<TC> cases = {
        {5,  5,  42},
        {10, 10, 123},
        {2,  2,  7},
        {15, 20, 999},
        {3,  8,  555},
    };

    // ── T1: DFS generation ──────────────────────────────────
    {
        bool ok = true; std::string reason;
        for (const auto& tc : cases) {
            Maze m; m.init(tc.rows, tc.cols);
            std::mt19937 rng(tc.seed);
            dfsGen.generate(m, rng);

            if (m.rows() != tc.rows || m.cols() != tc.cols)
                { ok=false; reason="Wrong dimensions"; break; }
            if (!m.inBounds(m.start()) || !m.inBounds(m.end()))
                { ok=false; reason="Start/end OOB"; break; }
            int reach = countReachable(m, m.start());
            if (reach != tc.rows * tc.cols)
                { ok=false; reason="Not fully connected ("+std::to_string(reach)+"/"+std::to_string(tc.rows*tc.cols)+")"; break; }
            if (!m.wallsConsistent())
                { ok=false; reason="Inconsistent shared walls"; break; }
            if (!m.outerBoundaryIntact())
                { ok=false; reason="Outer boundary breached"; break; }
            if (!m.isGenerated())
                { ok=false; reason="markGenerated() not called"; break; }
        }
        ok ? pass("DFS generation: dimensions, connectivity, wall consistency")
           : fail("DFS generation: dimensions, connectivity, wall consistency", reason);
    }

    // ── T2: Kruskal generation ──────────────────────────────
    {
        bool ok = true; std::string reason;
        for (const auto& tc : cases) {
            Maze m; m.init(tc.rows, tc.cols);
            std::mt19937 rng(tc.seed);
            kruskalGen.generate(m, rng);

            int reach = countReachable(m, m.start());
            if (reach != tc.rows * tc.cols)
                { ok=false; reason="Not fully connected"; break; }
            if (!m.wallsConsistent())
                { ok=false; reason="Inconsistent shared walls"; break; }
            if (!m.outerBoundaryIntact())
                { ok=false; reason="Outer boundary breached"; break; }
        }
        ok ? pass("Kruskal generation: dimensions, connectivity, wall consistency")
           : fail("Kruskal generation: dimensions, connectivity, wall consistency", reason);
    }

    // ── T3: Kruskal perfect maze (N-1 passages) ─────────────
    {
        bool ok = true; std::string reason;
        for (const auto& tc : cases) {
            Maze m; m.init(tc.rows, tc.cols);
            std::mt19937 rng(tc.seed);
            kruskalGen.generate(m, rng);
            int N = tc.rows * tc.cols;
            int p = countOpenPassages(m);
            if (p != N - 1)
                { ok=false; reason="Expected "+std::to_string(N-1)+" passages, got "+std::to_string(p); break; }
        }
        ok ? pass("Kruskal: exactly N-1 open passages (perfect maze)")
           : fail("Kruskal: exactly N-1 open passages (perfect maze)", reason);
    }

    // ── T4: Shared-wall consistency, both generators ─────────
    {
        bool ok = true; std::string reason;
        MazeGenerator* gens[2] = {&dfsGen, &kruskalGen};
        for (auto* gen : gens) {
            for (const auto& tc : cases) {
                Maze m; m.init(tc.rows, tc.cols);
                std::mt19937 rng(tc.seed);
                gen->generate(m, rng);
                if (!m.wallsConsistent())
                    { ok=false; reason=gen->name()+" inconsistent walls"; break; }
            }
            if (!ok) break;
        }
        ok ? pass("Shared-wall consistency (both generators)")
           : fail("Shared-wall consistency (both generators)", reason);
    }

    // ── T5: BFS path validity ───────────────────────────────
    {
        bool ok = true; std::string reason;
        for (const auto& tc : cases) {
            Maze m; m.init(tc.rows, tc.cols);
            std::mt19937 rng(tc.seed);
            dfsGen.generate(m, rng);

            SolveResult res = bfsSolver.solve(m, m.start(), m.end());
            if (!res.solved)
                { ok=false; reason="BFS failed on connected maze"; break; }
            if (res.path.front() != m.start())
                { ok=false; reason="Path doesn't start at start"; break; }
            if (res.path.back() != m.end())
                { ok=false; reason="Path doesn't end at end"; break; }
            if (res.pathLength != (int)res.path.size() - 1)
                { ok=false; reason="pathLength != path.size()-1"; break; }
            bool pathOk = true;
            for (int i = 1; i < (int)res.path.size(); ++i) {
                Coord a = res.path[i-1], b = res.path[i];
                int dr = b.row-a.row, dc = b.col-a.col;
                int dir=-1;
                if(dr==-1&&dc==0)dir=0; else if(dr==0&&dc==1)dir=1;
                else if(dr==1&&dc==0)dir=2; else if(dr==0&&dc==-1)dir=3;
                if(dir<0||!m.canMove(a,dir)){pathOk=false;break;}
            }
            if (!pathOk) { ok=false; reason="BFS path has invalid step"; break; }
            if (!m.wallsConsistent()) { ok=false; reason="BFS modified walls"; break; }
        }
        ok ? pass("BFS path validity")
           : fail("BFS path validity", reason);
    }

    // ── T6: A* path validity ────────────────────────────────
    {
        bool ok = true; std::string reason;
        for (const auto& tc : cases) {
            Maze m; m.init(tc.rows, tc.cols);
            std::mt19937 rng(tc.seed);
            dfsGen.generate(m, rng);

            SolveResult res = astarSolver.solve(m, m.start(), m.end());
            if (!res.solved)
                { ok=false; reason="A* failed on connected maze"; break; }
            if (res.path.front() != m.start())
                { ok=false; reason="Path doesn't start at start"; break; }
            if (res.path.back() != m.end())
                { ok=false; reason="Path doesn't end at end"; break; }
            if (res.pathLength != (int)res.path.size() - 1)
                { ok=false; reason="pathLength != path.size()-1"; break; }
            bool pathOk = true;
            for (int i = 1; i < (int)res.path.size(); ++i) {
                Coord a = res.path[i-1], b = res.path[i];
                int dr = b.row-a.row, dc = b.col-a.col;
                int dir=-1;
                if(dr==-1&&dc==0)dir=0; else if(dr==0&&dc==1)dir=1;
                else if(dr==1&&dc==0)dir=2; else if(dr==0&&dc==-1)dir=3;
                if(dir<0||!m.canMove(a,dir)){pathOk=false;break;}
            }
            if (!pathOk) { ok=false; reason="A* path has invalid step"; break; }
            if (!m.wallsConsistent()) { ok=false; reason="A* modified walls"; break; }
        }
        ok ? pass("A* path validity")
           : fail("A* path validity", reason);
    }

    // ── T7: BFS/A* shortest-path agreement ─────────────────
    {
        bool ok = true; std::string reason;
        for (const auto& tc : cases) {
            Maze m; m.init(tc.rows, tc.cols);
            std::mt19937 rng(tc.seed);
            dfsGen.generate(m, rng);

            SolveResult bfs   = bfsSolver.solve(m, m.start(), m.end());
            SolveResult astar = astarSolver.solve(m, m.start(), m.end());

            if (bfs.solved != astar.solved)
                { ok=false; reason="One solver failed; other succeeded"; break; }
            if (bfs.solved && bfs.pathLength != astar.pathLength)
                { ok=false; reason="Lengths differ: BFS="+std::to_string(bfs.pathLength)+" A*="+std::to_string(astar.pathLength); break; }
        }
        ok ? pass("BFS/A* shortest-path agreement")
           : fail("BFS/A* shortest-path agreement", reason);
    }

    // ── T8: Unreachable destination ─────────────────────────
    {
        bool ok = true; std::string reason;
        Maze m; m.init(5, 5);
        m.markGenerated();  // walls all closed; no passages opened

        SolveResult bfs   = bfsSolver.solve(m, {0,0}, {4,4});
        SolveResult astar = astarSolver.solve(m, {0,0}, {4,4});

        if (bfs.solved)
            { ok=false; reason="BFS reported a path in fully-walled maze"; }
        else if (astar.solved)
            { ok=false; reason="A* reported a path in fully-walled maze"; }
        else if (!bfs.path.empty())
            { ok=false; reason="BFS returned non-empty path on failure"; }
        else if (!astar.path.empty())
            { ok=false; reason="A* returned non-empty path on failure"; }

        ok ? pass("Unreachable-destination handling (fully-walled maze)")
           : fail("Unreachable-destination handling (fully-walled maze)", reason);
    }

    // ── T9: Player collision checks ─────────────────────────
    {
        bool ok = true; std::string reason;
        Maze m; m.init(5, 5);
        std::mt19937 rng(42);
        dfsGen.generate(m, rng);

        // 9a: legal move increments counter
        Player p; p.init(m.start());
        bool moved = false;
        for (int d = 0; d < 4 && !moved; ++d) {
            if (m.canMove(p.pos(), d)) {
                Coord before = p.pos();
                bool res = p.move(m, d);
                if (res && p.pos() != before && p.moveCount() == 1)
                    moved = true;
            }
        }
        if (!moved) { ok=false; reason="No legal move found from start"; }

        // 9b: outer walls at (0,0) — top and left always blocked
        if (ok) {
            Player p2; p2.init({0,0});
            int bc = p2.moveCount();
            Coord bp = p2.pos();
            p2.move(m, 0);  // up — always blocked by outer wall
            if (p2.pos() != bp || p2.moveCount() != bc)
                { ok=false; reason="Up from (0,0) was not rejected"; }
        }
        if (ok) {
            Player p3; p3.init({0,0});
            int bc = p3.moveCount();
            p3.move(m, 3);  // left — always blocked by outer wall
            if (p3.pos() != Coord{0,0} || p3.moveCount() != bc)
                { ok=false; reason="Left from (0,0) was not rejected"; }
        }

        // 9c: victory detection
        if (ok) {
            Player p4; p4.init(m.end());
            p4.checkVictory(m.end());
            if (!p4.reachedEnd())
                { ok=false; reason="Victory not detected at end cell"; }
        }

        // 9d: wall-blocked move doesn't increment counter
        if (ok) {
            // Find a wall that is closed
            Player p5; p5.init({0,0});
            bool foundBlocked = false;
            for (int d = 0; d < 4; ++d) {
                if (!m.canMove({0,0}, d)) {
                    int bc = p5.moveCount();
                    Coord bp = p5.pos();
                    p5.move(m, d);
                    if (p5.moveCount() != bc || p5.pos() != bp) {
                        ok=false; reason="Blocked move changed state";
                    }
                    foundBlocked = true;
                    break;
                }
            }
            if (ok && !foundBlocked)
                { ok=false; reason="No blocked direction found at (0,0) — unexpected"; }
        }

        ok ? pass("Player collision checks")
           : fail("Player collision checks", reason);
    }

    // ── T10: Edge-case maze sizes ───────────────────────────
    {
        bool ok = true; std::string reason;
        struct EC { int r, c; };
        std::vector<EC> ecs = {{2,2},{2,10},{10,2},{2,3},{3,2}};
        MazeGenerator* gens[2] = {&dfsGen, &kruskalGen};

        for (auto& ec : ecs) {
            for (auto* gen : gens) {
                Maze m; m.init(ec.r, ec.c);
                std::mt19937 rng(99);
                gen->generate(m, rng);

                if (!m.wallsConsistent())
                    { ok=false; reason=gen->name()+" inconsistent on "+std::to_string(ec.r)+"x"+std::to_string(ec.c); break; }

                SolveResult br = bfsSolver.solve(m, m.start(), m.end());
                if (!br.solved)
                    { ok=false; reason="BFS failed on "+std::to_string(ec.r)+"x"+std::to_string(ec.c); break; }

                SolveResult ar = astarSolver.solve(m, m.start(), m.end());
                if (!ar.solved || ar.pathLength != br.pathLength)
                    { ok=false; reason="A*/BFS mismatch on "+std::to_string(ec.r)+"x"+std::to_string(ec.c); break; }
            }
            if (!ok) break;
        }
        ok ? pass("Edge-case maze sizes (2x2, 2x10, 10x2, 2x3, 3x2)")
           : fail("Edge-case maze sizes (2x2, 2x10, 10x2, 2x3, 3x2)", reason);
    }

    // ── T11: Outer boundary intact ──────────────────────────
    {
        bool ok = true; std::string reason;
        MazeGenerator* gens[2] = {&dfsGen, &kruskalGen};
        for (auto* gen : gens) {
            for (const auto& tc : cases) {
                Maze m; m.init(tc.rows, tc.cols);
                std::mt19937 rng(tc.seed);
                gen->generate(m, rng);
                if (!m.outerBoundaryIntact())
                    { ok=false; reason=gen->name()+" broke outer boundary"; break; }
            }
            if (!ok) break;
        }
        ok ? pass("Outer boundary intact after generation (both generators)")
           : fail("Outer boundary intact after generation (both generators)", reason);
    }

    // ── T12: DSU correctness ────────────────────────────────
    {
        bool ok = true; std::string reason;
        DSU dsu(10);

        for (int i = 0; i < 10 && ok; ++i)
            for (int j = i+1; j < 10 && ok; ++j)
                if (dsu.connected(i, j))
                    { ok=false; reason="Elements start connected"; }

        if (ok) {
            dsu.unite(0, 1);
            dsu.unite(2, 3);
            dsu.unite(1, 2);
            if (!dsu.connected(0, 3))
                { ok=false; reason="Transitivity failed"; }
            else if (dsu.connected(0, 4))
                { ok=false; reason="Unrelated elements appear connected"; }
            else if (dsu.unite(0, 3))
                { ok=false; reason="unite() returned true for already-connected"; }
        }

        ok ? pass("DSU correctness (path compression, union-by-rank)")
           : fail("DSU correctness (path compression, union-by-rank)", reason);
    }

    // ── T13: Solvers do not modify walls ────────────────────
    {
        bool ok = true; std::string reason;
        Maze m; m.init(8, 8);
        std::mt19937 rng(314);
        dfsGen.generate(m, rng);

        struct WSnap { bool t,b,l,r; };
        std::vector<std::vector<WSnap>> snap(8, std::vector<WSnap>(8));
        for (int r = 0; r < 8; ++r)
            for (int c = 0; c < 8; ++c) {
                const Cell& cl = m.cell(r,c);
                snap[r][c] = {cl.wallTop, cl.wallBottom, cl.wallLeft, cl.wallRight};
            }

        bfsSolver.solve(m, m.start(), m.end());
        astarSolver.solve(m, m.start(), m.end());

        for (int r = 0; r < 8 && ok; ++r)
            for (int c = 0; c < 8 && ok; ++c) {
                const Cell& cl = m.cell(r,c);
                if (cl.wallTop!=snap[r][c].t || cl.wallBottom!=snap[r][c].b ||
                    cl.wallLeft!=snap[r][c].l || cl.wallRight!=snap[r][c].r)
                    { ok=false; reason="Wall modified at ("+std::to_string(r)+","+std::to_string(c)+")"; }
            }

        ok ? pass("Solvers do not modify maze walls")
           : fail("Solvers do not modify maze walls", reason);
    }

    // ── Summary ─────────────────────────────────────────────
    std::cout << "\n-----------------------------------------\n";
    std::cout << "Total: " << passed << " passed, " << failed << " failed\n";
    std::cout << "=========================================\n\n";
}

// ============================================================
//  Gameplay session
// ============================================================
static void playMaze(Maze& maze, Player& player, const ConsoleRenderer& renderer) {
    player.init(maze.start());

    std::cout << "\n=== MAZE PLAY ===\n";
    std::cout << "Controls: W=Up  A=Left  S=Down  D=Right  Q=Quit\n\n";

    while (true) {
        renderer.render(maze, player);
        std::cout << "  Moves: " << player.moveCount()
                  << "  |  Time: " << std::fixed << std::setprecision(1)
                  << player.elapsedSeconds() << "s\n";

        if (player.reachedEnd()) {
            std::cout << "\n*** YOU REACHED THE EXIT! Congratulations! ***\n";
            break;
        }

        std::string line = readLine("  Command (W/A/S/D/Q): ");
        if (line.empty()) continue;
        char cmd = (char)std::toupper((unsigned char)line[0]);

        int dir = -1;
        switch (cmd) {
            case 'W': dir = 0; break;
            case 'D': dir = 1; break;
            case 'S': dir = 2; break;
            case 'A': dir = 3; break;
            case 'Q':
                std::cout << "  Quitting game session.\n";
                goto done;
            default:
                std::cout << "  Unknown command. Use W/A/S/D to move, Q to quit.\n";
                continue;
        }

        if (!player.move(maze, dir)) {
            std::cout << "  Move blocked by wall.\n";
        } else {
            player.checkVictory(maze.end());
        }
    }

done:
    double elapsed = player.elapsedSeconds();
    int    moves   = player.moveCount();
    bool   won     = player.reachedEnd();

    std::cout << "\n--- Session Summary ---\n";
    std::cout << "  Successful moves : " << moves << '\n';
    std::cout << "  Time elapsed     : " << std::fixed << std::setprecision(2)
              << elapsed << "s\n";
    std::cout << "  Destination      : " << (won ? "Reached" : "Not reached") << '\n';

    // Compute shortest path for comparison
    BFSSolver bfs;
    SolveResult shortest = bfs.solve(maze, maze.start(), maze.end());
    if (shortest.solved) {
        std::cout << "  Shortest route   : " << shortest.pathLength << " moves\n";
        if (won && moves > 0 && shortest.pathLength > 0)
            std::cout << "  Efficiency       : " << std::fixed << std::setprecision(1)
                      << (100.0 * shortest.pathLength / moves) << "%\n";
    }

    readLine("\n  Press Enter to return to menu...");
}

// ============================================================
//  Main application loop
// ============================================================
static void runMenu() {
    std::cout << "\n";
    std::cout << "  +======================================+\n";
    std::cout << "  |          MAZE LAB  --  Phase II      |\n";
    std::cout << "  +======================================+\n\n";

    int mazeRows = 10;
    int mazeCols = 10;

    Maze             maze;
    maze.init(mazeRows, mazeCols);

    Player           player;
    ConsoleRenderer  renderer;
    BFSSolver        bfsSolver;
    AStarSolver      astarSolver;
    DFSGenerator     dfsGen;
    KruskalGenerator kruskalGen;

    std::mt19937 rng(std::random_device{}());

    std::vector<Coord> currentPath;
    std::string genName = dfsGen.name();

    auto printHeader = [&]() {
        std::cout << "\n========================================\n";
        std::cout << "               MAZE LAB\n";
        std::cout << "========================================\n";
        std::cout << "  Maze Dimensions  : " << mazeRows << " x " << mazeCols << "\n";
        std::cout << "  Current Generator: " << genName << "\n";
        std::cout << "  Maze Status      : "
                  << (maze.isGenerated() ? "Generated" : "Not Generated") << "\n";
        std::cout << "----------------------------------------\n";
        std::cout << "  1. Generate Maze (Randomized DFS)\n";
        std::cout << "  2. Generate Maze (Kruskal's Algorithm)\n";
        std::cout << "  3. Play Maze\n";
        std::cout << "  4. Solve with BFS\n";
        std::cout << "  5. Solve with A*\n";
        std::cout << "  6. Compare BFS vs A*\n";
        std::cout << "  7. Change Maze Dimensions\n";
        std::cout << "  8. Run Built-in Validation Tests\n";
        std::cout << "  0. Exit\n";
        std::cout << "========================================\n";
    };

    while (true) {
        printHeader();
        int choice = readInt("  Enter your choice: ", 0, 8);

        switch (choice) {
            case 1: {
                dfsGen.generate(maze, rng);
                player.init(maze.start());
                currentPath.clear();
                genName = dfsGen.name();
                std::cout << "\n  Maze generated using " << dfsGen.name() << ".\n";
                renderer.renderNoPlayer(maze);
                readLine("  Press Enter to continue...");
                break;
            }
            case 2: {
                kruskalGen.generate(maze, rng);
                player.init(maze.start());
                currentPath.clear();
                genName = kruskalGen.name();
                std::cout << "\n  Maze generated using " << kruskalGen.name() << ".\n";
                renderer.renderNoPlayer(maze);
                readLine("  Press Enter to continue...");
                break;
            }
            case 3: {
                if (!maze.isGenerated()) {
                    std::cout << "\n  No maze generated. Use option 1 or 2 first.\n";
                    readLine("  Press Enter...");
                    break;
                }
                playMaze(maze, player, renderer);
                break;
            }
            case 4: {
                if (!maze.isGenerated()) {
                    std::cout << "\n  No maze generated. Use option 1 or 2 first.\n";
                    readLine("  Press Enter...");
                    break;
                }
                SolveResult res = bfsSolver.solve(maze, maze.start(), maze.end());
                printSolveResult(res, "BFS");
                if (res.solved) {
                    currentPath = res.path;
                    std::cout << "\n  Showing BFS solution path:\n";
                    renderer.renderNoPlayer(maze, currentPath);
                } else {
                    std::cout << "  Destination is unreachable.\n";
                }
                readLine("  Press Enter...");
                break;
            }
            case 5: {
                if (!maze.isGenerated()) {
                    std::cout << "\n  No maze generated. Use option 1 or 2 first.\n";
                    readLine("  Press Enter...");
                    break;
                }
                SolveResult res = astarSolver.solve(maze, maze.start(), maze.end());
                printSolveResult(res, "A*");
                if (res.solved) {
                    currentPath = res.path;
                    std::cout << "\n  Showing A* solution path:\n";
                    renderer.renderNoPlayer(maze, currentPath);
                } else {
                    std::cout << "  Destination is unreachable.\n";
                }
                readLine("  Press Enter...");
                break;
            }
            case 6: {
                if (!maze.isGenerated()) {
                    std::cout << "\n  No maze generated. Use option 1 or 2 first.\n";
                    readLine("  Press Enter...");
                    break;
                }
                std::cout << "\n  Running BFS and A* on the same maze...\n";
                SolveResult bfsRes   = bfsSolver.solve(maze, maze.start(), maze.end());
                SolveResult astarRes = astarSolver.solve(maze, maze.start(), maze.end());
                printComparisonTable(bfsRes, astarRes);

                if (bfsRes.solved || astarRes.solved) {
                    std::cout << "\n  Which path to display?\n";
                    std::cout << "    1. BFS path\n    2. A* path\n    3. None\n";
                    int v = readInt("  Choice: ", 1, 3);
                    if (v == 1 && bfsRes.solved) {
                        currentPath = bfsRes.path;
                        renderer.renderNoPlayer(maze, currentPath);
                    } else if (v == 2 && astarRes.solved) {
                        currentPath = astarRes.path;
                        renderer.renderNoPlayer(maze, currentPath);
                    }
                }
                readLine("  Press Enter...");
                break;
            }
            case 7: {
                mazeRows = readInt("  Rows    (2-50): ", 2, 50);
                mazeCols = readInt("  Columns (2-50): ", 2, 50);
                maze.init(mazeRows, mazeCols);
                player.init(maze.start());
                currentPath.clear();
                std::cout << "  Dimensions set to " << mazeRows << " x " << mazeCols
                          << ". Please generate a new maze.\n";
                readLine("  Press Enter...");
                break;
            }
            case 8: {
                runTests();
                readLine("  Press Enter to continue...");
                break;
            }
            case 0: {
                std::cout << "\n  Thank you for using Maze Lab. Goodbye!\n\n";
                return;
            }
        }
    }
}

// ============================================================
//  Entry point
// ============================================================
int main() {
    std::ios::sync_with_stdio(true);
    runMenu();
    return 0;
}
