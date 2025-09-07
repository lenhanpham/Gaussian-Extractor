#include "module_executor.h"
#include "coord_extractor.h"
#include "create_input.h"
#include "gaussian_extractor.h"
#include "high_level_energy.h"
#include "job_checker.h"
#include "version.h"
#include <algorithm>
#include <atomic>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// External global variable for shutdown handling
extern std::atomic<bool> g_shutdown_requested;

// Signal handler function
void signal_handler_func(int signal)
{
    std::cerr << "\nReceived signal " << signal << ". Initiating graceful shutdown..." << std::endl;
    g_shutdown_requested.store(true);
}

// Signal handler setup
void setup_signal_handlers()
{
    std::signal(SIGINT, signal_handler_func);
    std::signal(SIGTERM, signal_handler_func);
}

int execute_extract_command(const CommandContext& context)
{
    setup_signal_handlers();

    // Show warnings if any
    if (!context.warnings.empty() && !context.quiet)
    {
        for (const auto& warning : context.warnings)
        {
            std::cerr << warning << std::endl;
        }
        std::cerr << std::endl;
    }

    // Show resource information if requested
    if (context.show_resource_info)
    {
        JobResources job_resources  = context.job_resources;
        unsigned int hardware_cores = std::thread::hardware_concurrency();

        std::cout << "\n=== System Resource Information ===\n";
        std::cout << "Hardware cores detected: " << hardware_cores << "\n";
        std::cout << "System memory: " << MemoryMonitor::get_system_memory_mb() << " MB\n";
        std::cout << "Requested threads: " << context.requested_threads << "\n";
        if (context.memory_limit_mb > 0)
        {
            std::cout << "Memory limit: " << context.memory_limit_mb << " MB (user-specified)\n";
        }
        else
        {
            std::cout << "Memory limit: Auto-calculated based on threads and system memory\n";
        }

        // Job scheduler information
        if (job_resources.scheduler_type != SchedulerType::NONE)
        {
            std::cout << "\n=== Job Scheduler Information ===\n";
            std::cout << "Scheduler: " << JobSchedulerDetector::scheduler_name(job_resources.scheduler_type) << "\n";
            std::cout << "Job ID: " << job_resources.job_id << "\n";
            if (job_resources.has_cpu_limit)
            {
                std::cout << "Job allocated CPUs: " << job_resources.allocated_cpus << "\n";
            }
            if (job_resources.has_memory_limit)
            {
                std::cout << "Job allocated memory: " << job_resources.allocated_memory_mb << " MB\n";
            }
            if (!job_resources.partition.empty())
            {
                std::cout << "Partition/Queue: " << job_resources.partition << "\n";
            }
        }
        else
        {
            std::cout << "Job scheduler: None detected\n";
        }

        std::cout << "=====================================\n\n";
    }

    try
    {
        // Call the existing processAndOutputResults function
        processAndOutputResults(context.temp,
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
                                context.job_resources,
                                context.batch_size);

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_check_done_command(const CommandContext& context)
{
    setup_signal_handlers();

    try
    {
        // Find log files using batch processing if specified
        std::vector<std::string> log_files;

        // If using default extension (.log), search for both .log and .out files (case-insensitive)
        bool is_log_ext = (context.extension.length() == 4 && std::tolower(context.extension[1]) == 'l' &&
                           std::tolower(context.extension[2]) == 'o' && std::tolower(context.extension[3]) == 'g');
        if (is_log_ext)
        {
            std::vector<std::string> extensions = {".log", ".out"};
            if (context.batch_size > 0)
            {
                log_files = findLogFiles(extensions, context.max_file_size_mb, context.batch_size);
            }
            else
            {
                log_files = findLogFiles(extensions, context.max_file_size_mb);
            }
        }
        else
        {
            if (context.batch_size > 0)
            {
                log_files = findLogFiles(context.extension, context.max_file_size_mb, context.batch_size);
            }
            else
            {
                log_files = findLogFiles(context.extension, context.max_file_size_mb);
            }
        }

        if (log_files.empty())
        {
            if (!context.quiet)
            {
                if (context.extension == ".log")
                {
                    std::cout << "No .log or .out files found in current directory." << std::endl;
                }
                else
                {
                    std::cout << "No " << context.extension << " files found in current directory." << std::endl;
                }
            }
            return 0;
        }

        // Create processing context for resource management
        auto processing_context =
            std::make_shared<ProcessingContext>(298.15,  // Temperature not needed for job checking
                                                1000,    // Concentration not needed for job checking
                                                context.use_input_temp,
                                                context.requested_threads,
                                                context.extension,
                                                DEFAULT_MAX_FILE_SIZE_MB,  // Default max file size
                                                context.job_resources);

        // Set memory limit if specified
        if (context.memory_limit_mb > 0)
        {
            processing_context->memory_monitor->set_memory_limit(context.memory_limit_mb);
        }

        // Create job checker
        JobChecker checker(processing_context, context.quiet, false);

        // Determine target directory suffix
        std::string dir_suffix = context.dir_suffix;
        if (!context.target_dir.empty())
        {
            // If custom target directory is specified, use it as full name
            dir_suffix = context.target_dir;
        }

        // Check completed jobs
        CheckSummary summary = checker.check_completed_jobs(log_files, dir_suffix);

        // Print summary if not quiet
        if (!context.quiet)
        {
            checker.print_summary(summary, "Job completion check");
        }

        // Print any resource usage information
        if (!context.quiet)
        {
            printResourceUsage(*processing_context, context.quiet);
        }

        return (summary.errors.empty()) ? 0 : 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_check_errors_command(const CommandContext& context)
{
    setup_signal_handlers();

    try
    {
        // Find log files using batch processing if specified
        std::vector<std::string> log_files;

        // If using default extension (.log), search for both .log and .out files (case-insensitive)
        bool is_log_ext = (context.extension.length() == 4 && std::tolower(context.extension[1]) == 'l' &&
                           std::tolower(context.extension[2]) == 'o' && std::tolower(context.extension[3]) == 'g');
        if (is_log_ext)
        {
            std::vector<std::string> extensions = {".log", ".out", ".LOG", ".OUT", ".Log", ".Out"};
            if (context.batch_size > 0)
            {
                log_files = findLogFiles(extensions, context.max_file_size_mb, context.batch_size);
            }
            else
            {
                log_files = findLogFiles(extensions, context.max_file_size_mb);
            }
        }
        else
        {
            if (context.batch_size > 0)
            {
                log_files = findLogFiles(context.extension, context.max_file_size_mb, context.batch_size);
            }
            else
            {
                log_files = findLogFiles(context.extension, context.max_file_size_mb);
            }
        }

        if (log_files.empty())
        {
            if (!context.quiet)
            {
                if (context.extension == ".log")
                {
                    std::cout << "No .log or .out files found in current directory." << std::endl;
                }
                else
                {
                    std::cout << "No " << context.extension << " files found in current directory." << std::endl;
                }
            }
            return 0;
        }

        // Create processing context for resource management
        auto processing_context =
            std::make_shared<ProcessingContext>(298.15,  // Temperature not needed for job checking
                                                1000,    // Concentration not needed for job checking
                                                context.use_input_temp,
                                                context.requested_threads,
                                                context.extension,
                                                DEFAULT_MAX_FILE_SIZE_MB,  // Default max file size
                                                context.job_resources);

        // Set memory limit if specified
        if (context.memory_limit_mb > 0)
        {
            processing_context->memory_monitor->set_memory_limit(context.memory_limit_mb);
        }

        // Create job checker
        JobChecker checker(processing_context, context.quiet, context.show_error_details);

        // Determine target directory
        std::string target_dir = "errorJobs";
        if (!context.target_dir.empty())
        {
            target_dir = context.target_dir;
        }

        // Check error jobs
        CheckSummary summary = checker.check_error_jobs(log_files, target_dir);

        // Print summary if not quiet
        if (!context.quiet)
        {
            checker.print_summary(summary, "Error job check");
        }

        // Print any resource usage information
        if (!context.quiet)
        {
            printResourceUsage(*processing_context, context.quiet);
        }

        return (summary.errors.empty()) ? 0 : 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_check_pcm_command(const CommandContext& context)
{
    setup_signal_handlers();

    try
    {
        // Find log files using batch processing if specified
        std::vector<std::string> log_files;

        // If using default extension (.log), search for both .log and .out files (case-insensitive)
        bool is_log_ext = (context.extension.length() == 4 && std::tolower(context.extension[1]) == 'l' &&
                           std::tolower(context.extension[2]) == 'o' && std::tolower(context.extension[3]) == 'g');
        if (is_log_ext)
        {
            std::vector<std::string> extensions = {".log", ".out", ".LOG", ".OUT", ".Log", ".Out"};
            if (context.batch_size > 0)
            {
                log_files = findLogFiles(extensions, context.max_file_size_mb, context.batch_size);
            }
            else
            {
                log_files = findLogFiles(extensions, context.max_file_size_mb);
            }
        }
        else
        {
            if (context.batch_size > 0)
            {
                log_files = findLogFiles(context.extension, context.max_file_size_mb, context.batch_size);
            }
            else
            {
                log_files = findLogFiles(context.extension, context.max_file_size_mb);
            }
        }

        if (log_files.empty())
        {
            if (!context.quiet)
            {
                if (context.extension == ".log")
                {
                    std::cout << "No .log or .out files found in current directory." << std::endl;
                }
                else
                {
                    std::cout << "No " << context.extension << " files found in current directory." << std::endl;
                }
            }
            return 0;
        }

        // Create processing context for resource management
        auto processing_context =
            std::make_shared<ProcessingContext>(298.15,  // Temperature not needed for job checking
                                                1000,    // Concentration not needed for job checking
                                                context.use_input_temp,
                                                context.requested_threads,
                                                context.extension,
                                                DEFAULT_MAX_FILE_SIZE_MB,  // Default max file size
                                                context.job_resources);

        // Set memory limit if specified
        if (context.memory_limit_mb > 0)
        {
            processing_context->memory_monitor->set_memory_limit(context.memory_limit_mb);
        }

        // Create job checker
        JobChecker checker(processing_context, context.quiet, false);

        // Determine target directory
        std::string target_dir = "PCMMkU";
        if (!context.target_dir.empty())
        {
            target_dir = context.target_dir;
        }

        // Check PCM failed jobs
        CheckSummary summary = checker.check_pcm_failures(log_files, target_dir);

        // Print summary if not quiet
        if (!context.quiet)
        {
            checker.print_summary(summary, "PCM failure check");
        }

        // Print any resource usage information
        if (!context.quiet)
        {
            printResourceUsage(*processing_context, context.quiet);
        }

        return (summary.errors.empty()) ? 0 : 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_check_all_command(const CommandContext& context)
{
    setup_signal_handlers();

    try
    {
        // Find log files using batch processing if specified
        std::vector<std::string> log_files;

        // If using default extension (.log), search for both .log and .out files (case-insensitive)
        bool is_log_ext = (context.extension.length() == 4 && std::tolower(context.extension[1]) == 'l' &&
                           std::tolower(context.extension[2]) == 'o' && std::tolower(context.extension[3]) == 'g');
        if (is_log_ext)
        {
            std::vector<std::string> extensions = {".log", ".out", ".LOG", ".OUT", ".Log", ".Out"};
            if (context.batch_size > 0)
            {
                log_files = findLogFiles(extensions, context.max_file_size_mb, context.batch_size);
            }
            else
            {
                log_files = findLogFiles(extensions, context.max_file_size_mb);
            }
        }
        else
        {
            if (context.batch_size > 0)
            {
                log_files = findLogFiles(context.extension, context.max_file_size_mb, context.batch_size);
            }
            else
            {
                log_files = findLogFiles(context.extension, context.max_file_size_mb);
            }
        }

        if (log_files.empty())
        {
            if (!context.quiet)
            {
                if (context.extension == ".log")
                {
                    std::cout << "No .log or .out files found in current directory." << std::endl;
                }
                else
                {
                    std::cout << "No " << context.extension << " files found in current directory." << std::endl;
                }
            }
            return 0;
        }

        // Create processing context for resource management
        auto processing_context =
            std::make_shared<ProcessingContext>(298.15,  // Temperature not needed for job checking
                                                1000,    // Concentration not needed for job checking
                                                context.use_input_temp,
                                                context.requested_threads,
                                                context.extension,
                                                DEFAULT_MAX_FILE_SIZE_MB,  // Default max file size
                                                context.job_resources);

        // Set memory limit if specified
        if (context.memory_limit_mb > 0)
        {
            processing_context->memory_monitor->set_memory_limit(context.memory_limit_mb);
        }

        // Create job checker
        JobChecker checker(processing_context, context.quiet, context.show_error_details);

        // Run all checks
        CheckSummary summary = checker.check_all_job_types(log_files);

        // Print any resource usage information
        if (!context.quiet)
        {
            printResourceUsage(*processing_context, context.quiet);
        }

        return (summary.errors.empty()) ? 0 : 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_check_imaginary_command(const CommandContext& context)
{
    setup_signal_handlers();

    try
    {
        std::vector<std::string> log_files;

        // If using default extension (.log), search for both .log and .out files (case-insensitive)
        bool is_log_ext = (context.extension.length() == 4 && std::tolower(context.extension[1]) == 'l' &&
                           std::tolower(context.extension[2]) == 'o' && std::tolower(context.extension[3]) == 'g');
        if (is_log_ext)
        {
            std::vector<std::string> extensions = {".log", ".out"};
            if (context.batch_size > 0)
            {
                log_files = findLogFiles(extensions, context.max_file_size_mb, context.batch_size);
            }
            else
            {
                log_files = findLogFiles(extensions, context.max_file_size_mb);
            }
        }
        else
        {
            if (context.batch_size > 0)
            {
                log_files = findLogFiles(context.extension, context.max_file_size_mb, context.batch_size);
            }
            else
            {
                log_files = findLogFiles(context.extension, context.max_file_size_mb);
            }
        }

        if (log_files.empty())
        {
            if (!context.quiet)
            {
                if (context.extension == ".log")
                {
                    std::cout << "No .log or .out files found in current directory." << std::endl;
                }
                else
                {
                    std::cout << "No " << context.extension << " files found in current directory." << std::endl;
                }
            }
            return 0;
        }

        auto processing_context = std::make_shared<ProcessingContext>(298.15,
                                                                      1000,
                                                                      context.use_input_temp,
                                                                      context.requested_threads,
                                                                      context.extension,
                                                                      DEFAULT_MAX_FILE_SIZE_MB,
                                                                      context.job_resources);

        if (context.memory_limit_mb > 0)
        {
            processing_context->memory_monitor->set_memory_limit(context.memory_limit_mb);
        }

        JobChecker checker(processing_context, context.quiet, false);

        std::string target_dir_suffix = "imaginary_freqs";
        if (!context.target_dir.empty())
        {
            target_dir_suffix = context.target_dir;
        }

        CheckSummary summary = checker.check_imaginary_frequencies(log_files, target_dir_suffix);

        if (!context.quiet)
        {
            checker.print_summary(summary, "Imaginary frequency check");
        }

        if (!context.quiet)
        {
            printResourceUsage(*processing_context, context.quiet);
        }

        return (summary.errors.empty()) ? 0 : 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_high_level_kj_command(const CommandContext& context)
{
    setup_signal_handlers();

    try
    {
        // Validate that we're in a high-level directory
        if (!HighLevelEnergyUtils::is_valid_high_level_directory(context.extension, context.max_file_size_mb))
        {
            std::cerr << "Error: This command must be run from a directory containing high-level .log files"
                      << std::endl;
            std::cerr << "       with a parent directory containing low-level thermal data." << std::endl;
            return 1;
        }

        // Find and count log files using batch processing if specified
        std::vector<std::string> log_files;
        if (context.batch_size > 0)
        {
            log_files = findLogFiles(context.extension, context.max_file_size_mb, context.batch_size);
        }
        else
        {
            log_files = findLogFiles(context.extension, context.max_file_size_mb);
        }
        std::vector<std::string> filtered_files;
        std::copy_if(log_files.begin(),
                     log_files.end(),
                     std::back_inserter(filtered_files),
                     [&context](const std::string& file) {
                         return file.find(context.extension) != std::string::npos;
                     });

        if (!context.quiet)
        {
            std::cout << "Found " << filtered_files.size() << " " << context.extension << " files" << std::endl;

            // Debug thread calculation with detailed reasoning
            unsigned int hardware_cores = std::thread::hardware_concurrency();
            std::cout << "System: " << hardware_cores << " cores detected" << std::endl;
            std::cout << "Requested: " << context.requested_threads << " threads";
            if (context.requested_threads == hardware_cores / 2)
            {
                std::cout << " (default: half cores)";
            }
            std::cout << std::endl;

            // Show environment
            if (context.job_resources.scheduler_type != SchedulerType::NONE)
            {
                std::cout << "Environment: "
                          << JobSchedulerDetector::scheduler_name(context.job_resources.scheduler_type)
                          << " job execution" << std::endl;
            }
            else
            {
                std::cout << "Environment: Interactive/local execution" << std::endl;
            }
        }

        // Create enhanced processing context
        double concentration_m = static_cast<double>(context.concentration) / 1000.0;

        // Determine optimal thread count for high-level processing
        unsigned int requested_threads =
            context.requested_threads > 0
                ? context.requested_threads
                : calculateSafeThreadCount(context.requested_threads, 100, context.job_resources);
        unsigned int thread_count = std::min(requested_threads, static_cast<unsigned int>(filtered_files.size()));

        if (!context.quiet)
        {
            std::cout << "Using: " << thread_count << " threads";
            if (thread_count < requested_threads)
            {
                std::cout << " (reduced for safety)";
            }
            std::cout << std::endl;
            std::cout << "Max file size limit: " << context.max_file_size_mb << " MB" << std::endl;
        }

        // Create processing context with resource management using ALL CommandContext parameters
        JobResources job_resources;
        job_resources.allocated_memory_mb = context.memory_limit_mb > 0
                                                ? context.memory_limit_mb
                                                : calculateSafeMemoryLimit(0, thread_count, context.job_resources);
        job_resources.allocated_cpus      = thread_count;

        if (!context.quiet)
        {
            std::cout << "Memory limit: " << formatMemorySize(job_resources.allocated_memory_mb * 1024 * 1024)
                      << std::endl;
        }

        auto processing_context = std::make_shared<ProcessingContext>(context.temp,
                                                                      context.concentration,
                                                                      context.use_input_temp,
                                                                      thread_count,
                                                                      context.extension,
                                                                      context.max_file_size_mb,
                                                                      job_resources);

        // Create enhanced high-level energy calculator (KJ format)
        HighLevelEnergyCalculator calculator(
            processing_context, context.temp, concentration_m, context.sort_column, false);

        // Use parallel processing for better performance
        std::vector<HighLevelEnergyData> results;
        if (thread_count > 1)
        {
            results = calculator.process_directory_parallel(context.extension, thread_count, context.quiet);
        }
        else
        {
            results = calculator.process_directory(context.extension);
        }

        // Check for errors during processing
        if (processing_context->error_collector->has_errors())
        {
            auto errors = processing_context->error_collector->get_errors();
            if (!context.quiet)
            {
                std::cerr << "Errors encountered during processing:" << std::endl;
                for (const auto& error : errors)
                {
                    std::cerr << "  " << error << std::endl;
                }
            }
        }

        // Display warnings if any
        auto warnings = processing_context->error_collector->get_warnings();
        if (!warnings.empty() && !context.quiet)
        {
            std::cout << "Warnings:" << std::endl;
            for (const auto& warning : warnings)
            {
                std::cout << "  " << warning << std::endl;
            }
        }

        if (results.empty())
        {
            if (!context.quiet)
            {
                std::cout << "No valid " << context.extension << " files processed." << std::endl;
            }
            return processing_context->error_collector->has_errors() ? 1 : 0;
        }

        if (!context.quiet)
        {
            std::cout << "Successfully processed " << results.size() << "/" << filtered_files.size() << " files."
                      << std::endl;
        }

        // Print results based on output format
        if (context.output_format == "csv")
        {
            calculator.print_gibbs_csv_format(results, context.quiet);
        }
        else
        {
            calculator.print_gibbs_format_dynamic(results, context.quiet);
        }

        // Save results to file
        std::string file_extension = (context.output_format == "csv") ? ".csv" : ".results";
        std::string output_filename =
            HighLevelEnergyUtils::get_current_directory_name() + "-highLevel-kJ" + file_extension;

        std::ofstream output_file(output_filename);
        if (output_file.is_open())
        {
            // Print results to file (include metadata)
            if (context.output_format == "csv")
            {
                calculator.print_gibbs_csv_format(results, false, &output_file);
            }
            else
            {
                calculator.print_gibbs_format_dynamic(results, false, &output_file);
            }
            output_file.close();

            if (!context.quiet)
            {
                std::cout << "\nResults saved to: " << output_filename << std::endl;

                // Print resource usage summary
                auto peak_memory = processing_context->memory_monitor->get_peak_usage();
                std::cout << "Peak memory usage: " << formatMemorySize(peak_memory) << std::endl;
            }
        }
        else
        {
            std::cerr << "Warning: Could not save results to " << output_filename << std::endl;
        }

        return processing_context->error_collector->has_errors() ? 1 : 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_high_level_au_command(const CommandContext& context)
{
    setup_signal_handlers();

    try
    {
        // Validate that we're in a high-level directory
        if (!HighLevelEnergyUtils::is_valid_high_level_directory(context.extension, context.max_file_size_mb))
        {
            std::cerr << "Error: This command must be run from a directory containing high-level .log files"
                      << std::endl;
            std::cerr << "       with a parent directory containing low-level thermal data." << std::endl;
            return 1;
        }

        // Find and count log files using batch processing if specified
        std::vector<std::string> log_files;
        if (context.batch_size > 0)
        {
            log_files = findLogFiles(context.extension, context.max_file_size_mb, context.batch_size);
        }
        else
        {
            log_files = findLogFiles(context.extension, context.max_file_size_mb);
        }
        std::vector<std::string> filtered_files;
        std::copy_if(log_files.begin(),
                     log_files.end(),
                     std::back_inserter(filtered_files),
                     [&context](const std::string& file) {
                         return file.find(context.extension) != std::string::npos;
                     });

        if (!context.quiet)
        {
            std::cout << "Found " << filtered_files.size() << " " << context.extension << " files" << std::endl;

            // Debug thread calculation with detailed reasoning
            unsigned int hardware_cores = std::thread::hardware_concurrency();
            std::cout << "System: " << hardware_cores << " cores detected" << std::endl;
            std::cout << "Requested: " << context.requested_threads << " threads";
            if (context.requested_threads == hardware_cores / 2)
            {
                std::cout << " (default: half cores)";
            }
            std::cout << std::endl;

            // Show environment
            if (context.job_resources.scheduler_type != SchedulerType::NONE)
            {
                std::cout << "Environment: "
                          << JobSchedulerDetector::scheduler_name(context.job_resources.scheduler_type)
                          << " job execution" << std::endl;
            }
            else
            {
                std::cout << "Environment: Interactive/local execution" << std::endl;
            }
        }

        // Create enhanced processing context
        double concentration_m = static_cast<double>(context.concentration) / 1000.0;

        // Determine optimal thread count for high-level processing
        unsigned int requested_threads =
            context.requested_threads > 0
                ? context.requested_threads
                : calculateSafeThreadCount(context.requested_threads, 100, context.job_resources);
        unsigned int thread_count = std::min(requested_threads, static_cast<unsigned int>(filtered_files.size()));

        if (!context.quiet)
        {
            std::cout << "Using: " << thread_count << " threads";
            if (thread_count < requested_threads)
            {
                std::cout << " (reduced for safety)";
            }
            std::cout << std::endl;
            std::cout << "Max file size limit: " << context.max_file_size_mb << " MB" << std::endl;
        }

        // Create processing context with resource management using ALL CommandContext parameters
        JobResources job_resources;
        job_resources.allocated_memory_mb = context.memory_limit_mb > 0
                                                ? context.memory_limit_mb
                                                : calculateSafeMemoryLimit(0, thread_count, context.job_resources);
        job_resources.allocated_cpus      = thread_count;

        if (!context.quiet)
        {
            std::cout << "Memory limit: " << formatMemorySize(job_resources.allocated_memory_mb * 1024 * 1024)
                      << std::endl;
        }

        auto processing_context = std::make_shared<ProcessingContext>(context.temp,
                                                                      context.concentration,
                                                                      context.use_input_temp,
                                                                      thread_count,
                                                                      context.extension,
                                                                      context.max_file_size_mb,
                                                                      job_resources);

        // Create enhanced high-level energy calculator (AU format)
        HighLevelEnergyCalculator calculator(
            processing_context, context.temp, concentration_m, context.sort_column, true);

        // Use parallel processing for better performance
        std::vector<HighLevelEnergyData> results;
        if (thread_count > 1)
        {
            results = calculator.process_directory_parallel(context.extension, thread_count, context.quiet);
        }
        else
        {
            results = calculator.process_directory(context.extension);
        }

        // Check for errors during processing
        if (processing_context->error_collector->has_errors())
        {
            auto errors = processing_context->error_collector->get_errors();
            if (!context.quiet)
            {
                std::cerr << "Errors encountered during processing:" << std::endl;
                for (const auto& error : errors)
                {
                    std::cerr << "  " << error << std::endl;
                }
            }
        }

        // Display warnings if any
        auto warnings = processing_context->error_collector->get_warnings();
        if (!warnings.empty() && !context.quiet)
        {
            std::cout << "Warnings:" << std::endl;
            for (const auto& warning : warnings)
            {
                std::cout << "  " << warning << std::endl;
            }
        }

        if (results.empty())
        {
            if (!context.quiet)
            {
                std::cout << "No valid " << context.extension << " files processed." << std::endl;
            }
            return processing_context->error_collector->has_errors() ? 1 : 0;
        }

        if (!context.quiet)
        {
            std::cout << "Successfully processed " << results.size() << "/" << filtered_files.size() << " files."
                      << std::endl;
        }

        // Print results based on output format
        if (context.output_format == "csv")
        {
            calculator.print_components_csv_format(results, context.quiet);
        }
        else
        {
            calculator.print_components_format_dynamic(results, context.quiet);
        }

        // Save results to file
        std::string file_extension = (context.output_format == "csv") ? ".csv" : ".results";
        std::string output_filename =
            HighLevelEnergyUtils::get_current_directory_name() + "-highLevel-au" + file_extension;

        std::ofstream output_file(output_filename);
        if (output_file.is_open())
        {
            // Print results to file (include metadata)
            if (context.output_format == "csv")
            {
                calculator.print_components_csv_format(results, false, &output_file);
            }
            else
            {
                calculator.print_components_format_dynamic(results, false, &output_file);
            }
            output_file.close();

            if (!context.quiet)
            {
                std::cout << "\nResults saved to: " << output_filename << std::endl;

                // Print resource usage summary
                auto peak_memory = processing_context->memory_monitor->get_peak_usage();
                std::cout << "Peak memory usage: " << formatMemorySize(peak_memory) << std::endl;
            }
        }
        else
        {
            std::cerr << "Warning: Could not save results to " << output_filename << std::endl;
        }

        return processing_context->error_collector->has_errors() ? 1 : 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_extract_coords_command(const CommandContext& context)
{
    setup_signal_handlers();

    try
    {
        std::vector<std::string> log_files;
        if (!context.specific_files.empty())
        {
            // Use specified files
            log_files = context.specific_files;
            // Filter out invalid files and ensure they exist
            log_files.erase(std::remove_if(log_files.begin(),
                                           log_files.end(),
                                           [&](const std::string& file) {
                                               bool exists = std::filesystem::exists(file);
                                               if (!exists && !context.quiet)
                                               {
                                                   std::cerr << "Warning: File not found: " << file << std::endl;
                                               }
                                               return !exists || !JobCheckerUtils::is_valid_log_file(
                                                                     file, context.max_file_size_mb);
                                           }),
                            log_files.end());
        }
        else
        {
            // Find all log files
            // If using default extension (.log), search for both .log and .out files (case-insensitive)
            bool is_log_ext = (context.extension.length() == 4 && std::tolower(context.extension[1]) == 'l' &&
                               std::tolower(context.extension[2]) == 'o' && std::tolower(context.extension[3]) == 'g');
            if (is_log_ext)
            {
                std::vector<std::string> extensions = {".log", ".out", ".LOG", ".OUT", ".Log", ".Out"};
                if (context.batch_size > 0)
                {
                    log_files = findLogFiles(extensions, context.max_file_size_mb, context.batch_size);
                }
                else
                {
                    log_files = findLogFiles(extensions, context.max_file_size_mb);
                }
            }
            else
            {
                if (context.batch_size > 0)
                {
                    log_files = findLogFiles(context.extension, context.max_file_size_mb, context.batch_size);
                }
                else
                {
                    log_files = findLogFiles(context.extension, context.max_file_size_mb);
                }
            }
        }

        if (log_files.empty())
        {
            if (!context.quiet)
            {
                std::cout << "No valid " << context.extension << " files found." << std::endl;
            }
            return 0;
        }

        auto processing_context = std::make_shared<ProcessingContext>(298.15,  // Default temp, not used
                                                                      1.0,     // Default conc, not used
                                                                      false,   // use_input_temp not relevant
                                                                      context.requested_threads,
                                                                      context.extension,
                                                                      context.max_file_size_mb,
                                                                      context.job_resources);

        if (context.memory_limit_mb > 0)
        {
            processing_context->memory_monitor->set_memory_limit(context.memory_limit_mb);
        }

        CoordExtractor extractor(processing_context, context.quiet);

        ExtractSummary summary = extractor.extract_coordinates(log_files);

        extractor.print_summary(summary, "Coordinate extraction");

        if (!context.quiet)
        {
            auto errors = processing_context->error_collector->get_errors();
            if (!errors.empty())
            {
                std::cout << "\nErrors encountered:" << std::endl;
                for (const auto& err : errors)
                {
                    std::cout << "  " << err << std::endl;
                }
            }
        }

        return summary.failed_files > 0 || !processing_context->error_collector->get_errors().empty() ? 1 : 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}

int execute_create_input_command(const CommandContext& context)
{
    setup_signal_handlers();

    try
    {
        std::vector<std::string> xyz_files;
        if (!context.specific_files.empty())
        {
            // Use specific files provided
            for (const auto& file : context.specific_files)
            {
                if (std::filesystem::exists(file) && std::filesystem::is_regular_file(file))
                {
                    xyz_files.push_back(file);
                }
                else
                {
                    std::cerr << "Warning: Specified file '" << file << "' does not exist or is not a regular file."
                              << std::endl;
                }
            }
        }
        else
        {
            // Find all XYZ files in current directory
            for (const auto& entry : std::filesystem::directory_iterator("."))
            {
                if (entry.is_regular_file())
                {
                    std::string extension = entry.path().extension().string();
                    if (extension == ".xyz")
                    {
                        xyz_files.push_back(entry.path().string());
                    }
                }
            }
        }

        if (xyz_files.empty())
        {
            if (!context.quiet)
            {
                std::cout << "No valid .xyz files found." << std::endl;
            }
            return 0;
        }

        auto processing_context = std::make_shared<ProcessingContext>(298.15,  // Default temp, not used
                                                                      1.0,     // Default conc, not used
                                                                      false,   // use_input_temp not relevant
                                                                      context.requested_threads,
                                                                      ".xyz",  // extension for XYZ files
                                                                      context.max_file_size_mb,
                                                                      context.job_resources);

        // Create CreateInput instance with default parameters
        CreateInput creator(processing_context, context.quiet);

        // Configure calculation parameters from context
        // Convert string calc_type to CalculationType enum
        CalculationType calc_type = CalculationType::SP;  // Default
        if (context.ci_calc_type == "opt_freq")
        {
            calc_type = CalculationType::OPT_FREQ;
        }
        else if (context.ci_calc_type == "ts_freq")
        {
            calc_type = CalculationType::TS_FREQ;
        }
        else if (context.ci_calc_type == "modre_opt")
        {
            calc_type = CalculationType::MODRE_OPT;
        }       
        else if (context.ci_calc_type == "oss_ts_freq")
        {
            calc_type = CalculationType::OSS_TS_FREQ;
        }
        else if (context.ci_calc_type == "modre_ts_freq")
        {
            calc_type = CalculationType::MODRE_TS_FREQ;
        }
        else if (context.ci_calc_type == "oss_check_sp")
        {
            calc_type = CalculationType::OSS_CHECK_SP;
        }
        else if (context.ci_calc_type == "high_sp")
        {
            calc_type = CalculationType::HIGH_SP;
        }
        else if (context.ci_calc_type == "irc_forward")
        {
            calc_type = CalculationType::IRC_FORWARD;
        }
        else if (context.ci_calc_type == "irc_reverse")
        {
            calc_type = CalculationType::IRC_REVERSE;
        }
        else if (context.ci_calc_type == "irc")
        {
            calc_type = CalculationType::IRC;
        }
        // Default is SP for any other value

        // Validate freeze atoms or modre for OSS_TS_FREQ and MODRE_TS_FREQ
        if (calc_type == CalculationType::OSS_TS_FREQ || calc_type == CalculationType::MODRE_TS_FREQ)
        {
            bool has_freeze_atoms = (context.ci_freeze_atom1 != 0 && context.ci_freeze_atom2 != 0);
            bool has_modre        = !context.ci_modre.empty();

            if (!has_freeze_atoms && !has_modre)
            {
                std::string calc_type_name =
                    (calc_type == CalculationType::OSS_TS_FREQ) ? "oss_ts_freq" : "modre_ts_freq";
                std::cerr << "Error: --freeze-atoms or modre parameter is required for " << calc_type_name
                          << " calculation type." << std::endl;
                std::cerr << "Please specify --freeze-atoms 1 2 or provide modre in the parameter file." << std::endl;
                return 1;
            }
        }

        creator.set_calculation_type(calc_type);
        creator.set_functional(context.ci_functional);
        creator.set_basis(context.ci_basis);
        if (!context.ci_large_basis.empty())
        {
            creator.set_large_basis(context.ci_large_basis);
        }
        if (!context.ci_solvent.empty())
        {
            creator.set_solvent(context.ci_solvent, context.ci_solvent_model);
        }
        creator.set_print_level(context.ci_print_level);
        creator.set_extra_keywords(context.ci_extra_keywords);
        creator.set_extra_keyword_section(context.ci_extra_keyword_section);
        creator.set_molecular_specs(context.ci_charge, context.ci_mult);
        creator.set_tail(context.ci_tail);
        creator.set_modre(context.ci_modre);
        creator.set_extension(context.ci_extension);
        creator.set_tschk_path(context.ci_tschk_path);
        if (context.ci_freeze_atom1 != 0 && context.ci_freeze_atom2 != 0)
        {
            creator.set_freeze_atoms(context.ci_freeze_atom1, context.ci_freeze_atom2);
        }
        creator.set_scf_maxcycle(context.ci_scf_maxcycle);
        creator.set_opt_maxcycles(context.ci_opt_maxcycles);
        creator.set_irc_maxpoints(context.ci_irc_maxpoints);
        creator.set_irc_recalc(context.ci_irc_recalc);
        creator.set_irc_maxcycle(context.ci_irc_maxcycle);
        creator.set_irc_stepsize(context.ci_irc_stepsize);

        // Execute the creation (with batch processing if enabled)
        CreateSummary total_summary;
        if (context.batch_size > 0 && xyz_files.size() > context.batch_size)
        {
            // Process files in batches
            size_t total_files       = xyz_files.size();
            size_t processed_batches = 0;

            if (!context.quiet)
            {
                std::cout << "Processing " << total_files << " files in batches of " << context.batch_size << std::endl;
            }

            for (size_t i = 0; i < total_files; i += context.batch_size)
            {
                size_t                   batch_end = std::min(i + context.batch_size, total_files);
                std::vector<std::string> batch(xyz_files.begin() + i, xyz_files.begin() + batch_end);

                if (!context.quiet)
                {
                    std::cout << "Processing batch " << (processed_batches + 1) << " (files " << (i + 1) << "-"
                              << batch_end << ")" << std::endl;
                }

                CreateSummary batch_summary = creator.create_inputs(batch);

                // Accumulate results
                total_summary.total_files += batch_summary.total_files;
                total_summary.processed_files += batch_summary.processed_files;
                total_summary.created_files += batch_summary.created_files;
                total_summary.failed_files += batch_summary.failed_files;
                total_summary.skipped_files += batch_summary.skipped_files;
                total_summary.execution_time += batch_summary.execution_time;

                processed_batches++;

                // Check for shutdown signal
                if (g_shutdown_requested.load())
                {
                    if (!context.quiet)
                    {
                        std::cout << "Shutdown requested, stopping batch processing..." << std::endl;
                    }
                    break;
                }
            }

            if (!context.quiet)
            {
                std::cout << "Completed processing " << processed_batches << " batches" << std::endl;
            }
        }
        else
        {
            // Process all files at once (original behavior)
            total_summary = creator.create_inputs(xyz_files);
        }

        // Print summary
        if (!context.quiet)
        {
            creator.print_summary(total_summary, "Input file creation");
        }

        // Check for errors
        if (!processing_context->error_collector->get_errors().empty())
        {
            if (!context.quiet)
            {
                std::cout << "\nProcessing errors:" << std::endl;
                for (const auto& error : processing_context->error_collector->get_errors())
                {
                    std::cout << "  " << error << std::endl;
                }
            }
            return 1;
        }

        return total_summary.failed_files > 0 ? 1 : 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 1;
    }
}
