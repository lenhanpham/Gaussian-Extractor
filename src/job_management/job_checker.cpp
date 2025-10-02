#include "job_checker.h"
#include "utilities/config_manager.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <filesystem>

// JobChecker Implementation
JobChecker::JobChecker(std::shared_ptr<ProcessingContext> ctx, bool quiet, bool show_details)
    : context(ctx), quiet_mode(quiet), show_error_details(show_details) {}

CheckSummary JobChecker::check_completed_jobs(const std::vector<std::string>& log_files,
                                             const std::string& target_dir_suffix) {
    CheckSummary summary;
    summary.total_files = log_files.size();

    auto start_time = std::chrono::high_resolution_clock::now();

    // Create target directory (current_dir-suffix)
    std::string target_dir = get_current_directory_name() + "-" + target_dir_suffix;
    if (!create_target_directory(target_dir)) {
        summary.errors.push_back("Failed to create target directory: " + target_dir);
        return summary;
    }

    if (!quiet_mode) {
        std::cout << "Found " << log_files.size() << " " << context->extension << " files" << std::endl;
        std::cout << "Checking for completed jobs..." << std::endl;
    }

    // Thread-safe containers
    std::vector<JobCheckResult> completed_jobs;
    std::mutex results_mutex;
    std::atomic<size_t> file_index{0};

    // Calculate safe thread count
    unsigned int num_threads = calculateSafeThreadCount(context->requested_threads,
                                                       log_files.size(),
                                                       context->job_resources);

    if (!quiet_mode) {
        std::cout << "Using " << num_threads << " threads" << std::endl;
    }

    // Process files in parallel
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            size_t index;
            while ((index = file_index.fetch_add(1)) < log_files.size()) {
                if (g_shutdown_requested.load()) break;

                try {
                    auto file_guard = context->file_manager->acquire();
                    if (!file_guard.is_acquired()) continue;

                    JobCheckResult result = check_job_status(log_files[index]);

                    {
                        std::lock_guard<std::mutex> lock(results_mutex);
                        summary.processed_files++;

                        if (result.status == JobStatus::COMPLETED) {
                            completed_jobs.push_back(result);
                            summary.matched_files++;
                        }
                    }

                    // Report progress
                    if (!quiet_mode && summary.processed_files % 50 == 0) {
                        report_progress(summary.processed_files, summary.total_files, "checking");
                    }

                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    summary.errors.push_back("Error checking " + log_files[index] + ": " + e.what());
                }
            }
        });
    }

    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }

    if (!quiet_mode && summary.processed_files > 0) {
        report_progress(summary.processed_files, summary.total_files, "checking");
        std::cout << std::endl;
    }

    // Move completed jobs
    if (!completed_jobs.empty()) {
        if (!quiet_mode) {
            std::cout << "Found " << completed_jobs.size() << " completed jobs" << std::endl;
            std::cout << "Moving files to " << target_dir << "/" << std::endl;
        }

        for (const auto& job : completed_jobs) {
            if (move_job_files(job, target_dir)) {
                summary.moved_files++;
                if (!quiet_mode) {
                    std::cout << job.filename << " done" << std::endl;
                }
            } else {
                summary.failed_moves++;
            }
        }
    } else if (!quiet_mode) {
        std::cout << "No completed jobs found" << std::endl;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    summary.execution_time = std::chrono::duration<double>(end_time - start_time).count();

    return summary;
}

CheckSummary JobChecker::check_error_jobs(const std::vector<std::string>& log_files,
                                         const std::string& target_dir) {
    CheckSummary summary;
    summary.total_files = log_files.size();

    auto start_time = std::chrono::high_resolution_clock::now();

    // Create target directory
    if (!create_target_directory(target_dir)) {
        summary.errors.push_back("Failed to create target directory: " + target_dir);
        return summary;
    }

    if (!quiet_mode) {
        std::cout << "Found " << log_files.size() << " " << context->extension << " files" << std::endl;
        std::cout << "Checking for error jobs..." << std::endl;
    }

    // Thread-safe containers
    std::vector<JobCheckResult> error_jobs;
    std::mutex results_mutex;
    std::atomic<size_t> file_index{0};

    // Calculate safe thread count
    unsigned int num_threads = calculateSafeThreadCount(context->requested_threads,
                                                       log_files.size(),
                                                       context->job_resources);

    if (!quiet_mode) {
        std::cout << "Using " << num_threads << " threads" << std::endl;
    }

    // Process files in parallel
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            size_t index;
            while ((index = file_index.fetch_add(1)) < log_files.size()) {
                if (g_shutdown_requested.load()) break;

                try {
                    auto file_guard = context->file_manager->acquire();
                    if (!file_guard.is_acquired()) continue;

                    // Use direct error checking (independent of job status)
                    JobCheckResult result = check_error_directly(log_files[index]);

                    {
                        std::lock_guard<std::mutex> lock(results_mutex);
                        summary.processed_files++;

                        if (result.status == JobStatus::ERROR) {
                            error_jobs.push_back(result);
                            summary.matched_files++;
                        }
                    }

                    // Report progress
                    if (!quiet_mode && summary.processed_files % 50 == 0) {
                        report_progress(summary.processed_files, summary.total_files, "checking");
                    }

                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    summary.errors.push_back("Error checking " + log_files[index] + ": " + e.what());
                }
            }
        });
    }

    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }

    if (!quiet_mode && summary.processed_files > 0) {
        report_progress(summary.processed_files, summary.total_files, "checking");
        std::cout << std::endl;
    }

    // Move error jobs
    if (!error_jobs.empty()) {
        if (!quiet_mode) {
            std::cout << "Found " << error_jobs.size() << " error jobs" << std::endl;
            std::cout << "Moving files to " << target_dir << "/" << std::endl;
        }

        for (const auto& job : error_jobs) {
            if (move_job_files(job, target_dir)) {
                summary.moved_files++;
                if (!quiet_mode || show_error_details) {
                    std::cout << job.filename << ": " << job.error_message << std::endl;
                }
            } else {
                summary.failed_moves++;
            }
        }
    } else if (!quiet_mode) {
        std::cout << "No error jobs found" << std::endl;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    summary.execution_time = std::chrono::duration<double>(end_time - start_time).count();

    return summary;
}

CheckSummary JobChecker::check_pcm_failures(const std::vector<std::string>& log_files,
                                           const std::string& target_dir) {
    CheckSummary summary;
    summary.total_files = log_files.size();

    auto start_time = std::chrono::high_resolution_clock::now();

    // Create target directory
    if (!create_target_directory(target_dir)) {
        summary.errors.push_back("Failed to create target directory: " + target_dir);
        return summary;
    }

    if (!quiet_mode) {
        std::cout << "Found " << log_files.size() << " " << context->extension << " files" << std::endl;
        std::cout << "Checking for PCM convergence failures..." << std::endl;
    }

    // Thread-safe containers
    std::vector<JobCheckResult> pcm_failed_jobs;
    std::mutex results_mutex;
    std::atomic<size_t> file_index{0};

    // Calculate safe thread count
    unsigned int num_threads = calculateSafeThreadCount(context->requested_threads,
                                                       log_files.size(),
                                                       context->job_resources);

    if (!quiet_mode) {
        std::cout << "Using " << num_threads << " threads" << std::endl;
    }

    // Process files in parallel
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            size_t index;
            while ((index = file_index.fetch_add(1)) < log_files.size()) {
                if (g_shutdown_requested.load()) break;

                try {
                    auto file_guard = context->file_manager->acquire();
                    if (!file_guard.is_acquired()) continue;

                    // Use direct PCM checking (independent of job status)
                    JobCheckResult result = check_pcm_directly(log_files[index]);

                    {
                        std::lock_guard<std::mutex> lock(results_mutex);
                        summary.processed_files++;

                        if (result.status == JobStatus::PCM_FAILED) {
                            pcm_failed_jobs.push_back(result);
                            summary.matched_files++;
                        }
                    }

                    // Report progress
                    if (!quiet_mode && summary.processed_files % 50 == 0) {
                        report_progress(summary.processed_files, summary.total_files, "checking");
                    }

                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    summary.errors.push_back("Error checking " + log_files[index] + ": " + e.what());
                }
            }
        });
    }

    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }

    if (!quiet_mode && summary.processed_files > 0) {
        report_progress(summary.processed_files, summary.total_files, "checking");
        std::cout << std::endl;
    }

    // Move PCM failed jobs
    if (!pcm_failed_jobs.empty()) {
        if (!quiet_mode) {
            std::cout << "Found " << pcm_failed_jobs.size() << " PCM failed jobs" << std::endl;
            std::cout << "Moving files to " << target_dir << "/" << std::endl;
        }

        for (const auto& job : pcm_failed_jobs) {
            if (move_job_files(job, target_dir)) {
                summary.moved_files++;
                if (!quiet_mode) {
                    std::cout << job.filename << " " << job.error_message << std::endl;
                    std::cout << job.filename << " moved to " << target_dir << std::endl;
                }
            } else {
                summary.failed_moves++;
            }
        }
    } else if (!quiet_mode) {
        std::cout << "No PCM failed jobs found" << std::endl;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    summary.execution_time = std::chrono::duration<double>(end_time - start_time).count();

    return summary;
}

CheckSummary JobChecker::check_all_job_types(const std::vector<std::string>& log_files) {
    // Use optimized version for better performance
    return check_all_job_types_optimized(log_files);
}

CheckSummary JobChecker::check_all_job_types_optimized(const std::vector<std::string>& log_files) {
    CheckSummary total_summary;
    total_summary.total_files = log_files.size();

    auto start_time = std::chrono::high_resolution_clock::now();

    if (!quiet_mode) {
        std::cout << "Running optimized all job checks..." << std::endl;
        std::cout << "Found " << log_files.size() << " " << context->extension << " files" << std::endl;
    }

    // Create target directories
    std::string done_dir = get_current_directory_name() + "-done";
    std::string error_dir = "errorJobs";
    std::string pcm_dir = "PCMMkU";

    bool dirs_ok = create_target_directory(done_dir) &&
                   create_target_directory(error_dir) &&
                   create_target_directory(pcm_dir);

    if (!dirs_ok) {
        total_summary.errors.push_back("Failed to create one or more target directories");
        return total_summary;
    }

    // Thread-safe containers for classified jobs
    std::vector<JobCheckResult> completed_jobs;
    std::vector<JobCheckResult> error_jobs;
    std::vector<JobCheckResult> pcm_failed_jobs;
    std::mutex results_mutex;
    std::atomic<size_t> file_index{0};
    std::atomic<size_t> processed_count{0};

    // Calculate safe thread count
    unsigned int num_threads = calculateSafeThreadCount(context->requested_threads,
                                                       log_files.size(),
                                                       context->job_resources);

    if (!quiet_mode) {
        std::cout << "Using " << num_threads << " threads for single-pass classification" << std::endl;
    }

    // Process files in parallel with single-pass classification
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            size_t index;
            while ((index = file_index.fetch_add(1)) < log_files.size()) {
                if (g_shutdown_requested.load()) break;

                try {
                    auto file_guard = context->file_manager->acquire();
                    if (!file_guard.is_acquired()) continue;

                    // Single comprehensive status check with priority-based classification
                    JobCheckResult result = check_job_status(log_files[index]);

                    {
                        std::lock_guard<std::mutex> lock(results_mutex);

                        // Classify based on priority: completed > error > PCM
                        if (result.status == JobStatus::COMPLETED) {
                            completed_jobs.push_back(result);
                        } else if (result.status == JobStatus::ERROR) {
                            error_jobs.push_back(result);
                        } else if (result.status == JobStatus::PCM_FAILED) {
                            pcm_failed_jobs.push_back(result);
                        }
                        // RUNNING and UNKNOWN jobs are not moved
                    }

                    size_t current = processed_count.fetch_add(1) + 1;

                    // Report progress
                    if (!quiet_mode && current % 50 == 0) {
                        report_progress(current, total_summary.total_files, "classifying");
                    }

                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    total_summary.errors.push_back("Error checking " + log_files[index] + ": " + e.what());
                }
            }
        });
    }

    // Wait for classification to complete
    for (auto& thread : threads) {
        thread.join();
    }

    total_summary.processed_files = processed_count.load();
    total_summary.matched_files = completed_jobs.size() + error_jobs.size() + pcm_failed_jobs.size();

    if (!quiet_mode && total_summary.processed_files > 0) {
        report_progress(total_summary.processed_files, total_summary.total_files, "classifying");
        std::cout << std::endl;
    }

    // Move files in batches
    if (!quiet_mode) {
        std::cout << "\n=== Classification Results ===" << std::endl;
        std::cout << "Completed jobs found: " << completed_jobs.size() << std::endl;
        std::cout << "Error jobs found: " << error_jobs.size() << std::endl;
        std::cout << "PCM failed jobs found: " << pcm_failed_jobs.size() << std::endl;
    }

    // Move completed jobs
    if (!completed_jobs.empty()) {
        if (!quiet_mode) {
            std::cout << "\nMoving completed jobs to " << done_dir << "/" << std::endl;
        }
        for (const auto& job : completed_jobs) {
            if (move_job_files(job, done_dir)) {
                total_summary.moved_files++;
                if (!quiet_mode) {
                    std::cout << job.filename << " done" << std::endl;
                }
            } else {
                total_summary.failed_moves++;
            }
        }
    }

    // Move error jobs
    if (!error_jobs.empty()) {
        if (!quiet_mode) {
            std::cout << "\nMoving error jobs to " << error_dir << "/" << std::endl;
        }
        for (const auto& job : error_jobs) {
            if (move_job_files(job, error_dir)) {
                total_summary.moved_files++;
                if (!quiet_mode || show_error_details) {
                    std::cout << job.filename << ": " << job.error_message << std::endl;
                }
            } else {
                total_summary.failed_moves++;
            }
        }
    }

    // Move PCM failed jobs
    if (!pcm_failed_jobs.empty()) {
        if (!quiet_mode) {
            std::cout << "\nMoving PCM failed jobs to " << pcm_dir << "/" << std::endl;
        }
        for (const auto& job : pcm_failed_jobs) {
            if (move_job_files(job, pcm_dir)) {
                total_summary.moved_files++;
                if (!quiet_mode) {
                    std::cout << job.filename << " " << job.error_message << std::endl;
                }
            } else {
                total_summary.failed_moves++;
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    total_summary.execution_time = std::chrono::duration<double>(end_time - start_time).count();

    if (!quiet_mode) {
        std::cout << "\n=== Overall Summary ===" << std::endl;
        std::cout << "Completed jobs moved: " << completed_jobs.size() << std::endl;
        std::cout << "Error jobs moved: " << error_jobs.size() << std::endl;
        std::cout << "PCM failed jobs moved: " << pcm_failed_jobs.size() << std::endl;
        std::cout << "Total files processed: " << total_summary.processed_files << std::endl;
        std::cout << "Total files moved: " << total_summary.moved_files << std::endl;
        if (total_summary.failed_moves > 0) {
            std::cout << "Failed moves: " << total_summary.failed_moves << std::endl;
        }
        std::cout << "Total execution time: " << std::fixed << std::setprecision(3)
                  << total_summary.execution_time << " seconds" << std::endl;
    }

    return total_summary;
}

CheckSummary JobChecker::check_imaginary_frequencies(const std::vector<std::string>& log_files,
                                                     const std::string& target_dir_suffix) {
    CheckSummary summary;
    summary.total_files = log_files.size();
    auto start_time = std::chrono::high_resolution_clock::now();


    std::string target_dir = get_current_directory_name() + "-" + target_dir_suffix;
    if (!create_target_directory(target_dir)) {
        summary.errors.push_back("Failed to create target directory: " + target_dir);
        return summary;
    }

    if (!quiet_mode) {
        std::cout << "Found " << log_files.size() << " " << context->extension << " files" << std::endl;
        std::cout << "Checking for imaginary frequencies..." << std::endl;
    }

    std::vector<JobCheckResult> imag_freq_jobs;
    std::mutex results_mutex;
    std::atomic<size_t> file_index{0};

    unsigned int num_threads = calculateSafeThreadCount(context->requested_threads,
                                                       log_files.size(),
                                                       context->job_resources);
    if (!quiet_mode) {
        std::cout << "Using " << num_threads << " threads" << std::endl;
    }

    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            size_t index;
            while ((index = file_index.fetch_add(1)) < log_files.size()) {
                if (g_shutdown_requested.load()) break;

                try {
                    auto file_guard = context->file_manager->acquire();
                    if (!file_guard.is_acquired()) continue;

                    std::string content = read_file_unified(log_files[index], FileReadMode::FULL);
                    std::istringstream stream(content);
                    std::string line;
                    bool has_imag_freq = false;

                    while (std::getline(stream, line)) {
                        if (line.find("Frequencies --") != std::string::npos) {
                            std::string freqs_line = line.substr(line.find("--") + 2);
                            std::istringstream freq_stream(freqs_line);
                            double freq;
                            while (freq_stream >> freq) {
                                if (freq < 0) {
                                    has_imag_freq = true;
                                    break;
                                }
                            }
                        }
                        if (has_imag_freq) break;
                    }

                    std::lock_guard<std::mutex> lock(results_mutex);
                    summary.processed_files++;

                    if (has_imag_freq) {
                        JobCheckResult result(log_files[index], JobStatus::UNKNOWN);
                        result.related_files = find_related_files(log_files[index]);
                        imag_freq_jobs.push_back(result);
                        summary.matched_files++;
                    }

                    if (!quiet_mode && summary.processed_files % 50 == 0) {
                        report_progress(summary.processed_files, summary.total_files, "checking");
                    }

                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    summary.errors.push_back("Error checking " + log_files[index] + ": " + e.what());
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    if (!quiet_mode && summary.processed_files > 0) {
        report_progress(summary.processed_files, summary.total_files, "checking");
        std::cout << std::endl;
    }

    if (!imag_freq_jobs.empty()) {
        if (!quiet_mode) {
            std::cout << "Found " << imag_freq_jobs.size() << " jobs with imaginary frequencies" << std::endl;
            std::cout << "Moving files to " << target_dir << "/" << std::endl;
        }

        for (const auto& job : imag_freq_jobs) {
            if (move_job_files(job, target_dir)) {
                summary.moved_files++;
                if (!quiet_mode) {
                    std::cout << job.filename << " moved" << std::endl;
                }
            } else {
                summary.failed_moves++;
            }
        }
    } else if (!quiet_mode) {
        std::cout << "No jobs with imaginary frequencies found" << std::endl;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    summary.execution_time = std::chrono::duration<double>(end_time - start_time).count();

    return summary;
}


JobCheckResult JobChecker::check_job_status(const std::string& log_file) {
    JobCheckResult result(log_file, JobStatus::UNKNOWN);

    try {
        // Use unified reading with TAIL mode for efficiency
        std::string tail_content = read_file_unified(log_file, FileReadMode::TAIL, 10);

        // Check for normal termination first
        if (check_normal_termination(tail_content)) {
            result.status = JobStatus::COMPLETED;
            result.related_files = find_related_files(log_file);
            return result;
        }

        // Check for error termination first (matches bash script priority)
        std::string error_msg;
        if (check_error_termination(tail_content, error_msg)) {
            result.status = JobStatus::ERROR;
            result.error_message = error_msg;
            result.related_files = find_related_files(log_file);
            return result;
        }

        // Check for PCM failure - use FULL mode since pattern can be anywhere
        std::string full_content = read_file_unified(log_file, FileReadMode::FULL);
        if (check_pcm_failure(full_content)) {
            result.status = JobStatus::PCM_FAILED;
            result.error_message = "failed in PCMMkU";
            result.related_files = find_related_files(log_file);
            return result;
        }

        // If none of the above, assume still running
        result.status = JobStatus::RUNNING;

    } catch (const std::exception& e) {
        result.status = JobStatus::UNKNOWN;
        result.error_message = "Failed to read file: " + std::string(e.what());
    }

    return result;
}

bool JobChecker::check_normal_termination(const std::string& content) {
    // Look for "Normal" - matches bash: grep Normal
    return content.find("Normal") != std::string::npos;
}

bool JobChecker::check_error_termination(const std::string& content, std::string& error_msg) {
    // Replicate bash script logic exactly:
    // n=$(tail -10 $file_name|grep "Error" | tail -1)    # Last Error line
    // en=$(tail -10 $file_name|grep "Error on")          # Any "Error on" line
    // if [[ ! -z "$n" && -z "$en" ]]; then              # Has Error AND no "Error on"

    std::istringstream iss(content);
    std::string line;
    std::vector<std::string> error_lines;  // Store all lines containing "Error"

    // First pass: collect all lines containing "Error"
    while (std::getline(iss, line)) {
        if (line.find("Error") != std::string::npos) {
            error_lines.push_back(line);
            if (show_error_details && !quiet_mode) {
                std::cerr << "DEBUG: Found Error line: " << line << std::endl;
            }
        }
    }

    // If no error lines found, return false
    if (error_lines.empty()) {
        if (show_error_details && !quiet_mode) {
            std::cerr << "DEBUG: No error lines found" << std::endl;
        }
        return false;
    }

    // Check specifically within the error lines for "Error on"
    bool has_error_on = false;
    std::string last_error_line;

    if (!error_lines.empty()) {
        // Get the last error line (tail -1 from grep "Error")
        last_error_line = error_lines.back();

        // Check if any of the error lines contain "Error on"
        for (const auto& err_line : error_lines) {
            if (err_line.find("Error on") != std::string::npos) {
                has_error_on = true;
                if (show_error_details && !quiet_mode) {
                    std::cerr << "DEBUG: Found 'Error on' line: " << err_line << std::endl;
                }
                break;  // Found at least one, that's enough
            }
        }
    }

    // Apply bash logic: has Error AND no "Error on"
    if (!last_error_line.empty() && !has_error_on) {
        error_msg = last_error_line;
        if (show_error_details && !quiet_mode) {
            std::cerr << "DEBUG: Error detected - Last error: " << last_error_line << std::endl;
        }
        return true;
    }

    if (show_error_details && !quiet_mode) {
        std::cerr << "DEBUG: No error detected - Last error: '" << last_error_line
                  << "', Has 'Error on': " << (has_error_on ? "true" : "false") << std::endl;
    }

    return false;
}

bool JobChecker::check_pcm_failure(const std::string& content) {
    // Look for "failed in PCMMkU" - matches bash: grep "failed in PCMMkU"
    return content.find("failed in PCMMkU") != std::string::npos;
}

// Independent error checking - matches bash script exactly
JobCheckResult JobChecker::check_error_directly(const std::string& log_file) {
    JobCheckResult result(log_file, JobStatus::UNKNOWN);

    try {
        // Use unified reading with TAIL mode for efficiency
        std::string tail_content = read_file_unified(log_file, FileReadMode::TAIL, 10);

        // Check for normal termination first - if found, skip file
        if (check_normal_termination(tail_content)) {
            result.status = JobStatus::COMPLETED;
            return result;
        }

        // Check for error termination using bash script logic
        std::string error_msg;
        if (check_error_termination(tail_content, error_msg)) {
            result.status = JobStatus::ERROR;
            result.error_message = error_msg;
            result.related_files = find_related_files(log_file);

            if (show_error_details && !quiet_mode) {
                std::cerr << "DEBUG ERROR: " << log_file << " -> " << error_msg << std::endl;
            }
            return result;
        }

        // If no error found, assume still running
        result.status = JobStatus::RUNNING;

    } catch (const std::exception& e) {
        result.status = JobStatus::UNKNOWN;
        result.error_message = "Failed to read file: " + std::string(e.what());
    }

    return result;
}

// Independent PCM checking - looks only for PCM failures
JobCheckResult JobChecker::check_pcm_directly(const std::string& log_file) {
    JobCheckResult result(log_file, JobStatus::UNKNOWN);

    try {
        // PCM failures often appear near the end, try tail first with SMART mode
        // This will read tail first, and only read full if pattern might be elsewhere
        std::string content = read_file_unified(log_file, FileReadMode::TAIL, 100);

        //// If not found in tail, check full file
        //if (!check_pcm_failure(content)) {
        //    content = read_file_unified(log_file, FileReadMode::FULL);
        //}

        if (check_pcm_failure(content)) {
            result.status = JobStatus::PCM_FAILED;
            result.error_message = "failed in PCMMkU";
            result.related_files = find_related_files(log_file);

            if (show_error_details && !quiet_mode) {
                std::cerr << "DEBUG PCM: " << log_file << " -> PCM failure detected" << std::endl;
            }
            return result;
        }

        // If no PCM failure found, status remains unknown for this check
        result.status = JobStatus::UNKNOWN;

    } catch (const std::exception& e) {
        result.status = JobStatus::UNKNOWN;
        result.error_message = "Failed to read file: " + std::string(e.what());
    }

    return result;
}

std::string JobChecker::read_file_tail(const std::string& filename, size_t lines) {
    // Delegate to unified function with TAIL mode for backward compatibility
    return read_file_unified(filename, FileReadMode::TAIL, lines);
}

std::string JobChecker::read_file_content(const std::string& filename) {
    // Delegate to unified function with FULL mode for backward compatibility
    return read_file_unified(filename, FileReadMode::FULL);
}

std::string JobChecker::read_file_unified(const std::string& filename,
                                          FileReadMode mode,
                                          size_t tail_lines) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::streamsize file_size = file.tellg();

    // For FULL mode or empty files, read entire content
    if (mode == FileReadMode::FULL || file_size == 0) {
        file.seekg(0, std::ios::beg);
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // For TAIL or SMART mode
    if (mode == FileReadMode::TAIL || mode == FileReadMode::SMART) {
        if (tail_lines == 0) {
            return "";
        }

        const size_t CHUNK_SIZE = 4096;
        std::vector<char> buffer(CHUNK_SIZE);
        std::string accumulated;
        size_t lines_found = 0;
        std::streampos pos = file_size;

        // Read backwards in chunks until we have enough lines (tail_lines + 1 to account for potential trailing newline)
        while (pos > 0 && lines_found < tail_lines + 1) {
            // Determine the position and size of the chunk to read
            std::streampos read_pos = static_cast<std::streampos>(std::max(static_cast<std::streamoff>(0), static_cast<std::streamoff>(pos) - static_cast<std::streamoff>(CHUNK_SIZE)));
            size_t chunk_to_read = pos - read_pos;
            pos = read_pos;

            // Seek to the position and read the chunk
            file.seekg(pos);
            file.read(buffer.data(), chunk_to_read);

            // Prepend the chunk to the accumulated string
            accumulated.insert(0, buffer.data(), chunk_to_read);

            // Count newlines in the accumulated string
            lines_found = std::count(accumulated.begin(), accumulated.end(), '\n');
        }

        // Find the starting position of the desired lines
        size_t start_pos = accumulated.length();
        size_t newlines_to_find = tail_lines;

        // If the accumulated string ends with a newline, we might need to adjust start_pos to not include it if it's the very last character
        // However, the logic below correctly handles finding the nth newline from the end.

        // Iterate backwards to find the (tail_lines)-th newline character from the end of the accumulated string
        while (start_pos > 0 && newlines_to_find > 0) {
            start_pos--;
            if (accumulated[start_pos] == '\n') {
                newlines_to_find--;
            }
        }

        // If we found the start position (meaning we found enough newlines)
        if (start_pos > 0) {
            // Return the substring starting after the found newline
            return accumulated.substr(start_pos + 1);
        } else {
            // If start_pos is 0, it means the file has fewer than tail_lines lines, so return the whole accumulated string
            return accumulated;
        }
    }

    // Fallback for any other case (e.g., mode is not FULL, TAIL, or SMART)
    file.seekg(0, std::ios::beg);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}


std::vector<std::string> JobChecker::find_related_files(const std::string& log_file) {
    std::vector<std::string> related_files;
    std::string base_name_with_path = extract_base_name(log_file); // This returns path/filename_without_ext
    std::filesystem::path log_path(log_file); // For getting the log file's extension

    // Get input extensions from the global config manager
    std::vector<std::string> input_extensions = g_config_manager.get_input_extensions();

    // Add .chk explicitly as it's a common related file and might not be an "input" extension
    input_extensions.push_back(".chk");

    for (const auto& ext : input_extensions) {
        std::string related_file = base_name_with_path + ext;

        // Avoid adding the log file itself if its extension matches an input extension
        if (log_path.extension().string() == ext) {
            continue;
        }

        if (JobCheckerUtils::file_exists(related_file)) {
            related_files.push_back(related_file);
        }
    }

    return related_files;
}

std::string JobChecker::extract_base_name(const std::string& log_file) {
    std::filesystem::path path(log_file);
    std::string stem = path.stem().string();
    std::string parent = path.parent_path().string();

    if (parent.empty()) {
        return stem;
    } else {
        return parent + "/" + stem;
    }
}

bool JobChecker::move_job_files(const JobCheckResult& result, const std::string& target_dir) {
    try {
        std::filesystem::path target_path = target_dir;

        // Move the log file
        std::filesystem::path log_src = result.filename;
        std::filesystem::path log_dest = target_path / log_src.filename();
        std::filesystem::rename(log_src, log_dest);

        // Move related files
        for (const auto& related_file : result.related_files) {
            std::filesystem::path src = related_file;
            std::filesystem::path dest = target_path / src.filename();
            if (std::filesystem::exists(src)) {
                std::filesystem::rename(src, dest);
            }
        }

        return true;

    } catch (const std::filesystem::filesystem_error& e) {
        log_error("Failed to move files for " + result.filename + ": " + e.what());
        return false;
    }
}

bool JobChecker::create_target_directory(const std::string& target_dir) {
    try {
        std::filesystem::path dir_path = target_dir;
        if (!std::filesystem::exists(dir_path)) {
            std::filesystem::create_directories(dir_path);
        }
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        log_error("Failed to create directory " + target_dir + ": " + e.what());
        return false;
    }
}

std::string JobChecker::get_current_directory_name() {
    try {
        std::filesystem::path current = std::filesystem::current_path();
        return current.filename().string();
    } catch (const std::filesystem::filesystem_error& e) {
        return "unknown";
    }
}

void JobChecker::report_progress(size_t current, size_t total, const std::string& operation) {
    if (quiet_mode) return;

    double percentage = (static_cast<double>(current) / total) * 100.0;
    std::cout << "\r" << operation << ": " << current << "/" << total << " files ("
              << std::fixed << std::setprecision(0) << percentage << "%)" << std::flush;
}

void JobChecker::print_summary(const CheckSummary& summary, const std::string& operation) {
    if (quiet_mode) return;

    std::cout << "\n" << operation << " completed:" << std::endl;
    std::cout << "Files processed: " << summary.processed_files << "/" << summary.total_files << std::endl;
    std::cout << "Files matched: " << summary.matched_files << std::endl;
    std::cout << "Files moved: " << summary.moved_files << std::endl;

    if (summary.failed_moves > 0) {
        std::cout << "Failed moves: " << summary.failed_moves << std::endl;
    }

    std::cout << "Execution time: " << std::fixed << std::setprecision(3)
              << summary.execution_time << " seconds" << std::endl;

    if (!summary.errors.empty()) {
        std::cout << "\nErrors encountered:" << std::endl;
        for (const auto& error : summary.errors) {
            std::cout << "  " << error << std::endl;
        }
    }
}

void JobChecker::log_message(const std::string& message) {
    if (!quiet_mode) {
        std::cout << message << std::endl;
    }
}

void JobChecker::log_error(const std::string& error) {
    context->error_collector->add_error(error);
}

// JobCheckerUtils Implementation
namespace JobCheckerUtils {

bool file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::string get_file_extension(const std::string& filename) {
    std::filesystem::path path(filename);
    return path.extension().string();
}

bool is_valid_log_file(const std::string& filename, size_t max_size_mb) {
    if (!file_exists(filename)) return false;

    std::string extension = get_file_extension(filename);
    if (extension != ".log" && extension != ".out") return false;

    try {
        std::uintmax_t size = std::filesystem::file_size(filename) / (1024 * 1024);  // Convert to MB
        if (size > max_size_mb) return false;
    } catch (...) {
        return false;
    }
    // Check if readable
    std::ifstream file(filename);
    return file.good();
}
}
