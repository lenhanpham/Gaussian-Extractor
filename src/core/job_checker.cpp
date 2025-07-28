#include "job_checker.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
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
    CheckSummary total_summary;

    if (!quiet_mode) {
        std::cout << "Running all job checks..." << std::endl << std::endl;
    }

    // Check completed jobs
    if (!quiet_mode) std::cout << "=== Checking Completed Jobs ===" << std::endl;
    CheckSummary done_summary = check_completed_jobs(log_files);

    // Check error jobs
    if (!quiet_mode) std::cout << "\n=== Checking Error Jobs ===" << std::endl;
    CheckSummary error_summary = check_error_jobs(log_files);

    // Check PCM failures
    if (!quiet_mode) std::cout << "\n=== Checking PCM Failures ===" << std::endl;
    CheckSummary pcm_summary = check_pcm_failures(log_files);

    // Combine summaries
    total_summary.total_files = log_files.size();
    total_summary.processed_files = log_files.size();
    total_summary.matched_files = done_summary.matched_files + error_summary.matched_files + pcm_summary.matched_files;
    total_summary.moved_files = done_summary.moved_files + error_summary.moved_files + pcm_summary.moved_files;
    total_summary.failed_moves = done_summary.failed_moves + error_summary.failed_moves + pcm_summary.failed_moves;
    total_summary.execution_time = done_summary.execution_time + error_summary.execution_time + pcm_summary.execution_time;

    // Combine errors
    total_summary.errors.insert(total_summary.errors.end(), done_summary.errors.begin(), done_summary.errors.end());
    total_summary.errors.insert(total_summary.errors.end(), error_summary.errors.begin(), error_summary.errors.end());
    total_summary.errors.insert(total_summary.errors.end(), pcm_summary.errors.begin(), pcm_summary.errors.end());

    if (!quiet_mode) {
        std::cout << "\n=== Overall Summary ===" << std::endl;
        std::cout << "Completed jobs: " << done_summary.moved_files << std::endl;
        std::cout << "Error jobs: " << error_summary.moved_files << std::endl;
        std::cout << "PCM failed jobs: " << pcm_summary.moved_files << std::endl;
        std::cout << "Total files processed: " << total_summary.total_files << std::endl;
        std::cout << "Total execution time: " << std::fixed << std::setprecision(3) << total_summary.execution_time << " seconds" << std::endl;
    }

    return total_summary;
}

JobCheckResult JobChecker::check_job_status(const std::string& log_file) {
    JobCheckResult result(log_file, JobStatus::UNKNOWN);

    try {
        // Read the last 10 lines for efficiency (matching bash script behavior)
        std::string tail_content = read_file_tail(log_file, 10);

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

        // Check for PCM failure only if no error termination found
        std::string full_content = read_file_content(log_file);
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
    std::string last_error_line;
    bool has_error_on = false;

    // First pass: find last "Error" line and check for any "Error on"
    while (std::getline(iss, line)) {
        if (line.find("Error") != std::string::npos) {
            last_error_line = line;  // Keep updating to get the last one
            if (show_error_details && !quiet_mode) {
                std::cerr << "DEBUG: Found Error line: " << line << std::endl;
            }
        }
        if (line.find("Error on") != std::string::npos) {
            has_error_on = true;
            if (show_error_details && !quiet_mode) {
                std::cerr << "DEBUG: Found 'Error on' line: " << line << std::endl;
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
        // Read the last 10 lines for efficiency (matching bash script behavior)
        std::string tail_content = read_file_tail(log_file, 10);
        
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
        // Check for PCM failure (need to check entire file)
        std::string full_content = read_file_content(log_file);
        if (check_pcm_failure(full_content)) {
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
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    // Simple approach: read entire file and get last N lines
    std::vector<std::string> all_lines;
    std::string line;

    while (std::getline(file, line)) {
        all_lines.push_back(line);
    }

    // Get last N lines
    std::ostringstream result;
    size_t start_index = (all_lines.size() > lines) ? all_lines.size() - lines : 0;

    for (size_t i = start_index; i < all_lines.size(); ++i) {
        result << all_lines[i] << "\n";
    }

    return result.str();
}

std::string JobChecker::read_file_content(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> JobChecker::find_related_files(const std::string& log_file) {
    std::vector<std::string> related_files;
    std::string base_name = extract_base_name(log_file);

    // Look for .gau and .chk files
    std::vector<std::string> extensions = {".gau", ".chk"};

    for (const auto& ext : extensions) {
        std::string related_file = base_name + ext;
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

}
