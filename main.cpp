#include "gaussian_extractor.h"
#include "job_scheduler.h"
#include <string>
#include <iostream>
#include <thread>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <limits>

// Environment detection for cluster safety - now uses JobSchedulerDetector
bool isClusterEnvironment() {
    return JobSchedulerDetector::is_running_in_job();
}

int main(int argc, char* argv[]) {
    double temp = 298.15;
    int C = 1000;
    int column = 2;
    std::string extension = ".log";
    bool quiet = false;
    std::string format = "text";
    bool use_input_temp = false;
    unsigned int requested_threads = 0; // 0 means use default
    size_t max_file_size_mb = DEFAULT_MAX_FILE_SIZE_MB;
    size_t memory_limit_mb = 0; // 0 means auto-calculate
    std::vector<std::string> warnings;
    bool show_resource_info = false;

    // Parse command line arguments
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "-t" || arg == "--temp") {
                if (++i < argc) {
                    try {
                        temp = std::stod(argv[i]);
                        if (temp <= 0) {
                            warnings.push_back("Warning: Temperature must be positive. Using default 298.15 K.");
                            temp = 298.15;
                        } else {
                            use_input_temp = true;
                        }
                    } catch (const std::exception& e) {
                        warnings.push_back("Error: Invalid temperature format. Using default 298.15 K.");
                        temp = 298.15;
                    }
                } else {
                    warnings.push_back("Error: Temperature value required after -t/--temp.");
                }
            } 
            else if (arg == "-c" || arg == "--cm") {
                if (++i < argc) {
                    try {
                        int conc = std::stoi(argv[i]);
                        if (conc <= 0) {
                            warnings.push_back("Error: Concentration must be positive. Using default 1 M.");
                        } else {
                            C = conc * 1000;
                        }
                    } catch (const std::exception& e) {
                        warnings.push_back("Error: Invalid concentration format. Using default 1 M.");
                    }
                } else {
                    warnings.push_back("Error: Concentration value required after -c/--cm.");
                }
            } 
            else if (arg == "-col" || arg == "--column") {
                if (++i < argc) {
                    try {
                        int col = std::stoi(argv[i]);
                        if (col >= 2 && col <= 10) {
                            column = col;
                        } else {
                            warnings.push_back("Error: Column must be between 2-10. Using default column 2.");
                        }
                    } catch (const std::exception& e) {
                        warnings.push_back("Error: Invalid column format. Using default column 2.");
                    }
                } else {
                    warnings.push_back("Error: Column value required after -col/--column.");
                }
            } 
            else if (arg == "-e" || arg == "--ext") {
                if (++i < argc) {
                    std::string ext = argv[i];
                    if (ext == "log" || ext == "out") {
                        extension = "." + ext;
                    } else {
                        warnings.push_back("Error: Extension must be 'log' or 'out'. Using default '.log'.");
                        extension = ".log";
                    }
                } else {
                    warnings.push_back("Error: Extension value required after -e/--ext.");
                }
            } 
            else if (arg == "-q" || arg == "--quiet") {
                quiet = true;
            } 
            else if (arg == "-f" || arg == "--format") {
                if (++i < argc) {
                    std::string fmt = argv[i];
                    if (fmt == "text" || fmt == "csv") {
                        format = fmt;
                    } else {
                        warnings.push_back("Error: Format must be 'text' or 'csv'. Using default 'text'.");
                        format = "text";
                    }
                } else {
                    warnings.push_back("Error: Format value required after -f/--format.");
                }
            } 
            else if (arg == "-nt" || arg == "--threads") {
                if (++i < argc) {
                    std::string threads_arg = argv[i];
                    unsigned int hardware_cores = std::thread::hardware_concurrency();
                    if (hardware_cores == 0) hardware_cores = 4;

                    if (threads_arg == "max") {
                        requested_threads = hardware_cores;
                    } else if (threads_arg == "half") {
                        requested_threads = std::max(1u, hardware_cores / 2);
                    } else {
                        try {
                            unsigned int req_threads = std::stoul(threads_arg);
                            if (req_threads == 0) {
                                warnings.push_back("Error: Thread count must be at least 1. Using default.");
                                requested_threads = 0; // Use default
                            } else {
                                requested_threads = req_threads;
                            }
                        } catch (const std::exception& e) {
                            warnings.push_back("Error: Invalid thread count format. Using default.");
                            requested_threads = 0; // Use default
                        }
                    }
                } else {
                    warnings.push_back("Error: Thread count required after -nt/--threads.");
                }
            }
            else if (arg == "--memory-limit") {
                if (++i < argc) {
                    try {
                        int size = std::stoi(argv[i]);
                        if (size <= 0) {
                            warnings.push_back("Error: Memory limit must be positive. Using auto-calculated limit.");
                        } else {
                            memory_limit_mb = static_cast<size_t>(size);
                        }
                    } catch (const std::exception& e) {
                        warnings.push_back("Error: Invalid memory limit format. Using auto-calculated limit.");
                    }
                } else {
                    warnings.push_back("Error: Memory limit value required after --memory-limit.");
                }
            }
            else if (arg == "--resource-info") {
                show_resource_info = true;
            }
            else if (arg == "--test-threads") {
                // Test thread calculation and exit
                JobResources job_resources = JobSchedulerDetector::detect_job_resources();
                unsigned int hardware_cores = std::thread::hardware_concurrency();
                if (hardware_cores == 0) hardware_cores = 4;
                
                std::cout << "\n=== Thread Calculation Test ===" << std::endl;
                std::cout << "Hardware cores: " << hardware_cores << std::endl;
                std::cout << "Default request (half): " << (hardware_cores / 2) << std::endl;
                std::cout << "Job scheduler: " << JobSchedulerDetector::scheduler_name(job_resources.scheduler_type) << std::endl;
                
                if (job_resources.has_cpu_limit) {
                    std::cout << "Job CPU limit: " << job_resources.allocated_cpus << std::endl;
                }
                
                // Test different thread requests
                std::vector<unsigned int> test_requests = {1, hardware_cores/4, hardware_cores/2, hardware_cores, hardware_cores*2};
                
                for (unsigned int req : test_requests) {
                    unsigned int result = calculateSafeThreadCount(req, 100, job_resources);
                    std::cout << "Request " << req << " threads -> get " << result << " threads" << std::endl;
                }
                
                std::cout << "===============================" << std::endl;
                return 0;
            }
            else if (arg == "--max-file-size") {
                if (++i < argc) {
                    try {
                        int size = std::stoi(argv[i]);
                        if (size <= 0) {
                            warnings.push_back("Error: Max file size must be positive. Using default 100MB.");
                        } else {
                            max_file_size_mb = static_cast<size_t>(size);
                        }
                    } catch (const std::exception& e) {
                        warnings.push_back("Error: Invalid max file size format. Using default 100MB.");
                    }
                } else {
                    warnings.push_back("Error: Max file size value required after --max-file-size.");
                }
            }
            else if (arg == "-h" || arg == "--help") {
                std::cout << "Gaussian Extractor - Parallel Energy Extraction Tool\n\n";
                std::cout << "Usage: " << argv[0] << " [OPTIONS]\n\n";
                std::cout << "Options:\n";
                std::cout << "  -t, --temp <K>        Temperature in Kelvin (default: 298.15)\n";
                std::cout << "  -c, --cm <M>          Concentration in M (default: 1)\n";
                std::cout << "  -col, --column <N>    Sort column 2-10 (default: 2)\n";
                std::cout << "  -e, --ext <ext>       File extension: log|out (default: log)\n";
                std::cout << "  -f, --format <fmt>    Output format: text|csv (default: text)\n";
                std::cout << "  -nt, --threads <N>    Thread count: number|max|half (default: half)\n";
                std::cout << "  -q, --quiet           Quiet mode (minimal output)\n";
                std::cout << "  --max-file-size <MB>  Maximum file size in MB (default: 100)\n";
                std::cout << "  --memory-limit <MB>   Memory limit in MB (default: auto-calculated)\n";
                std::cout << "  --resource-info       Show resource information\n";
                std::cout << "  --test-threads        Test thread calculation and exit\n";
                std::cout << "  -h, --help            Show this help message\n\n";
                std::cout << "Safety Features:\n";
                std::cout << "  - Automatic cluster environment detection\n";
                std::cout << "  - Memory usage monitoring and limits\n";
                std::cout << "  - File descriptor management\n";
                std::cout << "  - Resource-aware thread limiting\n\n";
                return 0;
            }
            else {
                warnings.push_back("Warning: Unknown argument '" + arg + "' ignored.");
            }
        }
    }

    // Set default threads if not specified
    if (requested_threads == 0) {
        unsigned int hardware_cores = std::thread::hardware_concurrency();
        if (hardware_cores == 0) hardware_cores = 4;
        requested_threads = std::max(1u, hardware_cores / 2); // Keep default as half
    }

    // Show resource information if requested
    if (show_resource_info) {
        JobResources job_resources = JobSchedulerDetector::detect_job_resources();
        unsigned int hardware_cores = std::thread::hardware_concurrency();
        
        std::cout << "\n=== System Resource Information ===\n";
        std::cout << "Hardware cores detected: " << hardware_cores << "\n";
        std::cout << "System memory: " << MemoryMonitor::get_system_memory_mb() << " MB\n";
        std::cout << "Requested threads: " << requested_threads << "\n";
        if (memory_limit_mb > 0) {
            std::cout << "Memory limit: " << memory_limit_mb << " MB (user-specified)\n";
        } else {
            std::cout << "Memory limit: Auto-calculated based on threads and system memory\n";
        }
        
        // Job scheduler information
        if (job_resources.scheduler_type != SchedulerType::NONE) {
            std::cout << "\n=== Job Scheduler Information ===\n";
            std::cout << "Scheduler: " << JobSchedulerDetector::scheduler_name(job_resources.scheduler_type) << "\n";
            std::cout << "Job ID: " << job_resources.job_id << "\n";
            if (job_resources.has_cpu_limit) {
                std::cout << "Job allocated CPUs: " << job_resources.allocated_cpus << "\n";
            }
            if (job_resources.has_memory_limit) {
                std::cout << "Job allocated memory: " << job_resources.allocated_memory_mb << " MB\n";
            }
            if (!job_resources.partition.empty()) {
                std::cout << "Partition/Queue: " << job_resources.partition << "\n";
            }
        } else {
            std::cout << "Job scheduler: None detected\n";
        }
        
        // Note: Final thread count will be determined after file counting
        std::cout << "=====================================\n\n";
    }

    // Additional safety warnings
    if (isClusterEnvironment()) {
        if (requested_threads > std::thread::hardware_concurrency() / 4) {
            warnings.push_back("Warning: Running on cluster with high thread count. Consider using fewer threads to avoid overloading shared resources.");
        }
    }

    // Detect job scheduler resources
    JobResources job_resources = JobSchedulerDetector::detect_job_resources();
    
    // Process files with all safety features including job scheduler awareness
    try {
        processAndOutputResults(temp, C, column, extension, quiet, format, 
                              use_input_temp, requested_threads, max_file_size_mb, memory_limit_mb, warnings, job_resources);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }

    return 0;
}