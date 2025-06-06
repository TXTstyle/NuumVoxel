#include "Nuum.hpp"
#include <iostream>

int main(int argc, char** argv) {
    Nuum nuum;
    if (nuum.Init(argc, argv) != 0) {
        std::cerr << "Failed to initialize Nuum!" << std::endl;
        return 1;
    }
    nuum.Run();
    nuum.Shutdown();
    return 0;
}
