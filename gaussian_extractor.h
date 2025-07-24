#ifndef GAUSSIAN_EXTRACTOR_H
#define GAUSSIAN_EXTRACTOR_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cstddef>
#include <functional>
#if __cpp_lib_semaphore >= 201907L
#include <semaphore>
#else
// Fallback for systems without C++20 semaphore support
#include <mutex>
#include <condition_variable>
#endif
#include "job_scheduler.h"

// Constants
extern const double R;           // J/K/mol
extern const double Po;          // 1 atm in N/m^2
extern const double kB;          // Boltzmann constant in Hartree/K

// Safety limits
const size_t DEFAULT_MEMORY_MB = 4096;      // Default 4GB memory limit
const size_t MIN_MEMORY_MB = 1024;          // Minimum 1GB memory limit
const size_t MAX_MEMORY_MB = 32768;         // Maximum 32GB memory limit
const size_t MAX_FILE_HANDLES = 20;         // Max concurrent file operations
const size_t DEFAULT_MAX_FILE_SIZE_MB = 100;  // Default max individual file size
const size_t CONTENT_BUFFER_SIZE = 50;      // Lines to keep in memory

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

// Resource management classes
class MemoryMonitor {
private:
    std::atomic<size_t> current_usage_bytes{0};
    std::atomic<size_t> peak_usage_bytes{0};
    size_t max_bytes;

public:
    explicit MemoryMonitor(size_t max_memory_mb = DEFAULT_MEMORY_MB);
    bool can_allocate(size_t bytes);
    void add_usage(size_t bytes);
    void remove_usage(size_t bytes);
    size_t get_current_usage() const;
    size_t get_peak_usage() const;
    size_t get_max_usage() const;
    void set_memory_limit(size_t max_memory_mb);
    static size_t get_system_memory_mb();
    static size_t calculate_optimal_memory_limit(unsigned int thread_count, size_t system_memory_mb = 0);
};

class FileHandleManager {
private:
#if __cpp_lib_semaphore >= 201907L
    std::counting_semaphore<MAX_FILE_HANDLES> semaphore{MAX_FILE_HANDLES};
#else
    // Fallback implementation using mutex and condition variable
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    std::atomic<int> available_handles{MAX_FILE_HANDLES};
#endif

public:
    class FileGuard {
    private:
        FileHandleManager* manager;
        bool acquired;

    public:
        explicit FileGuard(FileHandleManager* mgr);
        ~FileGuard();
        FileGuard(const FileGuard&) = delete;
        FileGuard& operator=(const FileGuard&) = delete;
        FileGuard(FileGuard&& other) noexcept;
        FileGuard& operator=(FileGuard&& other) noexcept;
        bool is_acquired() const { return acquired; }
    };

    FileGuard acquire();
    void release();
};

class ThreadSafeErrorCollector {
private:
    mutable std::mutex error_mutex;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

public:
    void add_error(const std::string& error);
    void add_warning(const std::string& warning);
    std::vector<std::string> get_errors() const;
    std::vector<std::string> get_warnings() const;
    bool has_errors() const;
    void clear();
};

// Processing context for thread safety
struct ProcessingContext {
    std::shared_ptr<MemoryMonitor> memory_monitor;
    std::shared_ptr<FileHandleManager> file_manager;
    std::shared_ptr<ThreadSafeErrorCollector> error_collector;
    double base_temp;
    int concentration;
    bool use_input_temp;
    
    ProcessingContext(double temp, int C, bool use_temp, unsigned int thread_count = 1)
        : memory_monitor(std::make_shared<MemoryMonitor>(MemoryMonitor::calculate_optimal_memory_limit(thread_count)))
        , file_manager(std::make_shared<FileHandleManager>())
        , error_collector(std::make_shared<ThreadSafeErrorCollector>())
        , base_temp(temp)
        , concentration(C)
        , use_input_temp(use_temp) {}
};

// Function declarations
bool compareResults(const Result& a, const Result& b, int column);

Result extract(const std::string& file_name_param, 
               const ProcessingContext& context);

void processAndOutputResults(double temp, 
                           int C, 
                           int column, 
                           const std::string& extension, 
                           bool quiet, 
                           const std::string& format, 
                           bool use_input_temp, 
                           unsigned int requested_threads, 
                           size_t max_file_size_mb,
                           size_t memory_limit_mb,
                           const std::vector<std::string>& warnings,
                           const JobResources& job_resources = JobResources{});

// Utility functions
std::vector<std::string> findLogFiles(const std::string& extension, size_t max_file_size_mb = DEFAULT_MAX_FILE_SIZE_MB);
bool validateFileSize(const std::string& filename, size_t max_size_mb = DEFAULT_MAX_FILE_SIZE_MB);
std::string formatMemorySize(size_t bytes);
void printResourceUsage(const ProcessingContext& context, bool quiet = false);
void printJobResourceInfo(const JobResources& job_resources, bool quiet = false);

// Job-aware resource calculation
unsigned int calculateSafeThreadCount(unsigned int requested_threads, 
                                     unsigned int file_count, 
                                     const JobResources& job_resources);
size_t calculateSafeMemoryLimit(size_t requested_memory_mb, 
                               unsigned int thread_count,
                               const JobResources& job_resources);

// Safe string parsing functions
bool safe_stod(const std::string& str, double& result);
bool safe_stoi(const std::string& str, int& result);
bool safe_stoul(const std::string& str, unsigned long& result);

#endif // GAUSSIAN_EXTRACTOR_H