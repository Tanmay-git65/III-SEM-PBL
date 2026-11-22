#include <iostream>
#include <string>
#include "session.hpp"
#include "console_ui.hpp"
#include "gui_window.hpp"
#include "benchmark.hpp"

void openGui(MazeLabSession& session) {
    GuiApp app(session);
    std::cout << "\nOpening the Maze Lab window. Close it to return to this menu.\n";
    if (runGuiWindow(app) != 0)
        std::cout << "The graphical window could not be opened on this system.\n";
}

void runMenu(MazeLabSession& session) {
    ConsoleApp console(session);
    while (true) {
        console.showMenu();
        int choice = readNumber("Enter your choice: ", 0, 9);
        if (!std::cin || choice == 0) {
            std::cout << "Goodbye!\n";
            return;
        }
        switch (choice) {
            case 1: console.generateAndShow(session.dfsGenerator); break;
            case 2: console.generateAndShow(session.kruskalGenerator); break;
            case 3: console.playMaze(); break;
            case 4: console.solveAndShow(session.bfsSolver); break;
            case 5: console.solveAndShow(session.aStarSolver); break;
            case 6: console.compareSolvers(); break;
            case 7: console.changeSize(); break;
            case 8: console.runTests(); break;
            case 9: openGui(session); break;
        }
    }
}

int main(int argc, char* argv[]) {
    MazeLabSession session;
    std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "--gui") {
        openGui(session);
        return 0;
    }
    if (mode == "--benchmark") {
        runBenchmark(std::cout);
        return 0;
    }
    runMenu(session);
    return 0;
}
