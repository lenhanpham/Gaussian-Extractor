/**
 * @file gaussian_extractor.h
 * @brief Core extraction engine and data structures for Gaussian log file processing
 * @author Le Nhan Pham
 * @date 2025
 *
 * This header file contains the core functionality for extracting thermodynamic
 * data from Gaussian computational chemistry log files. It provides:
 * - Multi-threaded file processing with resource management
 * - Thread-safe data extraction and result collection
 * - Memory and file handle management for large-scale processing
 * - Integration with job scheduler systems for HPC environments
 *
 * @section Key Features
 * - Concurrent processing of multiple log files
 * - Automatic resource management and memory limiting
 * - Thread-safe error collection and reporting
 * - Support for various thermodynamic calculations
 * - Integration with configuration and job management systems
 *
 * @section Threading Model
 * The extraction system uses a producer-consumer model with:
 * - File discovery and validation (single-threaded)
 * - Parallel file processing (multi-threaded)
 * - Result collection and sorting (thread-safe)
 * - Output generation (single-threaded)
 *
 * @section Resource Management
 * Comprehensive resource management includes:
 * - Memory usage monitoring and limiting
 * - File handle management to prevent exhaustion
 * - Thread-safe error and warning collection
 * - Integration with job scheduler resource limits
 */

#ifndef GAUSSIAN_EXTRACTOR_H
#define GAUSSIAN_EXTRACTOR_H

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#if __cpp_lib_semaphore >= 201907L
    #include <semaphore>
#else
    // Fallback for systems without C++20 semaphore support
    #include <condition_variable>
    #include <mutex>
#endif
#include "job_management/job_scheduler.h"

/**
 * @brief Global flag for graceful termination of long-running operations
 *
 * This atomic boolean is used to coordinate graceful shutdown across all threads
 * when a termination signal is received or when the user requests cancellation.
 * All long-running loops and operations should periodically check this flag.
 *
 * @note This is defined in main.cpp and set by the signal handler
 */
extern std::atomic<bool> g_shutdown_requested;

/**
 * @defgroup PhysicsConstants Physical Constants
 * @brief Fundamental physical constants used in thermodynamic calculations
 * @{
 */

/**
 * @brief Universal gas constant in J/(K·mol)
 *
 * Used for phase corrections and thermodynamic calculations.
 * Value: 8.314462618 J/(K·mol)
 */
extern const double R;

/**
 * @brief Standard atmospheric pressure in N/m²
 *
 * Reference pressure for standard state calculations.
 * Value: 101325.0 N/m² (1 atm)
 */
extern const double Po;

/**
 * @brief Boltzmann constant in Hartree/K
 *
 * Used for temperature-dependent energy calculations in atomic units.
 * Value: 3.166811563×10⁻⁶ Hartree/K
 */
extern const double kB;

/** @} */  // end of PhysicsConstants group

/**
 * @defgroup SafetyLimits Resource Safety Limits
 * @brief Constants defining safe operating limits for resource usage
 * @{
 */
const size_t DEFAULT_MEMORY_MB        = 4096;   ///< Default memory limit: 4GB
const size_t MIN_MEMORY_MB            = 1024;   ///< Minimum safe memory limit: 1GB
const size_t MAX_MEMORY_MB            = 32768;  ///< Maximum memory limit: 32GB
const size_t MAX_FILE_HANDLES         = 20;     ///< Maximum concurrent file operations
const size_t DEFAULT_MAX_FILE_SIZE_MB = 100;    ///< Default maximum individual file size: 100MB

/** @} */  // end of SafetyLimits group

/**
 * @struct Result
 * @brief Structure containing extracted thermodynamic data from a single Gaussian log file
 *
 * This structure holds all relevant thermodynamic and electronic structure data
 * extracted from a Gaussian calculation log file. It serves as the primary data
 * container for results that are collected, sorted, and output by the system.
 *
 * @section Energy Units
 * - Electronic energies: Hartree (atomic units)
 * - Thermal energies: Various units as specified
 * - Phase corrections: Applied as specified by concentration and temperature
 *
 * @section Data Sources
 * Different fields are extracted from different sections of the log file:
 * - SCF energies: From SCF convergence sections
 * - Thermal corrections: From frequency calculation sections
 * - Status information: From job termination sections
 */
struct Result
{
    std::string file_name;         ///< Original log file name (without path)
    double      etgkj;             ///< Electronic + thermal energy in kJ/mol
    double      lf;                ///< Lowest vibrational frequency (cm⁻¹)
    double      GibbsFreeHartree;  ///< Gibbs free energy in Hartree (with corrections)
    double      nucleare;          ///< Nuclear repulsion energy in Hartree
    double      scf;               ///< Final SCF energy in Hartree
    double      zpe;               ///< Zero-point energy correction in Hartree
    std::string status;            ///< Job termination status ("DONE", "ERROR", etc.)
    std::string phaseCorr;         ///< Phase correction applied ("Yes", "No", or value)
    int         copyright_count;   ///< Number of Gaussian copyright notices (job progress indicator)
};

/**
 * @class MemoryMonitor
 * @brief Thread-safe memory usage tracking and limiting system
 *
 * The MemoryMonitor class provides centralized memory usage tracking to prevent
 * the application from exceeding system memory limits during intensive processing.
 * It uses atomic operations for thread-safe tracking and provides both current
 * and peak usage statistics.
 *
 * @section Memory Features
 * - Thread-safe memory usage tracking
 * - Configurable memory limits with safety margins
 * - Peak usage tracking for performance analysis
 * - Integration with system memory detection
 * - Automatic memory limit calculation based on thread count
 *
 * @section MemoryMonitor Usage Pattern
 * 1. Create MemoryMonitor with desired limit
 * 2. Check can_allocate() before
 * large allocations
 * 3. Call add_usage() when allocating memory
 * 4. Call remove_usage() when freeing memory
 * 5.
 * Monitor current and peak usage for optimization
 *
 * @note All methods are thread-safe and can be called from multiple threads
 *       simultaneously without external synchronization
 */
class MemoryMonitor
{
private:
    std::atomic<size_t> current_usage_bytes{0};
    std::atomic<size_t> peak_usage_bytes{0};
    size_t              max_bytes;

public:
    /**
     * @brief Constructor with memory limit specification
     * @param max_memory_mb Maximum memory usage limit in megabytes
     *
     * Creates a memory monitor with the specified limit. The limit should
     * account for other system processes and leave sufficient free memory.
     */
    explicit MemoryMonitor(size_t max_memory_mb = DEFAULT_MEMORY_MB);

    /**
     * @brief Check if a memory allocation would exceed the limit
     * @param bytes Number of bytes to potentially allocate
     * @return true if allocation is safe, false if it would exceed limit
     *
     * Thread-safe check that should be called before large allocations
     * to prevent memory exhaustion.
     */
    bool can_allocate(size_t bytes);

    /**
     * @brief Record memory allocation in usage tracking
     * @param bytes Number of bytes allocated
     *
     * Updates current usage atomically and tracks peak usage.
     * Should be called immediately after successful allocation.
     */
    void add_usage(size_t bytes);

    /**
     * @brief Record memory deallocation in usage tracking
     * @param bytes Number of bytes freed
     *
     * Updates current usage atomically. Should be called when
     * memory is freed or objects are destroyed.
     */
    void remove_usage(size_t bytes);

    /**
     * @brief Get current memory usage
     * @return Current memory usage in bytes
     */
    size_t get_current_usage() const;

    /**
     * @brief Get peak memory usage since monitor creation
     * @return Peak memory usage in bytes
     */
    size_t get_peak_usage() const;

    /**
     * @brief Get configured memory limit
     * @return Maximum allowed memory usage in bytes
     */
    size_t get_max_usage() const;

    /**
     * @brief Update memory limit during runtime
     * @param max_memory_mb New memory limit in megabytes
     *
     * Allows dynamic adjustment of memory limits based on
     * system conditions or user preferences.
     */
    void set_memory_limit(size_t max_memory_mb);

    /**
     * @brief Detect system memory capacity
     * @return Total system memory in megabytes
     *
     * Platform-specific detection of available system memory
     * for automatic limit calculation.
     */
    static size_t get_system_memory_mb();

    /**
     * @brief Calculate optimal memory limit for given thread count
     * @param thread_count Number of threads that will be processing
     * @param system_memory_mb Total system memory (0 = auto-detect)
     * @return Recommended memory limit in megabytes
     *
     * Calculates a safe memory limit that accounts for:
     * - System memory capacity
     * - Number of processing threads
     * - Memory overhead for other processes
     * - Safety margins for system stability
     */
    static size_t calculate_optimal_memory_limit(unsigned int thread_count, size_t system_memory_mb = 0);
};

/**
 * @class FileHandleManager
 * @brief Thread-safe file handle resource management system
 *
 * The FileHandleManager prevents file handle exhaustion by limiting the number
 * of files that can be open simultaneously across all threads. It uses either
 * C++20 counting_semaphore (preferred) or a fallback implementation with mutex
 * and condition variables for older compilers.
 *
 * @section Design
 * The manager uses RAII (Resource Acquisition Is Initialization) through the
 * FileGuard class to ensure file handles are properly released even if
 * exceptions occur during file processing.
 *
 * @section FileHandleManager Usage Pattern
 * 1. Call acquire() to get a FileGuard
 * 2. Check is_acquired() to ensure
 * handle was obtained
 * 3. Open file if guard is acquired
 * 4. Process file content
 * 5. FileGuard destructor
 * automatically releases handle
 *
 * @note The manager is designed to prevent system file handle exhaustion
 *       which can cause application crashes or system instability
 */
class FileHandleManager
{
private:
#if __cpp_lib_semaphore >= 201907L
    std::counting_semaphore<MAX_FILE_HANDLES> semaphore{MAX_FILE_HANDLES};  ///< C++20 semaphore for handle counting
#else
    // Fallback implementation using mutex and condition variable
    mutable std::mutex              mutex_;  ///< Mutex for fallback synchronization
    mutable std::condition_variable cv_;     ///< Condition variable for handle availability
    std::atomic<int>                available_handles{MAX_FILE_HANDLES};  ///< Available handle count
#endif

public:
    /**
     * @class FileGuard
     * @brief RAII guard for automatic file handle management
     *
     * The FileGuard class provides automatic file handle acquisition and release
     * using RAII principles. It ensures that file handles are properly released
     * even if exceptions occur during file processing.
     *
     * @section FileGuard Features
     * - Automatic handle release in destructor
     * - Move semantics for efficient transfer
     * - Non-copyable to prevent handle duplication
     * - Status checking to verify successful acquisition
     *
     * @note FileGuard objects should be short-lived and created immediately
     *       before file operations to minimize handle holding time
     */
    class FileGuard
    {
    private:
        FileHandleManager* manager;   ///< Pointer to managing FileHandleManager
        bool               acquired;  ///< Whether handle was successfully acquired

    public:
        /**
         * @brief Constructor - attempts to acquire file handle
         * @param mgr Pointer to the FileHandleManager
         *
         * Attempts to acquire a file handle from the manager. Check is_acquired()
         * to determine if the acquisition was successful.
         */
        explicit FileGuard(FileHandleManager* mgr);

        /**
         * @brief Destructor - automatically releases file handle
         *
         * Releases the file handle back to the manager if it was successfully
         * acquired. This ensures proper cleanup even in exception scenarios.
         */
        ~FileGuard();

        /**
         * @brief Copy constructor (deleted)
         *
         * FileGuard is non-copyable to prevent accidental handle duplication
         * which could lead to resource leaks or double-release errors.
         */
        FileGuard(const FileGuard&) = delete;

        /**
         * @brief Copy assignment operator (deleted)
         *
         * FileGuard is non-copyable to prevent accidental handle duplication
         * which could lead to resource leaks or double-release errors.
         */
        FileGuard& operator=(const FileGuard&) = delete;

        /**
         * @brief Move constructor for efficient transfer
         * @param other FileGuard to move from
         *
         * Transfers ownership of the file handle from another FileGuard.
         * The source guard becomes invalid after the move.
         */
        FileGuard(FileGuard&& other) noexcept;

        /**
         * @brief Move assignment operator for efficient transfer
         * @param other FileGuard to move from
         * @return Reference to this FileGuard
         *
         * Transfers ownership of the file handle from another FileGuard.
         * Releases any currently held handle before acquiring the new one.
         */
        FileGuard& operator=(FileGuard&& other) noexcept;

        /**
         * @brief Check if file handle was successfully acquired
         * @return true if handle is available for use, false otherwise
         *
         * Always check this before attempting file operations to ensure
         * a handle is available. If false, defer the operation or retry later.
         */
        bool is_acquired() const
        {
            return acquired;
        }
    };

    /**
     * @brief Acquire a file handle guard
     * @return FileGuard object (check is_acquired() for success)
     *
     * Attempts to acquire a file handle and returns a FileGuard for RAII
     * management. The guard may not have successfully acquired a handle
     * if all handles are currently in use.
     */
    FileGuard acquire();

    /**
     * @brief Release a file handle back to the pool
     *
     * Manually releases a file handle. Typically not needed as FileGuard
     * handles automatic release, but available for advanced usage patterns.
     *
     * @note This should only be called if a handle was successfully acquired
     */
    void release();
};

/**
 * @class ThreadSafeErrorCollector
 * @brief Thread-safe collection and management of error and warning messages
 *
 * The ThreadSafeErrorCollector provides a centralized, thread-safe mechanism
 * for collecting error and warning messages from multiple processing threads.
 * It ensures that messages are properly synchronized and can be safely
 * retrieved for display or logging.
 *
 * @section Thread Safety
 * All methods use internal synchronization to ensure thread safety:
 * - Multiple threads can add messages simultaneously
 * - Message retrieval is atomic and returns consistent snapshots
 * - No external synchronization is required by users
 *
 * @section Message Categories
 * - Errors: Critical issues that prevent successful processing
 * - Warnings: Non-critical issues that users should be aware of
 *
 * @note Error and warning collections are maintained separately to allow
 *       different handling and display strategies for each category
 */

class ThreadSafeErrorCollector
{
private:
    mutable std::mutex       error_mutex;  ///< Mutex for thread-safe access to error collections
    std::vector<std::string> errors;       ///< Collection of error messages
    std::vector<std::string> warnings;     ///< Collection of warning messages

public:
    /**
     * @brief Add an error message to the collection
     * @param error Error message to add
     *
     * Thread-safe addition of error messages. Multiple threads can call
     * this method simultaneously without external synchronization.
     *
     * @note Error messages indicate critical issues that prevent successful
     *       completion of operations
     */
    void add_error(const std::string& error);

    /**
     * @brief Add a warning message to the collection
     * @param warning Warning message to add
     *
     * Thread-safe addition of warning messages. Multiple threads can call
     * this method simultaneously without external synchronization.
     *
     * @note Warning messages indicate non-critical issues that users should
     *       be aware of but don't prevent operation completion
     */
    void add_warning(const std::string& warning);

    /**
     * @brief Retrieve all collected error messages
     * @return Vector containing all error messages
     *
     * Returns a snapshot of all error messages collected so far.
     * The returned vector is a copy and can be safely used without
     * additional synchronization.
     */
    std::vector<std::string> get_errors() const;

    /**
     * @brief Retrieve all collected warning messages
     * @return Vector containing all warning messages
     *
     * Returns a snapshot of all warning messages collected so far.
     * The returned vector is a copy and can be safely used without
     * additional synchronization.
     */
    std::vector<std::string> get_warnings() const;

    /**
     * @brief Check if any error messages have been collected
     * @return true if errors exist, false otherwise
     *
     * Thread-safe check for the presence of error messages.
     * Useful for determining if operations completed successfully.
     */
    bool has_errors() const;

    /**
     * @brief Clear all collected messages
     *
     * Removes all error and warning messages from the collections.
     * Thread-safe operation that can be called to reset the collector
     * for reuse in subsequent operations.
     */
    void clear();
};


/**
 * @struct ProcessingContext
 * @brief Complete context for thread-safe file processing operations
 *
 * The ProcessingContext encapsulates all resources, parameters, and state
 * needed for safe multi-threaded processing of Gaussian log files. It provides
 * shared access to resource managers and maintains processing parameters
 * consistent across all worker threads.
 *
 * @section Design Philosophy
 * - Immutable after construction to prevent race conditions
 * - Shared resource managers for coordinated resource usage
 * - Integration with job scheduler resource limits
 * - Parameter validation and safe defaults
 *
 * @section Resource Management
 * The context owns shared resource managers that coordinate access to:
 * - System memory through MemoryMonitor
 * - File handles through FileHandleManager
 * - Error collection through ThreadSafeErrorCollector
 *
 * @note All resource managers are thread-safe and can be accessed
 *       simultaneously from multiple processing threads
 */
struct ProcessingContext
{
    std::shared_ptr<MemoryMonitor>            memory_monitor;     ///< Shared memory usage monitor
    std::shared_ptr<FileHandleManager>        file_manager;       ///< Shared file handle manager
    std::shared_ptr<ThreadSafeErrorCollector> error_collector;    ///< Shared error collector
    double                                    base_temp;          ///< Base temperature for calculations (K)
    int                                       concentration;      ///< Concentration for phase corrections (mM)
    bool                                      use_input_temp;     ///< Whether to use temperature from input files
    std::string                               extension;          ///< File extension to process
    unsigned int                              requested_threads;  ///< Number of requested processing threads
    size_t                                    max_file_size_mb;   ///< Maximum individual file size in MB
    JobResources                              job_resources;      ///< Job scheduler resource information

    /**
     * @brief Constructor with parameter validation and resource setup
     * @param temp Base temperature for thermodynamic calculations (K)
     * @param C Concentration for phase corrections (mM)
     * @param use_temp Whether to use temperatures from input files
     * @param thread_count Number of processing threads to use
     * @param ext File extension to process (default: ".log")
     * @param max_file_mb Maximum individual file size in MB (default: 100)
     * @param job_res Job scheduler resource information
     *
     * Creates a complete processing context with:
     * - Optimally configured resource managers
     * - Validated processing parameters
     * - Integration with job scheduler limits
     * - Thread-safe resource sharing setup
     *
     * @note Memory limits are automatically calculated based on thread count
     *       and available system resources
     */
    ProcessingContext(double              temp,
                      int                 C,
                      bool                use_temp,
                      unsigned int        thread_count = 1,
                      const std::string&  ext          = ".log",
                      size_t              max_file_mb  = DEFAULT_MAX_FILE_SIZE_MB,
                      const JobResources& job_res      = JobResources{})
        : memory_monitor(std::make_shared<MemoryMonitor>(MemoryMonitor::calculate_optimal_memory_limit(thread_count))),
          file_manager(std::make_shared<FileHandleManager>()),
          error_collector(std::make_shared<ThreadSafeErrorCollector>()), base_temp(temp), concentration(C),
          use_input_temp(use_temp), extension(ext), requested_threads(thread_count), max_file_size_mb(max_file_mb),
          job_resources(job_res)
    {}
};

/**
 * @defgroup CoreFunctions Core Extraction Functions
 * @brief Primary functions for Gaussian log file processing and data extraction
 * @{
 */

/**
 * @brief Compare two Result structures for sorting
 * @param a First Result to compare
 * @param b Second Result to compare
 * @param column Column number to use for comparison (1-based indexing)
 * @return true if a should be ordered before b, false otherwise
 *
 * Provides flexible sorting of extraction results based on different columns:
 * - Column 1: File name (alphabetical)
 * - Column 2: Electronic + thermal energy (etgkj)
 * - Column 3: Lowest frequency (lf)
 * - Column 4: Gibbs free energy (GibbsFreeHartree)
 * - Other columns: Additional energy components
 *
 * @note Used by std::sort() to order results before output
 */
bool compareResults(const Result& a, const Result& b, int column);

/**
 * @brief Extract thermodynamic data from a single Gaussian log file
 * @param file_name_param Path to the Gaussian log file to process
 * @param context Processing context with parameters and resource managers
 * @return Result structure containing extracted thermodynamic data
 *
 * This is the core extraction function that processes a single Gaussian log file
 * and extracts all relevant thermodynamic and electronic structure data.
 *
 * @section Extracted Data
 * - Electronic energies (SCF, correlation, etc.)
 * - Thermal corrections (ZPE, enthalpy, Gibbs)
 * - Vibrational frequencies (especially lowest frequency)
 * - Job status and completion information
 * - Phase corrections based on concentration and temperature
 *
 * @section Error Handling
 * - File access errors are collected in context error collector
 * - Parsing errors result in default/invalid values with error status
 * - Resource limits are respected through context resource managers
 *
 * @section Thread Safety
 * This function is thread-safe and can be called concurrently from multiple
 * threads using the same ProcessingContext for resource coordination.
 *
 * @note The function respects global shutdown requests and will terminate
 *       gracefully if g_shutdown_requested becomes true
 */
Result extract(const std::string& file_name_param, const ProcessingContext& context);

/**
 * @brief Process multiple files and output formatted results
 * @param temp Temperature for thermodynamic calculations (K)
 * @param C Concentration for phase corrections (mM)
 * @param column Column number for result sorting (1-based)
 * @param extension File extension to search for
 * @param quiet Suppress non-essential output
 * @param format Output format ("text", "csv", etc.)
 * @param use_input_temp Use temperatures from input files rather than fixed temperature
 * @param requested_threads Number of processing threads to use
 * @param max_file_size_mb Maximum individual file size to process (MB)
 * @param memory_limit_mb Total memory usage limit (MB, 0 = auto)
 * @param warnings Vector of warnings to display before processing
 * @param job_resources Job scheduler resource information
 *
 * This is the main orchestration function that coordinates the complete
 * processing workflow:
 *
 * @section Processing Workflow
 * 1. File discovery and validation
 * 2. Resource allocation and thread pool setup
 * 3. Parallel file processing with progress reporting
 * 4. Result collection and sorting
 * 5. Formatted output generation
 * 6. Resource cleanup and statistics reporting
 *
 * @section Resource Management
 * - Automatically determines optimal thread count based on system resources
 * - Manages memory usage to prevent system overload
 * - Coordinates file handle allocation across threads
 * - Provides progress reporting for long-running operations
 *
 * @section Output Formats
 * - text: Human-readable tabular format
 * - csv: Comma-separated values for spreadsheet import
 * - Additional formats may be supported in future versions
 *
 * @note This function handles all aspects of multi-threaded processing
 *       including graceful shutdown, error collection, and resource cleanup
 */
void processAndOutputResults(double                          temp,
                             int                             C,
                             int                             column,
                             const std::string&              extension,
                             bool                            quiet,
                             const std::string&              format,
                             bool                            use_input_temp,
                             unsigned int                    requested_threads,
                             size_t                          max_file_size_mb,
                             size_t                          memory_limit_mb,
                             const std::vector<std::string>& warnings,
                             const JobResources&             job_resources = JobResources{},
                             size_t                          batch_size    = 0);

/** @} */  // end of CoreFunctions group

/**
 * @defgroup UtilityFunctions Utility Functions
 * @brief Helper functions for file operations, formatting, and resource reporting
 * @{
 */

/**
 * @brief Find all Gaussian log files in the current directory
 * @param extension File extension to search for (e.g., ".log", ".out")
 * @param max_file_size_mb Maximum file size to include (MB)
 * @return Vector of file paths that match the criteria
 *
 * Recursively searches the current directory for files matching the specified
 * extension and size constraints. Files exceeding the size limit are skipped
 * with warnings to prevent memory exhaustion.
 *
 * @note The function filters out files that are too large to process safely
 *       based on available system resources
 */
// Optimized version that uses direct iteration and sorting for consistent file processing order
std::vector<std::string> findLogFiles(const std::string& extension, size_t max_file_size_mb = DEFAULT_MAX_FILE_SIZE_MB);
// Batch processing version for handling millions of files with controlled memory usage
std::vector<std::string> findLogFiles(const std::string& extension, size_t max_file_size_mb, size_t batch_size);
// Multiple extensions version
std::vector<std::string> findLogFiles(const std::vector<std::string>& extensions, size_t max_file_size_mb);
// Batch processing version for multiple extensions
std::vector<std::string>
findLogFiles(const std::vector<std::string>& extensions, size_t max_file_size_mb, size_t batch_size);
/**
 * @brief Validate that a file size is within processing limits
 * @param filename Path to file to check
 * @param max_size_mb Maximum allowed size in megabytes
 * @return true if file is within size limits, false otherwise
 *
 * Checks file size against specified limits to prevent processing of files
 * that could cause memory exhaustion or excessive processing time.
 *
 * @note Used during file discovery to filter out oversized files
 */
bool validateFileSize(const std::string& filename, size_t max_size_mb = DEFAULT_MAX_FILE_SIZE_MB);

/**
 * @brief Format byte count into human-readable size string
 * @param bytes Number of bytes to format
 * @return Formatted string with appropriate units (B, KB, MB, GB)
 *
 * Converts raw byte counts into human-readable format with appropriate
 * unit suffixes for display in progress reports and resource summaries.
 *
 * @example
 * formatMemorySize(1024) returns "1.00 KB"
 * formatMemorySize(1048576) returns "1.00 MB"
 */
std::string formatMemorySize(size_t bytes);

/**
 * @brief Print resource usage statistics from processing context
 * @param context Context containing resource managers
 * @param quiet Suppress output in quiet mode
 *
 * Displays detailed resource usage statistics including:
 * - Current and peak memory usage
 * - File handle utilization
 * - Thread pool efficiency
 * - Processing performance metrics
 *
 * @note Useful for performance tuning and resource optimization
 */
void printResourceUsage(const ProcessingContext& context, bool quiet = false);

/**
 * @brief Print job scheduler resource information
 * @param job_resources JobResources structure with scheduler information
 * @param quiet Suppress output in quiet mode
 *
 * Displays information about job scheduler resource allocation including:
 * - Detected scheduler type (SLURM, PBS, etc.)
 * - Allocated CPU count and memory limits
 * - Job ID and queue information
 * - Resource constraint warnings
 *
 * @note Helps users understand resource constraints in HPC environments
 */
void printJobResourceInfo(const JobResources& job_resources, bool quiet = false);

/** @} */  // end of UtilityFunctions group

/**
 * @defgroup ResourceCalculation Job-Aware Resource Calculation
 * @brief Functions for calculating safe resource usage limits in HPC environments
 * @{
 */

/**
 * @brief Calculate safe thread count considering job scheduler limits
 * @param requested_threads User-requested number of threads
 * @param file_count Number of files to process
 * @param job_resources Job scheduler resource allocation information
 * @return Safe thread count that respects all constraints
 *
 * Determines the optimal number of threads to use considering:
 * - User preferences (requested_threads)
 * - Job scheduler CPU allocation limits
 * - Number of files to process (no point using more threads than files)
 * - System CPU count and architecture
 *
 * @section Algorithm
 * 1. Start with user-requested thread count
 * 2. Apply job scheduler CPU limits if detected
 * 3. Limit to actual file count (efficiency)
 * 4. Apply system CPU count limits
 * 5. Ensure minimum of 1 thread
 *
 * @note Essential for proper resource usage in HPC cluster environments
 */
unsigned int
calculateSafeThreadCount(unsigned int requested_threads, unsigned int file_count, const JobResources& job_resources);

/**
 * @brief Calculate safe memory limit considering job scheduler constraints
 * @param requested_memory_mb User-requested memory limit (MB)
 * @param thread_count Number of threads that will be used
 * @param job_resources Job scheduler resource allocation information
 * @return Safe memory limit in megabytes
 *
 * Determines the safe memory usage limit considering:
 * - User preferences and configuration
 * - Job scheduler memory allocation limits
 * - Number of processing threads (memory per thread)
 * - System memory availability
 * - Safety margins for system stability
 *
 * @section Safety Margins
 * - Reserves memory for system processes
 * - Accounts for memory fragmentation
 * - Provides buffer for temporary allocations
 * - Prevents swap usage that degrades performance
 *
 * @note Critical for preventing out-of-memory conditions that can
 *       crash jobs or entire cluster nodes
 */
size_t
calculateSafeMemoryLimit(size_t requested_memory_mb, unsigned int thread_count, const JobResources& job_resources);

/** @} */  // end of ResourceCalculation group

/**
 * @defgroup SafeParsing Safe String Parsing Functions
 * @brief Robust string-to-numeric conversion functions with error handling
 *
 * These functions provide safe alternatives to standard library conversion
 * functions (std::stod, std::stoi, etc.) with proper error handling and
 * validation. They prevent crashes from malformed input data commonly
 * found in computational chemistry log files.
 *
 * @section Error Handling
 * - Invalid format strings return false without throwing exceptions
 * - Out-of-range values are detected and handled gracefully
 * - Partial conversions are rejected (strict validation)
 * - NaN and infinity values are handled appropriately
 *
 * @{
 */

/**
 * @brief Safely convert string to double with error handling
 * @param str String to convert
 * @param result Reference to store the converted value
 * @return true if conversion successful, false otherwise
 *
 * Converts string to double with comprehensive error checking:
 * - Validates string format before conversion
 * - Handles scientific notation (1.23e-4)
 * - Rejects partial conversions (strict mode)
 * - Manages NaN and infinity values appropriately
 *
 * @note Preferred over std::stod for parsing log file data where
 *       malformed entries are common
 */
bool safe_stod(const std::string& str, double& result);

/**
 * @brief Safely convert string to integer with error handling
 * @param str String to convert
 * @param result Reference to store the converted value
 * @return true if conversion successful, false otherwise
 *
 * Converts string to integer with comprehensive error checking:
 * - Validates string format and numeric content
 * - Checks for integer overflow/underflow
 * - Rejects floating-point strings (strict integer mode)
 * - Handles leading/trailing whitespace appropriately
 *
 * @note Essential for parsing iteration counts, job IDs, and other
 *       integer values from log files
 */
bool safe_stoi(const std::string& str, int& result);

/**
 * @brief Safely convert string to unsigned long with error handling
 * @param str String to convert
 * @param result Reference to store the converted value
 * @return true if conversion successful, false otherwise
 *
 * Converts string to unsigned long with comprehensive error checking:
 * - Validates string format and ensures non-negative values
 * - Checks for unsigned long overflow
 * - Rejects negative numbers (strict unsigned mode)
 * - Handles edge cases like empty strings and whitespace
 *
 * @note Used for parsing large positive integers like byte counts,
 *       memory sizes, and iteration indices
 */
bool safe_stoul(const std::string& str, unsigned long& result);

/** @} */  // end of SafeParsing group

#endif  // GAUSSIAN_EXTRACTOR_H
