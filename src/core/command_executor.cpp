#include "command_system.h"
#include "gaussian_extractor.h"
#include "job_checker.h"
#include "high_level_energy.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <memory>
#include <csignal>
#include <thread>
#include <string>
#include <atomic>

// External global variable for shutdown handling
extern std::atomic<bool> g_shutdown_requested;

// Signal handler function
void signal_handler_func(int signal) {
    std::cerr << "\nReceived signal " << signal << ". Initiating graceful shutdown..." << std::endl;
    g_shutdown_requested.store(true);
}

// Signal handler setup
void setup_signal_handlers() {
    std::signal(SIGINT, signal_handler_func);
    std::signal(SIGTERM, signal_handler_func);
}

int execute_extract_command(const CommandContext& context) {
    setup_signal_handlers();
    
    // Show warnings if any
    if (!context.warnings.empty() && !context.quiet) {
        for (const auto& warning : context.warnings) {
            std::cerr << warning << std::endl;
        }
        std::cerr << std::endl;
    }
    
    // Show resource information if requested
    if (context.show_resource_info) {
        JobResources job_resources = context.job_resources;
        unsigned int hardware_cores = std::thread::hardware_concurrency();
        
        std::cout << "\n=== System Resource Information ===\n";
        std::cout << "Hardware cores detected: " << hardware_cores << "\n";
        std::cout << "System memory: " << MemoryMonitor::get_system_memory_mb() << " MB\n";
        std::cout << "Requested threads: " << context.requested_threads << "\n";
        if (context.memory_limit_mb > 0) {
            std::cout << "Memory limit: " << context.memory_limit_mb << " MB (user-specified)\n";
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
        
        std::cout << "=====================================\n\n";
    }
    
    try {
        // Call the existing processAndOutputResults function
        processAndOutputResults(
            context.temp,
            context.concentration,
            context.sort_column,
            context.extension,
            context.quiet,
            context.output_format,
            context.use_input_temp,
            context.requested_threads,
            context.max_file_size_mb,
            context.memory_limit_mb,
            context.warnings,
            context.job_resources
        );
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_check_done_command(const CommandContext& context) {
    setup_signal_handlers();
    
    try {
        // Find log files
        std::vector<std::string> log_files = findLogFiles(context.extension, context.max_file_size_mb);
        
        if (log_files.empty()) {
            if (!context.quiet) {
                std::cout << "No " << context.extension << " files found in current directory." << std::endl;
            }
            return 0;
        }
        
        // Create processing context for resource management
        auto processing_context = std::make_shared<ProcessingContext>(
            298.15, // Temperature not needed for job checking
            1000,   // Concentration not needed for job checking
            context.use_input_temp,
            context.requested_threads,
            context.extension,
            context.job_resources
        );
        
        // Set memory limit if specified
        if (context.memory_limit_mb > 0) {
            processing_context->memory_monitor->set_memory_limit(context.memory_limit_mb);
        }
        
        // Create job checker
        JobChecker checker(processing_context, context.quiet, false);
        
        // Determine target directory suffix
        std::string dir_suffix = context.dir_suffix;
        if (!context.target_dir.empty()) {
            // If custom target directory is specified, use it as full name
            dir_suffix = context.target_dir;
        }
        
        // Check completed jobs
        CheckSummary summary = checker.check_completed_jobs(log_files, dir_suffix);
        
        // Print summary if not quiet
        if (!context.quiet) {
            checker.print_summary(summary, "Job completion check");
        }
        
        // Print any resource usage information
        if (!context.quiet) {
            printResourceUsage(*processing_context, context.quiet);
        }
        
        return (summary.errors.empty()) ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_check_errors_command(const CommandContext& context) {
    setup_signal_handlers();
    
    try {
        // Find log files
        std::vector<std::string> log_files = findLogFiles(context.extension, context.max_file_size_mb);
        
        if (log_files.empty()) {
            if (!context.quiet) {
                std::cout << "No " << context.extension << " files found in current directory." << std::endl;
            }
            return 0;
        }
        
        // Create processing context for resource management
        auto processing_context = std::make_shared<ProcessingContext>(
            298.15, // Temperature not needed for job checking
            1000,   // Concentration not needed for job checking
            context.use_input_temp,
            context.requested_threads,
            context.extension,
            context.job_resources
        );
        
        // Set memory limit if specified
        if (context.memory_limit_mb > 0) {
            processing_context->memory_monitor->set_memory_limit(context.memory_limit_mb);
        }
        
        // Create job checker
        JobChecker checker(processing_context, context.quiet, context.show_error_details);
        
        // Determine target directory
        std::string target_dir = "errorJobs";
        if (!context.target_dir.empty()) {
            target_dir = context.target_dir;
        }
        
        // Check error jobs
        CheckSummary summary = checker.check_error_jobs(log_files, target_dir);
        
        // Print summary if not quiet
        if (!context.quiet) {
            checker.print_summary(summary, "Error job check");
        }
        
        // Print any resource usage information
        if (!context.quiet) {
            printResourceUsage(*processing_context, context.quiet);
        }
        
        return (summary.errors.empty()) ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_check_pcm_command(const CommandContext& context) {
    setup_signal_handlers();
    
    try {
        // Find log files
        std::vector<std::string> log_files = findLogFiles(context.extension, context.max_file_size_mb);
        
        if (log_files.empty()) {
            if (!context.quiet) {
                std::cout << "No " << context.extension << " files found in current directory." << std::endl;
            }
            return 0;
        }
        
        // Create processing context for resource management
        auto processing_context = std::make_shared<ProcessingContext>(
            298.15, // Temperature not needed for job checking
            1000,   // Concentration not needed for job checking
            context.use_input_temp,
            context.requested_threads,
            context.extension,
            context.job_resources
        );
        
        // Set memory limit if specified
        if (context.memory_limit_mb > 0) {
            processing_context->memory_monitor->set_memory_limit(context.memory_limit_mb);
        }
        
        // Create job checker
        JobChecker checker(processing_context, context.quiet, false);
        
        // Determine target directory
        std::string target_dir = "PCMMkU";
        if (!context.target_dir.empty()) {
            target_dir = context.target_dir;
        }
        
        // Check PCM failed jobs
        CheckSummary summary = checker.check_pcm_failures(log_files, target_dir);
        
        // Print summary if not quiet
        if (!context.quiet) {
            checker.print_summary(summary, "PCM failure check");
        }
        
        // Print any resource usage information
        if (!context.quiet) {
            printResourceUsage(*processing_context, context.quiet);
        }
        
        return (summary.errors.empty()) ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_check_all_command(const CommandContext& context) {
    setup_signal_handlers();
    
    try {
        // Find log files
        std::vector<std::string> log_files = findLogFiles(context.extension, context.max_file_size_mb);
        
        if (log_files.empty()) {
            if (!context.quiet) {
                std::cout << "No " << context.extension << " files found in current directory." << std::endl;
            }
            return 0;
        }
        
        // Create processing context for resource management
        auto processing_context = std::make_shared<ProcessingContext>(
            298.15, // Temperature not needed for job checking
            1000,   // Concentration not needed for job checking
            context.use_input_temp,
            context.requested_threads,
            context.extension,
            context.job_resources
        );
        
        // Set memory limit if specified
        if (context.memory_limit_mb > 0) {
            processing_context->memory_monitor->set_memory_limit(context.memory_limit_mb);
        }
        
        // Create job checker
        JobChecker checker(processing_context, context.quiet, context.show_error_details);
        
        // Run all checks
        CheckSummary summary = checker.check_all_job_types(log_files);
        
        // Print any resource usage information
        if (!context.quiet) {
            printResourceUsage(*processing_context, context.quiet);
        }
        
        return (summary.errors.empty()) ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_high_level_kj_command(const CommandContext& context) {
    setup_signal_handlers();
    
    try {
        // Validate that we're in a high-level directory
        if (!HighLevelEnergyUtils::is_valid_high_level_directory()) {
            std::cerr << "Error: This command must be run from a directory containing high-level .log files" << std::endl;
            std::cerr << "       with a parent directory containing low-level thermal data." << std::endl;
            return 1;
        }
        
        // Create high-level energy calculator with specified temperature and concentration
        // Convert concentration from mol/m³ to M (mol/L)
        double concentration_m = static_cast<double>(context.concentration) / 1000.0;
        HighLevelEnergyCalculator calculator(context.temp, concentration_m);
        
        // Process all files in current directory
        auto results = calculator.process_directory(context.extension);
        
        if (results.empty()) {
            if (!context.quiet) {
                std::cout << "No " << context.extension << " files found in current directory." << std::endl;
            }
            return 0;
        }
        
        // Print results in Gibbs-focused format to console
        calculator.print_gibbs_format(results, context.quiet);
        
        // Save results to file
        std::string output_filename = HighLevelEnergyUtils::get_current_directory_name() + "-highLevel-kJ.results";
        
        std::ofstream output_file(output_filename);
        if (output_file.is_open()) {
            // Print results to file (include metadata)
            calculator.print_gibbs_format(results, false, &output_file);
            output_file.close();
            
            if (!context.quiet) {
                std::cout << "\nResults saved to: " << output_filename << std::endl;
            }
        } else {
            std::cerr << "Warning: Could not save results to " << output_filename << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_high_level_au_command(const CommandContext& context) {
    setup_signal_handlers();
    
    try {
        // Validate that we're in a high-level directory
        if (!HighLevelEnergyUtils::is_valid_high_level_directory()) {
            std::cerr << "Error: This command must be run from a directory containing high-level .log files" << std::endl;
            std::cerr << "       with a parent directory containing low-level thermal data." << std::endl;
            return 1;
        }
        
        // Create high-level energy calculator with specified temperature and concentration
        // Convert concentration from mol/m³ to M (mol/L)
        double concentration_m = static_cast<double>(context.concentration) / 1000.0;
        HighLevelEnergyCalculator calculator(context.temp, concentration_m);
        
        // Process all files in current directory
        auto results = calculator.process_directory(context.extension);
        
        if (results.empty()) {
            if (!context.quiet) {
                std::cout << "No " << context.extension << " files found in current directory." << std::endl;
            }
            return 0;
        }
        
        // Print results in component breakdown format to console
        calculator.print_components_format(results, context.quiet);
        
        // Save results to file
        std::string output_filename = HighLevelEnergyUtils::get_current_directory_name() + "-highLevel-au.results";
        
        std::ofstream output_file(output_filename);
        if (output_file.is_open()) {
            // Print results to file (include metadata)
            calculator.print_components_format(results, false, &output_file);
            output_file.close();
            
            if (!context.quiet) {
                std::cout << "\nResults saved to: " << output_filename << std::endl;
            }
        } else {
            std::cerr << "Warning: Could not save results to " << output_filename << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}