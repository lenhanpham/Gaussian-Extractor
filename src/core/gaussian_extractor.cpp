/**
 * @file gaussian_extractor.cpp
 * @brief Implementation of core Gaussian log file processing and data extraction
 * @author Le Nhan Pham
 * @date 2025
 * 
 * This file implements the core functionality for extracting thermodynamic data
 * from Gaussian computational chemistry log files. It provides multi-threaded
 * processing with comprehensive resource management, thread-safe operations,
 * and integration with job scheduler systems for HPC environments.
 * 
 * @section Key Implementations
 * - MemoryMonitor: Thread-safe memory usage tracking and limiting
 * - FileHandleManager: RAII-based file handle resource management  
 * - ThreadSafeErrorCollector: Centralized error collection across threads
 * - Result extraction: Core data parsing from Gaussian log files
 * - Multi-threaded processing: Parallel file processing with coordination
 * 
 * @section Threading Architecture
 * The implementation uses a producer-consumer model:
 * - Main thread: File discovery, validation, and result coordination
 * - Worker threads: Parallel file processing with resource management
 * - Shared resources: Memory monitor, file handle manager, error collector
 * 
 * @section Resource Management
 * Comprehensive resource management prevents system overload:
 * - Memory usage monitoring with configurable limits
 * - File handle limiting to prevent system exhaustion
 * - Thread-safe error collection and reporting
 * - Integration with job scheduler resource limits
 * 
 * @section Platform Support
 * The implementation supports multiple platforms:
 * - Windows: Using Windows API for system information
 * - Linux: Using sysinfo and proc filesystem
 * - Other Unix: Using standard POSIX interfaces
 */

#include "gaussian_extractor.h"
#include "version.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <future>
#include <exception>
#include <csignal>
#include <cstdlib>
#include "job_scheduler.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif
#endif

/**
 * @defgroup PhysicalConstants Physical Constants Implementation
 * @brief Definition of fundamental physical constants used throughout calculations
 * @{
 */

/// Universal gas constant in J/(K·mol) - 2018 CODATA value
const double R = 8.314462618;

/// Standard atmospheric pressure in N/m² (exactly 1 atm)
const double Po = 101325;

/// Boltzmann constant in Hartree/K for atomic unit calculations
const double kB = 0.000003166811563;

/** @} */ // end of PhysicalConstants group

// External global variable for shutdown handling
extern std::atomic<bool> g_shutdown_requested;

/**
 * @defgroup MemoryMonitorImpl MemoryMonitor Implementation
 * @brief Implementation of thread-safe memory usage tracking and limiting
 * @{
 */

/**
 * @brief Constructor initializes memory monitor with specified limit
 * @param max_memory_mb Maximum memory usage limit in megabytes
 * 
 * Converts megabyte limit to internal byte representation for efficient
 * atomic operations. The limit should account for other system processes.
 */
MemoryMonitor::MemoryMonitor(size_t max_memory_mb)
    : max_bytes(max_memory_mb * 1024 * 1024) {}

/**
 * @brief Check if allocation would exceed memory limit
 * @param bytes Number of bytes to potentially allocate
 * @return true if allocation is within limits, false if would exceed
 * 
 * Thread-safe check using atomic load operation. Should be called before
 * large allocations to prevent memory exhaustion.
 */
bool MemoryMonitor::can_allocate(size_t bytes) {
    return (current_usage_bytes.load() + bytes) < max_bytes;
}

/**
 * @brief Record memory allocation and update peak usage tracking
 * @param bytes Number of bytes allocated
 * 
 * Atomically updates current usage and peak usage statistics. Uses
 * compare-and-swap to handle concurrent peak updates correctly.
 */
void MemoryMonitor::add_usage(size_t bytes) {
    size_t new_usage = current_usage_bytes.fetch_add(bytes) + bytes;
    size_t current_peak = peak_usage_bytes.load();
    while (new_usage > current_peak) {
        if (peak_usage_bytes.compare_exchange_weak(current_peak, new_usage)) {
            break;
        }
    }
}

/**
 * @brief Record memory deallocation
 * @param bytes Number of bytes freed
 * 
 * Atomically decreases current usage counter. Should be called when
 * memory is freed or objects are destroyed.
 */
void MemoryMonitor::remove_usage(size_t bytes) {
    current_usage_bytes.fetch_sub(bytes);
}

/**
 * @brief Get current memory usage
 * @return Current memory usage in bytes
 * 
 * Thread-safe atomic read of current memory usage.
 */
size_t MemoryMonitor::get_current_usage() const {
    return current_usage_bytes.load();
}

/**
 * @brief Get peak memory usage since monitor creation
 * @return Peak memory usage in bytes
 * 
 * Thread-safe atomic read of peak memory usage for performance analysis.
 */
size_t MemoryMonitor::get_peak_usage() const {
    return peak_usage_bytes.load();
}

/**
 * @brief Get configured memory limit
 * @return Maximum allowed memory usage in bytes
 */
size_t MemoryMonitor::get_max_usage() const {
    return max_bytes;
}

/**
 * @brief Update memory limit during runtime
 * @param max_memory_mb New memory limit in megabytes
 * 
 * Allows dynamic adjustment of memory limits based on system conditions
 * or user preferences. Not thread-safe for concurrent limit changes.
 */
void MemoryMonitor::set_memory_limit(size_t max_memory_mb) {
    max_bytes = max_memory_mb * 1024 * 1024;
}

/**
 * @brief Detect total system memory capacity
 * @return Total system memory in megabytes
 * 
 * Platform-specific implementation for detecting available system memory.
 * Uses Windows API on Windows and POSIX/Linux interfaces on Unix-like systems.
 * Provides fallback values if detection fails.
 */
size_t MemoryMonitor::get_system_memory_mb() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return static_cast<size_t>(memInfo.ullTotalPhys / (1024 * 1024));
    }
    return DEFAULT_MEMORY_MB; // Fallback
#else
    // Try multiple methods for Unix-like systems

    // Method 1: sysconf (POSIX)
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        return static_cast<size_t>((pages * page_size) / (1024 * 1024));
    }

    // Method 2: Linux-specific sysinfo (fallback)
    #if defined(__linux__) && defined(__GLIBC__)
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return static_cast<size_t>((si.totalram * si.mem_unit) / (1024 * 1024));
    }
    #endif

    return DEFAULT_MEMORY_MB; // Final fallback
#endif
}

size_t MemoryMonitor::calculate_optimal_memory_limit(unsigned int thread_count, size_t system_memory_mb) {
    if (system_memory_mb == 0) {
        system_memory_mb = get_system_memory_mb();
    }

    // Use percentage of system memory based on thread count and environment
    double memory_percentage = 0.5; // Default 50% of system memory

    // Adjust based on thread count
    if (thread_count <= 4) {
        memory_percentage = 0.3; // Conservative for few threads
    } else if (thread_count <= 8) {
        memory_percentage = 0.4; // Moderate usage
    } else if (thread_count <= 16) {
        memory_percentage = 0.5; // Standard usage
    } else {
        memory_percentage = 0.6; // Higher usage for many threads
    }

    // Be more conservative in cluster environments
    bool is_cluster = (std::getenv("SLURM_JOB_ID") != nullptr ||
                      std::getenv("PBS_JOBID") != nullptr ||
                      std::getenv("SGE_JOB_ID") != nullptr ||
                      std::getenv("LSB_JOBID") != nullptr);

    if (is_cluster) {
        memory_percentage *= 0.7; // Use 70% of calculated amount in clusters
    }

    size_t calculated_memory = static_cast<size_t>(system_memory_mb * memory_percentage);

    // Apply bounds
    calculated_memory = std::max(calculated_memory, MIN_MEMORY_MB);
    calculated_memory = std::min(calculated_memory, MAX_MEMORY_MB);

    return calculated_memory;
}

// FileHandleManager implementation
FileHandleManager::FileGuard::FileGuard(FileHandleManager* mgr)
    : manager(mgr), acquired(false) {
    if (manager) {
#if __cpp_lib_semaphore >= 201907L
        manager->semaphore.acquire();
#else
        std::unique_lock<std::mutex> lock(manager->mutex_);
        manager->cv_.wait(lock, [mgr] { return mgr->available_handles.load() > 0; });
        manager->available_handles.fetch_sub(1);
#endif
        acquired = true;
    }
}

FileHandleManager::FileGuard::~FileGuard() {
    if (acquired && manager) {
#if __cpp_lib_semaphore >= 201907L
        manager->semaphore.release();
#else
        {
            std::lock_guard<std::mutex> lock(manager->mutex_);
            manager->available_handles.fetch_add(1);
        }
        manager->cv_.notify_one();
#endif
    }
}

FileHandleManager::FileGuard::FileGuard(FileGuard&& other) noexcept
    : manager(other.manager), acquired(other.acquired) {
    other.manager = nullptr;
    other.acquired = false;
}

FileHandleManager::FileGuard& FileHandleManager::FileGuard::operator=(FileGuard&& other) noexcept {
    if (this != &other) {
        if (acquired && manager) {
#if __cpp_lib_semaphore >= 201907L
            manager->semaphore.release();
#else
            {
                std::lock_guard<std::mutex> lock(manager->mutex_);
                manager->available_handles.fetch_add(1);
            }
            manager->cv_.notify_one();
#endif
        }
        manager = other.manager;
        acquired = other.acquired;
        other.manager = nullptr;
        other.acquired = false;
    }
    return *this;
}

FileHandleManager::FileGuard FileHandleManager::acquire() {
    return FileGuard(this);
}

void FileHandleManager::release() {
#if __cpp_lib_semaphore >= 201907L
    semaphore.release();
#else
    {
        std::lock_guard<std::mutex> lock(mutex_);
        available_handles.fetch_add(1);
    }
    cv_.notify_one();
#endif
}

// ThreadSafeErrorCollector implementation
void ThreadSafeErrorCollector::add_error(const std::string& error) {
    std::lock_guard<std::mutex> lock(error_mutex);
    errors.push_back(error);
}

void ThreadSafeErrorCollector::add_warning(const std::string& warning) {
    std::lock_guard<std::mutex> lock(error_mutex);
    warnings.push_back(warning);
}

std::vector<std::string> ThreadSafeErrorCollector::get_errors() const {
    std::lock_guard<std::mutex> lock(error_mutex);
    return errors;
}

std::vector<std::string> ThreadSafeErrorCollector::get_warnings() const {
    std::lock_guard<std::mutex> lock(error_mutex);
    return warnings;
}

bool ThreadSafeErrorCollector::has_errors() const {
    std::lock_guard<std::mutex> lock(error_mutex);
    return !errors.empty();
}

void ThreadSafeErrorCollector::clear() {
    std::lock_guard<std::mutex> lock(error_mutex);
    errors.clear();
    warnings.clear();
}

// Utility functions implementation
bool safe_stod(const std::string& str, double& result) {
    try {
        size_t pos;
        result = std::stod(str, &pos);
        return pos == str.length(); // Ensure entire string was consumed
    } catch (const std::exception&) {
        return false;
    }
}

bool safe_stoi(const std::string& str, int& result) {
    try {
        size_t pos;
        result = std::stoi(str, &pos);
        return pos == str.length();
    } catch (const std::exception&) {
        return false;
    }
}

bool safe_stoul(const std::string& str, unsigned long& result) {
    try {
        size_t pos;
        result = std::stoul(str, &pos);
        return pos == str.length();
    } catch (const std::exception&) {
        return false;
    }
}

std::string formatMemorySize(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        unit++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

bool validateFileSize(const std::string& filename, size_t max_size_mb) {
    try {
        std::filesystem::path file_path(filename);
        if (!std::filesystem::exists(file_path)) {
            return false;
        }

        auto file_size = std::filesystem::file_size(file_path);
        size_t max_bytes = max_size_mb * 1024 * 1024;

        return file_size <= max_bytes;
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

std::vector<std::string> findLogFiles(const std::string& extension, size_t max_file_size_mb) {
    std::vector<std::string> log_files;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            if (g_shutdown_requested.load()) {
                break;
            }

            if (entry.is_regular_file() && entry.path().extension() == extension) {
                std::string filename = entry.path().string();

                // Validate file size before adding
                if (validateFileSize(filename, max_file_size_mb)) {
                    log_files.push_back(filename);
                } else {
                    std::cerr << "Warning: Skipping oversized file: " << filename
                              << " (>" << max_file_size_mb << "MB)" << std::endl;
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Error accessing directory: " + std::string(e.what()));
    }

    // Sort files for consistent processing order
    std::sort(log_files.begin(), log_files.end());
    return log_files;
}

void printResourceUsage(const ProcessingContext& context, bool quiet) {
    if (quiet) return;

    size_t current_mem = context.memory_monitor->get_current_usage();
    size_t peak_mem = context.memory_monitor->get_peak_usage();
    size_t max_mem = context.memory_monitor->get_max_usage();

    std::cout << "Memory usage: " << formatMemorySize(current_mem)
              << " (peak: " << formatMemorySize(peak_mem) << ")"
              << " / " << formatMemorySize(max_mem) << std::endl;
}

void printJobResourceInfo(const JobResources& job_resources, bool quiet) {
    if (quiet || job_resources.scheduler_type == SchedulerType::NONE) return;

    std::cout << "\n=== Job Scheduler Information ===" << std::endl;
    std::cout << "Scheduler: " << JobSchedulerDetector::scheduler_name(job_resources.scheduler_type) << std::endl;
    std::cout << "Job ID: " << job_resources.job_id << std::endl;

    if (job_resources.has_cpu_limit) {
        std::cout << "Allocated CPUs: " << job_resources.allocated_cpus << std::endl;
    }

    if (job_resources.has_memory_limit) {
        std::cout << "Allocated Memory: " << formatMemorySize(job_resources.allocated_memory_mb * 1024 * 1024) << std::endl;
    }

    if (!job_resources.partition.empty()) {
        std::cout << "Partition/Queue: " << job_resources.partition << std::endl;
    }

    if (!job_resources.account.empty()) {
        std::cout << "Account: " << job_resources.account << std::endl;
    }

    std::cout << "=================================\n" << std::endl;
}

unsigned int calculateSafeThreadCount(unsigned int requested_threads,
                                     unsigned int file_count,
                                     const JobResources& job_resources) {
    unsigned int hardware_cores = std::thread::hardware_concurrency();
    if (hardware_cores == 0) hardware_cores = 4;

    // Start with user's request (which defaults to half cores)
    unsigned int max_safe_threads = requested_threads;

    // Apply limits based on execution environment
    bool has_job_limits = (job_resources.scheduler_type != SchedulerType::NONE && job_resources.has_cpu_limit);

    if (has_job_limits) {
        // Running in a job with CPU limits - respect job allocation
        // Job limits are applied later in the function
    } else {
        // Interactive session or head node - allow requested amount with reasonable caps
        unsigned int reasonable_limit;

        if (hardware_cores >= 32) {
            // Large systems (32+ cores): allow up to half cores, capped at 32
            reasonable_limit = std::min(hardware_cores / 2, 32u);
        } else if (hardware_cores >= 16) {
            // Medium systems (16-31 cores): allow up to half cores, capped at 16
            reasonable_limit = std::min(hardware_cores / 2, 16u);
        } else {
            // Small systems (<16 cores): allow up to 8 threads
            reasonable_limit = std::min(hardware_cores, 8u);
        }

        // Only limit if user requests more than reasonable amount
        if (requested_threads > reasonable_limit) {
            max_safe_threads = reasonable_limit;
        }
    }

    // Apply job scheduler CPU limits if available
    if (job_resources.has_cpu_limit && job_resources.allocated_cpus > 0) {
        max_safe_threads = std::min(max_safe_threads, job_resources.allocated_cpus);
    }

    // Never exceed file count
    max_safe_threads = std::min(max_safe_threads, file_count);

    return std::max(1u, max_safe_threads);
}

size_t calculateSafeMemoryLimit(size_t requested_memory_mb,
                               unsigned int thread_count,
                               const JobResources& job_resources) {
    size_t calculated_memory = requested_memory_mb;

    // If no explicit request, calculate based on system and threads
    if (requested_memory_mb == 0) {
        calculated_memory = MemoryMonitor::calculate_optimal_memory_limit(thread_count);
    }

    // Apply job scheduler memory limits if available
    if (job_resources.has_memory_limit && job_resources.allocated_memory_mb > 0) {
        // Leave 5% overhead for system processes
        size_t job_memory_with_overhead = job_resources.allocated_memory_mb * 95 / 100;
        calculated_memory = std::min(calculated_memory, job_memory_with_overhead);
    }

    // Apply bounds
    calculated_memory = std::max(calculated_memory, MIN_MEMORY_MB);
    calculated_memory = std::min(calculated_memory, MAX_MEMORY_MB);

    return calculated_memory;
}

bool compareResults(const Result& a, const Result& b, int column) {
    switch (column) {
        case 2: return a.etgkj < b.etgkj;
        case 3: return a.lf < b.lf;
        case 4: return a.GibbsFreeHartree < b.GibbsFreeHartree;
        case 5: return a.nucleare < b.nucleare;
        case 6: return a.scf < b.scf;
        case 7: return a.zpe < b.zpe;
        case 10: return a.copyright_count < b.copyright_count;
        default: return false;
    }
}

Result extract(const std::string& file_name_param, const ProcessingContext& context) {
    // Check for shutdown signal
    if (g_shutdown_requested.load()) {
        throw std::runtime_error("Processing interrupted by shutdown signal");
    }

    // Acquire file handle
    auto file_guard = context.file_manager->acquire();
    if (!file_guard.is_acquired()) {
        throw std::runtime_error("Could not acquire file handle for: " + file_name_param);
    }

    // Estimate memory usage for this file
    size_t estimated_memory = 0;
    try {
        auto file_size = std::filesystem::file_size(file_name_param);
        estimated_memory = file_size + (CONTENT_BUFFER_SIZE * 256); // Estimate line buffer overhead
    } catch (const std::filesystem::filesystem_error&) {
        estimated_memory = 1024 * 1024; // 1MB default estimate
    }

    if (!context.memory_monitor->can_allocate(estimated_memory)) {
        throw std::runtime_error("Insufficient memory to process file: " + file_name_param);
    }

    context.memory_monitor->add_usage(estimated_memory);

    // RAII class for memory cleanup
    struct MemoryGuard {
        std::shared_ptr<MemoryMonitor> monitor;
        size_t bytes;

        MemoryGuard(std::shared_ptr<MemoryMonitor> m, size_t b) : monitor(m), bytes(b) {}
        ~MemoryGuard() { if (monitor) monitor->remove_usage(bytes); }
    };
    MemoryGuard memory_guard(context.memory_monitor, estimated_memory);

    std::ifstream file(file_name_param);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + file_name_param);
    }

    std::string file_name = file_name_param;
    if (file_name.substr(0, 2) == "./") {
        file_name = file_name.substr(2);
    }

    std::string line;
    std::vector<std::string> content;
    content.reserve(CONTENT_BUFFER_SIZE); // Pre-allocate to avoid reallocations

    int copyright_count = 0;
    double scf = 0, zpe = 0, tcg = 0, etg = 0, ezpe = 0, nucleare = 0;
    double scfEqui = 0, scftd = 0;
    double temp = context.base_temp; // Local copy for this file
    std::vector<double> negative_freqs, positive_freqs;
    std::string status = "UNDONE";
    std::string phaseCorr = "NO";

    // Pre-compile regex patterns for better performance
    static const std::regex scf_pattern(R"(SCF Done.*?=\s+(-?\d+\.\d+))");
    static const std::regex freq_pattern(R"(Frequencies\s+--\s+(.*))");

    std::vector<double> scf_values;
    size_t line_count = 0;

    try {
        while (std::getline(file, line) && !g_shutdown_requested.load()) {
            line_count++;

            // Maintain rolling buffer of recent lines
            context.memory_monitor->add_usage(line.capacity());
            content.push_back(line);
            if (content.size() > CONTENT_BUFFER_SIZE) {
                context.memory_monitor->remove_usage(content.front().capacity());
                content.erase(content.begin());
            }

            // Process line content
            if (line.find("Copyright") != std::string::npos) {
                ++copyright_count;
            }

            std::smatch match;
            try {
                if (line.find("SCF Done") != std::string::npos && std::regex_search(line, match, scf_pattern)) {
                    double value;
                    if (safe_stod(match[1], value)) {
                        scf_values.push_back(value);
                    }
                } else if (line.find("Total Energy, E(CIS") != std::string::npos) {
                    size_t eq_pos = line.find("=");
                    if (eq_pos != std::string::npos) {
                        std::string value_str = line.substr(eq_pos + 1);
                        safe_stod(value_str, scftd);
                    }
                } else if (line.find("After PCM corrections, the energy is") != std::string::npos) {
                    size_t is_pos = line.find("is");
                    if (is_pos != std::string::npos) {
                        std::string value_str = line.substr(is_pos + 2);
                        safe_stod(value_str, scfEqui);
                    }
                } else if (line.find("Zero-point correction") != std::string::npos) {
                    size_t eq_pos = line.find("=");
                    if (eq_pos != std::string::npos) {
                        std::string value_str = line.substr(eq_pos + 1);
                        safe_stod(value_str, zpe);
                    }
                } else if (line.find("Thermal correction to Gibbs Free Energy") != std::string::npos) {
                    size_t eq_pos = line.find("=");
                    if (eq_pos != std::string::npos) {
                        std::string value_str = line.substr(eq_pos + 1);
                        safe_stod(value_str, tcg);
                    }
                } else if (line.find("Sum of electronic and thermal Free Energies") != std::string::npos) {
                    size_t eq_pos = line.find("=");
                    if (eq_pos != std::string::npos) {
                        std::string value_str = line.substr(eq_pos + 1);
                        safe_stod(value_str, etg);
                    }
                } else if (line.find("Sum of electronic and zero-point Energies") != std::string::npos) {
                    size_t eq_pos = line.find("=");
                    if (eq_pos != std::string::npos) {
                        std::string value_str = line.substr(eq_pos + 1);
                        safe_stod(value_str, ezpe);
                    }
                } else if (line.find("nuclear repulsion energy") != std::string::npos) {
                    size_t pos = line.find("nuclear repulsion energy") + 25;
                    if (pos < line.length()) {
                        std::string num_str = line.substr(pos);

                        // Clean up the string
                        num_str.erase(0, num_str.find_first_not_of(" \t"));
                        size_t end_pos = num_str.find("Hartrees");
                        if (end_pos != std::string::npos) {
                            num_str = num_str.substr(0, end_pos);
                        }
                        num_str.erase(num_str.find_last_not_of(" \t") + 1);

                        if (!num_str.empty() && !safe_stod(num_str, nucleare)) {
                            context.error_collector->add_warning(
                                "Could not parse nuclear repulsion energy from '" + line + "' in file '" + file_name + "'");
                        }
                    }
                } else if (line.find("Frequencies") != std::string::npos && std::regex_search(line, match, freq_pattern)) {
                    std::istringstream iss(match[1]);
                    double freq;
                    while (iss >> freq) {
                        if (freq < 0) {
                            negative_freqs.push_back(freq);
                        } else {
                            positive_freqs.push_back(freq);
                        }
                    }
                } else if (!context.use_input_temp && line.find("Kelvin.  Pressure") != std::string::npos) {
                    size_t start_pos = line.find("Temperature");
                    size_t end_pos = line.find("Kelvin");

                    if (start_pos != std::string::npos && end_pos != std::string::npos && start_pos < end_pos) {
                        start_pos += 11; // Length of "Temperature"
                        std::string temp_str = line.substr(start_pos, end_pos - start_pos);

                        // Clean up the string
                        temp_str.erase(0, temp_str.find_first_not_of(" \t"));
                        temp_str.erase(temp_str.find_last_not_of(" \t") + 1);

                        if (!temp_str.empty()) {
                            if (!safe_stod(temp_str, temp)) {
                                context.error_collector->add_warning(
                                    "Could not parse temperature from '" + line + "' in file '" + file_name + "'. Using default 298.15 K");
                                temp = 298.15;
                            }
                        }
                    }
                } else if (line.find("scrf") != std::string::npos) {
                    phaseCorr = "YES";
                }
            } catch (const std::regex_error& e) {
                context.error_collector->add_warning("Regex error in file '" + file_name + "': " + e.what());
            } catch (const std::exception& e) {
                context.error_collector->add_warning("Error processing line in file '" + file_name + "': " + e.what());
            }

            // Periodically check for shutdown
            if (line_count % 1000 == 0 && g_shutdown_requested.load()) {
                throw std::runtime_error("Processing interrupted by shutdown signal");
            }
        }
    } catch (const std::ios_base::failure& e) {
        throw std::runtime_error("I/O error reading file '" + file_name + "': " + e.what());
    }

    file.close();

    // Release memory used by the content buffer
    for (const auto& s : content) {
        context.memory_monitor->remove_usage(s.capacity());
    }

    // Process extracted data
    if (!scf_values.empty()) {
        scf = scf_values.back();
    }

    double lf = 0;
    if (!negative_freqs.empty()) {
        lf = negative_freqs.back();
    } else if (!positive_freqs.empty()) {
        lf = *std::min_element(positive_freqs.begin(), positive_freqs.end());
    }

    scf = scfEqui ? scfEqui : (scftd ? scftd : scf);
    etg = etg ? etg : 0.0;
    nucleare = nucleare ? nucleare : 0;
    zpe = zpe ? zpe : 0;

    double GphaseCorr = R * temp * std::log(context.concentration * R * temp / Po) * 0.0003808798033989866 / 1000;
    double GibbsFreeHartree = (phaseCorr == "YES" && etg != 0.0) ? etg + GphaseCorr : etg;
    double etgkj = GibbsFreeHartree * 2625.5002;

    // Determine status from content buffer
    for (const auto& content_line : content) {
        if (content_line.find("Normal termination") != std::string::npos) {
            status = "DONE";
            break;
        } else if (content_line.find("Error termination") != std::string::npos) {
            status = "ERROR";
            break;
        }
    }

    // Truncate filename if too long
    if (file_name.length() > 53) {
        file_name = file_name.substr(file_name.length() - 53);
    }

    return Result{file_name, etgkj, lf, GibbsFreeHartree, nucleare, scf, zpe, status, phaseCorr, copyright_count};
}

// Legacy function - now wraps the new job-aware implementation
unsigned int getSafeThreadCount(unsigned int requested_threads, unsigned int file_count) {
    JobResources job_resources = JobSchedulerDetector::detect_job_resources();
    return calculateSafeThreadCount(requested_threads, file_count, job_resources);
}

void processAndOutputResults(double temp, int C, int column, const std::string& extension,
                           bool quiet, const std::string& format, bool use_input_temp,
                           unsigned int requested_threads, size_t max_file_size_mb,
                           size_t memory_limit_mb, const std::vector<std::string>& warnings,
                           const JobResources& job_resources) {

    auto start_time = std::chrono::high_resolution_clock::now();

    try {
        // Detect job scheduler resources if not provided
        JobResources final_job_resources = job_resources;
        if (final_job_resources.scheduler_type == SchedulerType::NONE) {
            final_job_resources = JobSchedulerDetector::detect_job_resources();
        }

        // Print job information
        printJobResourceInfo(final_job_resources, quiet);

        // Find and validate log files
        std::vector<std::string> log_files = findLogFiles(extension, max_file_size_mb);

        if (log_files.empty()) {
            std::cerr << "No " << extension << " files found in the current directory." << std::endl;
            return;
        }

        if (g_shutdown_requested.load()) {
            std::cerr << "Shutdown requested during file discovery." << std::endl;
            return;
        }

        // Calculate job-aware safe thread count
        unsigned int num_threads = calculateSafeThreadCount(requested_threads,
                                                          static_cast<unsigned int>(log_files.size()),
                                                          final_job_resources);

        // Calculate job-aware memory limit
        size_t calculated_memory_limit = calculateSafeMemoryLimit(memory_limit_mb, num_threads, final_job_resources);

        if (!quiet) {
            std::cout << "Found " << log_files.size() << " " << extension << " files" << std::endl;

            // Debug thread calculation with detailed reasoning
            unsigned int hardware_cores = std::thread::hardware_concurrency();
            std::cout << "System: " << hardware_cores << " cores detected" << std::endl;
            std::cout << "Requested: " << requested_threads << " threads";
            if (requested_threads == hardware_cores / 2) {
                std::cout << " (default: half cores)";
            }
            std::cout << std::endl;

            // Show job environment
            if (final_job_resources.scheduler_type != SchedulerType::NONE) {
                std::cout << "Job scheduler: " << JobSchedulerDetector::scheduler_name(final_job_resources.scheduler_type);
                if (final_job_resources.has_cpu_limit) {
                    std::cout << " (CPU limit: " << final_job_resources.allocated_cpus << ")";
                } else {
                    std::cout << " (no CPU limits detected - interactive session)";
                }
                std::cout << std::endl;
            } else {
                std::cout << "Environment: Interactive/local execution" << std::endl;
            }

            std::cout << "Using: " << num_threads << " threads";
            if (num_threads < requested_threads) {
                std::cout << " (reduced for safety)";
            } else if (num_threads == requested_threads) {
                std::cout << " (as requested)";
            }
            std::cout << std::endl;

            std::cout << "Max file size limit: " << max_file_size_mb << " MB" << std::endl;

            if (memory_limit_mb > 0 && calculated_memory_limit < memory_limit_mb) {
                std::cout << "Note: Memory limit reduced from " << memory_limit_mb
                         << " MB to " << calculated_memory_limit << " MB due to job allocation" << std::endl;
            }
        }

        // Set up output file
        std::filesystem::path cwd = std::filesystem::current_path();
        std::string dir_name = cwd.filename().string();
        std::string output_extension = (format == "csv") ? ".csv" : ".results";
        std::string output_filename = dir_name + output_extension;

        std::ofstream output_file(output_filename);
        if (!output_file.is_open()) {
            throw std::runtime_error("Could not open output file: " + output_filename);
        }

        // Create processing context with job-aware memory limit
        ProcessingContext context(temp, C, use_input_temp, num_threads, extension, max_file_size_mb, job_resources);

        // Apply calculated memory limit
        context.memory_monitor->set_memory_limit(calculated_memory_limit);

        if (!quiet) {
            std::cout << "Memory limit: " << formatMemorySize(context.memory_monitor->get_max_usage());
            if (final_job_resources.has_memory_limit) {
                std::cout << " (job allocation: " << formatMemorySize(final_job_resources.allocated_memory_mb * 1024 * 1024) << ")";
            }
            std::cout << std::endl;
        }

        // Thread-safe result collection
        std::vector<Result> results;
        std::mutex results_mutex;
        std::atomic<size_t> file_index(0);
        std::atomic<size_t> completed_files(0);

        // Worker function with comprehensive error handling
        auto worker_function = [&]() {
            while (!g_shutdown_requested.load()) {
                size_t i = file_index.fetch_add(1);
                if (i >= log_files.size()) {
                    break;
                }

                const std::string& file = log_files[i];

                try {
                    Result res = extract(file, context);

                    {
                        std::lock_guard<std::mutex> lock(results_mutex);
                        results.push_back(res);
                    }

                    size_t completed = completed_files.fetch_add(1) + 1;

                    // Progress reporting (every 10% or every 100 files, whichever is smaller)
                    size_t progress_interval = std::max(1UL, std::min(log_files.size() / 10, 100UL));
                    if (!quiet && completed % progress_interval == 0) {
                        std::cout << "Processed " << completed << "/" << log_files.size()
                                 << " files (" << (completed * 100 / log_files.size()) << "%)" << std::endl;
                    }

                } catch (const std::exception& e) {
                    context.error_collector->add_error("Error processing file '" + file + "': " + e.what());
                    completed_files.fetch_add(1);
                } catch (...) {
                    context.error_collector->add_error("Unknown error processing file: " + file);
                    completed_files.fetch_add(1);
                }
            }
        };

        // Launch worker threads
        std::vector<std::future<void>> futures;
        for (unsigned int i = 0; i < num_threads; ++i) {
            futures.emplace_back(std::async(std::launch::async, worker_function));
        }

        // Wait for all threads to complete
        for (auto& future : futures) {
            try {
                future.get();
            } catch (const std::exception& e) {
                context.error_collector->add_error("Thread execution error: " + std::string(e.what()));
            }
        }

        // Check for shutdown or critical errors
        if (g_shutdown_requested.load()) {
            std::cerr << "Processing interrupted by shutdown signal." << std::endl;
            std::cerr << "Processed " << completed_files.load() << "/" << log_files.size() << " files before interruption." << std::endl;
        }

        if (results.empty()) {
            std::cerr << "No valid results were extracted." << std::endl;

            // Report errors
            auto errors = context.error_collector->get_errors();
            if (!errors.empty()) {
                std::cerr << "\nErrors encountered:" << std::endl;
                for (const auto& error : errors) {
                    std::cerr << "  " << error << std::endl;
                }
            }
            return;
        }

        // Sort results
        std::sort(results.begin(), results.end(), [column](const Result& a, const Result& b) {
            return compareResults(a, b, column);
        });

        // Prepare header information
        std::ostringstream params;
        
        // Create dynamic header with proper formatting
        auto create_header_line = [](const std::string& content) -> std::string {
            const size_t total_width = 61;  // Including asterisks
            std::string line = "* " + content;
            while (line.length() < total_width - 2) line += " ";
            line += " *";
            return line;
        };
        
        params << "                                                             \n"
               << "*************************************************************\n"
               << create_header_line(GaussianExtractor::get_header_info()) << "\n"
               << create_header_line(GAUSSIAN_EXTRACTOR_REPOSITORY) << "\n"
               << "*************************************************************\n"
               << "                                                             \n";

        if (use_input_temp) {
            params << "Using specified temperature for all files: " << std::fixed << std::setprecision(3) << temp << " K\n";
        } else {
            params << "Default temperature for files without specified temp: " << std::fixed << std::setprecision(3) << temp << " K\n";
        }

        params << "The concentration for phase correction: " << C / 1000 << " M or " << C << " mol/m3\n";
        double representative_GphaseCorr = R * temp * std::log(C * R * temp / Po) * 0.0003808798033989866 / 1000;
        params << "Representative Gibbs free correction for phase changing at " << std::fixed << std::setprecision(3)
               << temp << " K: " << std::fixed << std::setprecision(6) << representative_GphaseCorr << " au\n";
        params << "Using " << num_threads << " threads for processing.\n";
        params << "Successfully processed " << results.size() << "/" << log_files.size() << " files.\n";

        // Add resource usage info
        params << "Peak memory usage: " << formatMemorySize(context.memory_monitor->get_peak_usage()) << "\n";

        // Add warnings and errors
        std::vector<std::string> all_warnings = warnings;
        auto processing_warnings = context.error_collector->get_warnings();
        all_warnings.insert(all_warnings.end(), processing_warnings.begin(), processing_warnings.end());

        auto processing_errors = context.error_collector->get_errors();

        if (!all_warnings.empty() || !processing_errors.empty()) {
            params << "\n-------------------------------------------------------------\n";

            if (!all_warnings.empty()) {
                params << "Warnings:\n";
                for (const auto& warning : all_warnings) {
                    params << "- " << warning << "\n";
                }
            }

            if (!processing_errors.empty()) {
                params << "Errors:\n";
                for (const auto& error : processing_errors) {
                    params << "- " << error << "\n";
                }
            }

            params << "-------------------------------------------------------------\n";
        }

        // Generate output
        std::ostringstream output_stream;

        if (format == "text") {
            // Text format output
            std::ostringstream header;
            header << std::setw(53) << std::left << "Output name"
                   << std::setw(18) << std::right << "ETG kJ/mol"
                   << std::setw(10) << std::right << "Low FC"
                   << std::setw(18) << std::right << "ETG a.u"
                   << std::setw(18) << std::right << "Nuclear E au"
                   << std::setw(18) << std::right << "SCFE"
                   << std::setw(10) << std::right << "ZPE "
                   << std::setw(8) << std::right << "Status"
                   << std::setw(6) << std::right << "PCorr"
                   << std::setw(6) << std::right << "Round" << "\n";

            std::ostringstream separator;
            separator << std::setw(53) << std::left << std::string(53, '-')
                      << std::setw(18) << std::right << std::string(18, '-')
                      << std::setw(10) << std::right << std::string(10, '-')
                      << std::setw(18) << std::right << std::string(18, '-')
                      << std::setw(18) << std::right << std::string(18, '-')
                      << std::setw(18) << std::right << std::string(18, '-')
                      << std::setw(10) << std::right << std::string(10, '-')
                      << std::setw(8) << std::right << std::string(8, '-')
                      << std::setw(6) << std::right << std::string(6, '-')
                      << std::setw(6) << std::right << std::string(6, '-') << "\n";

            for (const auto& result : results) {
                output_stream << std::setw(53) << std::left << result.file_name
                              << std::setw(18) << std::right << std::fixed << std::setprecision(6) << result.etgkj
                              << std::setw(10) << std::right << std::fixed << std::setprecision(2) << result.lf
                              << std::setw(18) << std::right << std::fixed << std::setprecision(6) << result.GibbsFreeHartree
                              << std::setw(18) << std::right << std::fixed << std::setprecision(6) << result.nucleare
                              << std::setw(18) << std::right << std::fixed << std::setprecision(6) << result.scf
                              << std::setw(10) << std::right << std::fixed << std::setprecision(6) << result.zpe
                              << std::setw(8) << std::right << result.status
                              << std::setw(6) << std::right << result.phaseCorr
                              << std::setw(6) << std::right << result.copyright_count << "\n";
            }

            output_file << params.str() << header.str() << separator.str() << output_stream.str();
            if (!quiet) {
                std::cout << params.str() << header.str() << separator.str() << output_stream.str();
            }

        } else if (format == "csv") {
            // CSV format output
            output_stream << "Output name,ETG kJ/mol,Low FC,ETG a.u,Nuclear E au,SCFE,ZPE,Status,PCorr,Round\n";
            for (const auto& result : results) {
                output_stream << "\"" << result.file_name << "\","
                              << std::fixed << std::setprecision(6) << result.etgkj << ","
                              << std::fixed << std::setprecision(2) << result.lf << ","
                              << std::fixed << std::setprecision(6) << result.GibbsFreeHartree << ","
                              << std::fixed << std::setprecision(6) << result.nucleare << ","
                              << std::fixed << std::setprecision(6) << result.scf << ","
                              << std::fixed << std::setprecision(6) << result.zpe << ","
                              << result.status << ","
                              << result.phaseCorr << ","
                              << result.copyright_count << "\n";
            }

            output_file << params.str() << output_stream.str();
            if (!quiet) {
                std::cout << params.str() << output_stream.str();
            }

        } else {
            throw std::runtime_error("Invalid format '" + format + "'. Supported formats: 'text', 'csv'.");
        }

        output_file.close();

        // Final summary
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end_time - start_time;

        if (!quiet) {
            std::cout << "\nResults written to " << output_filename << std::endl;
            std::cout << "Total execution time: " << std::fixed << std::setprecision(3)
                      << duration.count() << " seconds" << std::endl;

            // Final resource usage
            printResourceUsage(context, false);

        } else {
            std::cout << "Processed " << results.size() << "/" << log_files.size()
                      << " files. Results written to " << output_filename
                      << " (execution time: " << std::fixed << std::setprecision(1)
                      << duration.count() << "s)" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error in processAndOutputResults: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "Unknown fatal error in processAndOutputResults" << std::endl;
        throw;
    }
}
