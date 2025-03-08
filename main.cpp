#include "gaussian_extractor.h"
#include <string>
#include <iostream>

int main(int argc, char* argv[]) {
    double temp = 298.15;
    int C = 1000;
    int column = 2;
    std::string extension = ".log";  // Default extension

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-t" || arg == "--temp") {
                if (++i < argc) temp = std::stod(argv[i]);
            } else if (arg == "-c" || arg == "--cm") {
                if (++i < argc) C = std::stoi(argv[i]) * 1000;
            } else if (arg == "-col" || arg == "--column") {
                if (++i < argc) column = std::stoi(argv[i]);
            } else if (arg == "-e" || arg == "--ext") {
                if (++i < argc) {
                    extension = "." + std::string(argv[i]);
                    if (extension != ".log" && extension != ".out") {
                        std::cerr << "Error: Extension must be 'log' or 'out'. Using default '.log'." << std::endl;
                        extension = ".log";
                    }
                }
            }
        }
    }

    processAndOutputResults(temp, C, column, extension);
    return 0;
}