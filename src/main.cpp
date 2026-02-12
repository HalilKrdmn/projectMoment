#include "gui/core/MainWindow.h"
#include "core/Config.h"

#include <iostream>

int main() {
    try {
        std::cout << "================================" << std::endl;
        std::cout << " ProjectMoment Starting...      " << std::endl;
        std::cout << "================================" << std::endl;

        // Check config
        if (!Config::InitializeOrCreateConfig()) return -1;

        // Create MainWindow
        const MainWindow window(1280, 720, "ProjectMoment");
        const int result = window.Run();

        std::cout << "[Main] Goodbye!" << std::endl;
        return result;
    } catch (const std::exception& e) {
        std::cerr << "\n[FATAL ERROR] " << e.what() << std::endl;
        return -1;
    }
}