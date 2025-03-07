#ifndef GAUSSIAN_EXTRACTOR_H
#define GAUSSIAN_EXTRACTOR_H

#include <string>
#include <vector>

// Constants
extern const double R;           // J/K/mol
extern const double Po;          // 1 atm in N/m^2
extern const double kB;          // Boltzmann constant in Hartree/K

struct Result {
    std::string file_name;
    double etgkj;
    double lf;
    double GibbsFreeHartree;
    double nucleare;
    double scf;
    double zpe;
    std::string status;
    std::string phaseCorr;
    int copyright_count;
};

// Function declarations
bool compareResults(const Result& a, const Result& b, int column);
Result extract(const std::string& file_name_param, double temp, int C, double Po);
void processAndOutputResults(double temp, int C, int column);

#endif // GAUSSIAN_EXTRACTOR_H