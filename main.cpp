#include "gaussian_extractor.h"
#include <string>
#include <iostream>
#include <thread>
#include <algorithm> // For std::max
#include <vector>

int main(int argc, char* argv[]) {
    double temp = 298.15;
    int C = 1000;
    int column = 2;
    std::string extension = ".log";
    bool quiet = false;
    std::string format = "text";
    bool use_input_temp = false;  // Default to false (read from files)
    unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency() / 2);
    std::vector<std::string> warnings;

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-t" || arg == "--temp") {
                if (++i < argc) {
                    temp = std::stod(argv[i]);
                    use_input_temp = true;
                }
            } else if (arg == "-c" || arg == "--cm") {
                if (++i < argc) C = std::stoi(argv[i]) * 1000;
            } else if (arg == "-col" || arg == "--column") {
                if (++i < argc) column = std::stoi(argv[i]);
            } else if (arg == "-e" || arg == "--ext") {
                if (++i < argc) {
                    extension = "." + std::string(argv[i]);
                    if (extension != ".log" && extension != ".out") {
                        warnings.push_back("Error: Extension must be 'log' or 'out'. Using default '.log'.");
                        extension = ".log";
                    }
                }
            } else if (arg == "-q" || arg == "--quiet") {
                quiet = true;
            } else if (arg == "-f" || arg == "--format") {
                if (++i < argc) {
                    format = argv[i];
                    if (format != "text" && format != "csv") {
                        warnings.push_back("Error: Format must be 'text' or 'csv'. Using default 'text'.");
                        format = "text";
                    }
                }
            } else if (arg == "-nt" || arg == "--threads") {
                if (++i < argc) {
                    std::string threads_arg = argv[i];
                    unsigned int hardware_cores = std::thread::hardware_concurrency();

                    if (threads_arg == "max") {
                        num_threads = hardware_cores;
                    } else if (threads_arg == "half") {
                        num_threads = std::max(1u, hardware_cores / 2);
                    } else {
                        try {
                            unsigned int requested_threads = std::stoul(threads_arg);
                            if (requested_threads > hardware_cores) {
                                std::string warning_msg = "Warning: Requested " + std::to_string(requested_threads) + " threads, but only " + std::to_string(hardware_cores) + " are available. Using all available cores.";
                                warnings.push_back(warning_msg);
                                num_threads = hardware_cores;
                            } else {
                                num_threads = std::max(1u, requested_threads);
                            }
                        } catch (const std::invalid_argument& e) {
                            warnings.push_back("Error: Invalid argument for --threads. Please provide a number, 'max', or 'half'.");
                        } catch (const std::out_of_range& e) {
                            warnings.push_back("Error: Number of threads is out of range.");
                        }
                    }

                    if (num_threads == hardware_cores) {
                        warnings.push_back("Warning: Using all available hardware cores (" + std::to_string(num_threads) + "). This can slow down the system, especially if this is a shared head node.");
                    }
                }
            }
        }
    }

    processAndOutputResults(temp, C, column, extension, quiet, format, use_input_temp, num_threads, warnings);
    return 0;
}