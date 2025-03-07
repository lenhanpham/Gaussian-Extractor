#include "gaussian_extractor.h"
#include <string>

int main(int argc, char* argv[]) {
    double temp = 298.15;
    int C = 1000;
    int column = 2;

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-t" || arg == "--temp") {
                temp = std::stod(argv[++i]);
            } else if (arg == "-c" || arg == "--cm") {
                C = std::stoi(argv[++i]) * 1000;
            } else if (arg == "-col" || arg == "--column") {
                column = std::stoi(argv[++i]);
            }
        }
    }

    processAndOutputResults(temp, C, column);
    return 0;
}