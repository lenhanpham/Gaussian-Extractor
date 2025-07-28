/**
 * @file job_scheduler.h
 * @brief Job scheduler detection and resource management for HPC environments
 * @author Le Nhan Pham
 * @date 2025
 * 
 * This header provides comprehensive job scheduler detection and resource
 * management for High Performance Computing (HPC) environments. It automatically
 * detects the active job scheduler, extracts resource allocation information,
 * and provides safe resource usage calculations that respect cluster limits.
 * 
 * @section Supported Schedulers
 * The system supports detection and resource extraction for:
 * - SLURM (Simple Linux Utility for Resource Management)
 * - PBS/Torque (Portable Batch System)
 * - SGE/OGS (Sun Grid Engine / Open Grid Scheduler)
 * - LSF (IBM Load Sharing Facility)
 * - Generic cluster detection for unknown schedulers
 * 
 * @section Resource Management
 * Key features include:
 * - Automatic detection of allocated CPUs and memory
 * - Safe resource limit calculations to prevent oversubscription
 * - Integration with application threading and memory management
 * - Environment variable parsing for all major schedulers
 * - Validation and safety checks for resource requests
 * 
 * @section Integration
 * The job scheduler system integrates with:
 * - Multi-threading systems for safe CPU allocation
 * - Memory management systems for memory limit enforcement
 * - Configuration systems for default resource preferences
 * - Error reporting for resource allocation issues
 */

#ifndef JOB_SCHEDULER_H
#define JOB_SCHEDULER_H

#include <string>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <functional>

/**
 * @enum SchedulerType
 * @brief Enumeration of supported job scheduler types
 * 
 * This enumeration defines all job scheduler types that the system can
 * detect and extract resource information from. Each scheduler type has
 * specific environment variables and resource allocation patterns.
 */
enum class SchedulerType {
    NONE,           ///< Not running under any job scheduler (standalone execution)
    SLURM,          ///< SLURM (Simple Linux Utility for Resource Management)
    PBS,            ///< PBS/Torque (Portable Batch System)
    SGE,            ///< SGE (Sun Grid Engine) / Open Grid Scheduler
    LSF,            ///< LSF (IBM Load Sharing Facility)
    UNKNOWN_CLUSTER ///< Detected cluster environment but unknown scheduler type
};

/**
 * @struct JobResources
 * @brief Complete resource allocation information from job scheduler
 * 
 * This structure contains all resource allocation information extracted from
 * the job scheduler environment. It provides a unified interface for resource
 * information regardless of the underlying scheduler type.
 * 
 * @section Resource Information
 * The structure captures:
 * - CPU allocation (cores, tasks, nodes)
 * - Memory allocation (total and per-node limits)
 * - Job identification and queue information
 * - Resource limit flags for validation
 * 
 * @section Usage
 * This structure is populated by JobSchedulerDetector and used throughout
 * the application to make resource allocation decisions that respect
 * cluster limits and prevent resource conflicts.
 */
struct JobResources {
    SchedulerType scheduler_type = SchedulerType::NONE; ///< Type of detected job scheduler
    std::string job_id;                    ///< Unique job identifier from scheduler
    unsigned int allocated_cpus = 0;       ///< Number of CPU cores allocated to job
    size_t allocated_memory_mb = 0;        ///< Memory allocated to job in megabytes
    unsigned int nodes = 1;                ///< Number of compute nodes allocated
    unsigned int tasks_per_node = 0;       ///< Number of tasks per compute node
    bool has_cpu_limit = false;            ///< Whether CPU limit is explicitly specified
    bool has_memory_limit = false;         ///< Whether memory limit is explicitly specified
    std::string partition;                 ///< Partition/queue name where job is running
    std::string account;                   ///< Account/project name for resource billing
};

/**
 * @class JobSchedulerDetector
 * @brief Static utility class for job scheduler detection and resource management
 * 
 * The JobSchedulerDetector class provides a comprehensive interface for detecting
 * job schedulers, extracting resource allocation information, and calculating
 * safe resource usage limits in HPC environments. All methods are static as
 * scheduler detection is inherently a system-level operation.
 * 
 * @section Detection Strategy
 * The detector uses environment variable analysis to identify schedulers:
 * 1. Check for scheduler-specific environment variables
 * 2. Parse resource allocation information from environment
 * 3. Validate and normalize resource information
 * 4. Provide safe resource usage recommendations
 * 
 * @section Safety Philosophy
 * All resource calculations follow conservative safety principles:
 * - Never exceed allocated resources
 * - Provide safety margins for system stability
 * - Validate resource requests against limits
 * - Gracefully handle missing or invalid information
 */
class JobSchedulerDetector {
public:
    /**
     * @defgroup DetectionMethods Job Scheduler Detection Methods
     * @brief Methods for detecting and analyzing job scheduler environments
     * @{
     */
    
    /**
     * @brief Detect current job scheduler and extract complete resource information
     * @return JobResources structure with complete resource allocation details
     * 
     * This is the main entry point for job scheduler detection. It automatically
     * identifies the active scheduler type and extracts all available resource
     * allocation information into a unified JobResources structure.
     * 
     * @section Detection Process
     * 1. Identify scheduler type from environment variables
     * 2. Call scheduler-specific resource extraction methods
     * 3. Validate and normalize resource information
     * 4. Return complete resource structure
     */
    static JobResources detect_job_resources();
    
    /**
     * @brief Identify the type of job scheduler currently active
     * @return SchedulerType enumeration value
     * 
     * Determines which job scheduler is currently managing the execution
     * environment by analyzing scheduler-specific environment variables.
     */
    static SchedulerType get_scheduler_type();
    
    /**
     * @brief Extract job ID from scheduler environment
     * @param scheduler Type of scheduler to extract job ID for
     * @return Job ID string, empty if not available
     * 
     * Retrieves the unique job identifier assigned by the scheduler.
     * Useful for logging, debugging, and job management operations.
     */
    static std::string get_job_id(SchedulerType scheduler);
    
    /** @} */ // end of DetectionMethods group
    
    /**
     * @defgroup SchedulerSpecific Scheduler-Specific Resource Detection
     * @brief Methods for extracting resources from specific scheduler types
     * @{
     */
    
    /**
     * @brief Extract resource information from SLURM environment
     * @return JobResources with SLURM-specific resource allocation
     * 
     * Parses SLURM environment variables to extract CPU allocation,
     * memory limits, node information, and job metadata.
     */
    static JobResources detect_slurm_resources();
    
    /**
     * @brief Extract resource information from PBS/Torque environment
     * @return JobResources with PBS-specific resource allocation
     * 
     * Parses PBS/Torque environment variables to extract resource
     * allocation information including CPU cores, memory, and nodes.
     */
    static JobResources detect_pbs_resources();
    
    /**
     * @brief Extract resource information from SGE/OGS environment
     * @return JobResources with SGE-specific resource allocation
     * 
     * Parses Sun Grid Engine / Open Grid Scheduler environment
     * variables for resource allocation and job information.
     */
    static JobResources detect_sge_resources();
    
    /**
     * @brief Extract resource information from LSF environment
     * @return JobResources with LSF-specific resource allocation
     * 
     * Parses IBM Load Sharing Facility environment variables
     * for resource allocation and job management information.
     */
    static JobResources detect_lsf_resources();
    
    /** @} */ // end of SchedulerSpecific group
    
    /**
     * @defgroup UtilityMethods Utility and Helper Methods
     * @brief General utility methods for scheduler operations
     * @{
     */
    
    /**
     * @brief Convert scheduler type to human-readable name
     * @param type SchedulerType to convert
     * @return Human-readable scheduler name string
     */
    static std::string scheduler_name(SchedulerType type);
    
    /**
     * @brief Check if currently running under a job scheduler
     * @return true if running in a scheduled job, false if standalone
     * 
     * Determines whether the application is running under job scheduler
     * control or as a standalone process. Useful for adapting behavior
     * based on execution environment.
     */
    static bool is_running_in_job();
    
    /** @} */ // end of UtilityMethods group
    
    /**
     * @defgroup SafetyMethods Resource Safety and Validation Methods
     * @brief Methods for calculating safe resource usage limits
     * @{
     */
    
    /**
     * @brief Calculate safe CPU count respecting job scheduler limits
     * @param requested User-requested number of CPUs
     * @param job_resources Job scheduler resource allocation information
     * @return Safe CPU count that respects all limits
     * 
     * Determines the maximum safe number of CPUs to use considering:
     * - User preferences (requested count)
     * - Job scheduler CPU allocation limits
     * - System CPU availability
     * - Safety margins for system stability
     */
    static unsigned int get_safe_cpu_count(unsigned int requested, const JobResources& job_resources);
    
    /**
     * @brief Calculate safe memory limit respecting job scheduler constraints
     * @param requested User-requested memory limit in MB
     * @param job_resources Job scheduler resource allocation information
     * @return Safe memory limit in MB that respects all constraints
     * 
     * Determines the maximum safe memory usage considering:
     * - User preferences and configuration
     * - Job scheduler memory allocation limits
     * - System memory availability
     * - Safety margins for system processes
     */
    static size_t get_safe_memory_limit_mb(size_t requested, const JobResources& job_resources);
    
    /** @} */ // end of SafetyMethods group
    
    /**
     * @defgroup EnvironmentHelpers Environment Variable Helper Methods
     * @brief Methods for parsing scheduler environment variables
     * @{
     */
    
    /**
     * @brief Get environment variable value with default fallback
     * @param name Environment variable name
     * @param default_value Default value if variable not found
     * @return Environment variable value or default
     */
    static std::string get_env_var(const char* name, const std::string& default_value = "");
    
    /**
     * @brief Get boolean environment variable value
     * @param name Environment variable name
     * @param default_value Default value if variable not found
     * @return Boolean interpretation of environment variable
     * 
     * Supports various boolean representations: true/false, yes/no, 1/0, on/off
     */
    static bool get_env_bool(const char* name, bool default_value = false);
    
    /**
     * @brief Get long integer environment variable value
     * @param name Environment variable name
     * @param default_value Default value if variable not found or invalid
     * @return Long integer value from environment variable
     */
    static long get_env_long(const char* name, long default_value = 0);
    
    /**
     * @brief Parse memory specification string to megabytes
     * @param memory_str Memory specification (e.g., "4GB", "2048MB", "1T")
     * @return Memory amount in megabytes
     * 
     * Parses various memory format specifications used by different
     * schedulers and converts them to a consistent megabyte representation.
     */
    static size_t parse_memory_string(const std::string& memory_str);
    
    /** @} */ // end of EnvironmentHelpers group
    
private:
    /**
     * @defgroup PrivateParsers Private Parsing Helper Methods
     * @brief Internal methods for parsing scheduler-specific formats
     * @{
     */
    
    /**
     * @brief Parse SLURM-specific memory format strings
     * @param mem_str SLURM memory specification string
     * @return Memory amount in megabytes
     * 
     * Handles SLURM-specific memory formats including per-CPU and
     * per-node memory specifications with various unit suffixes.
     */
    static size_t parse_slurm_memory(const std::string& mem_str);
    
    /**
     * @brief Parse PBS/Torque-specific memory format strings
     * @param mem_str PBS memory specification string
     * @return Memory amount in megabytes
     * 
     * Handles PBS-specific memory formats and unit conventions.
     */
    static size_t parse_pbs_memory(const std::string& mem_str);
    
    /**
     * @brief Parse general memory format strings
     * @param mem_str Generic memory specification string
     * @return Memory amount in megabytes
     * 
     * Handles common memory format patterns used across multiple
     * schedulers and provides fallback parsing for unknown formats.
     */
    static size_t parse_general_memory(const std::string& mem_str);
    
    /**
     * @brief Parse CPU list specification to count
     * @param cpu_str CPU list specification (e.g., "0,1,4-7")
     * @return Number of CPUs in the specification
     * 
     * Parses CPU list formats used by schedulers to specify
     * allocated CPU cores, handling ranges and individual cores.
     */
    static unsigned int parse_cpu_list(const std::string& cpu_str);
    
    /**
     * @brief Parse CPU range specification to count
     * @param range_str CPU range specification (e.g., "4-7")
     * @return Number of CPUs in the range
     * 
     * Parses individual CPU range specifications as part of
     * larger CPU list processing.
     */
    static unsigned int parse_cpu_range(const std::string& range_str);
    
    /** @} */ // end of PrivateParsers group
};

/**
 * @defgroup InlineImplementations Inline Method Implementations
 * @brief Inline implementations of simple JobSchedulerDetector methods
 * 
 * These methods are implemented inline for performance reasons as they
 * perform simple operations that benefit from inlining optimization.
 * @{
 */

/**
 * @brief Inline implementation of environment variable getter
 * @param name Environment variable name to retrieve
 * @param default_value Value to return if variable not found
 * @return Environment variable value or default
 * 
 * Provides efficient access to environment variables with automatic
 * fallback to default values for missing variables.
 */
inline std::string JobSchedulerDetector::get_env_var(const char* name, const std::string& default_value) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : default_value;
}

inline bool JobSchedulerDetector::get_env_bool(const char* name, bool default_value) {
    const char* value = std::getenv(name);
    if (!value) return default_value;
    std::string val(value);
    std::transform(val.begin(), val.end(), val.begin(), ::tolower);
    return val == "true" || val == "1" || val == "yes" || val == "on";
}

inline long JobSchedulerDetector::get_env_long(const char* name, long default_value) {
    const char* value = std::getenv(name);
    if (!value) return default_value;
    try {
        return std::stol(value);
    } catch (...) {
        return default_value;
    }
}

inline std::string JobSchedulerDetector::scheduler_name(SchedulerType type) {
    switch (type) {
        case SchedulerType::SLURM: return "SLURM";
        case SchedulerType::PBS: return "PBS/Torque";
        case SchedulerType::SGE: return "SGE/OGS";
        case SchedulerType::LSF: return "LSF";
        case SchedulerType::UNKNOWN_CLUSTER: return "Unknown Cluster";
        case SchedulerType::NONE: return "None";
        default: return "Unknown";
    }
}

/**
 * @brief Inline implementation of job execution environment check
 * @return true if running under job scheduler, false if standalone
 * 
 * Efficiently determines execution environment by checking if any
 * job scheduler has been detected.
 */
inline bool JobSchedulerDetector::is_running_in_job() {
    return get_scheduler_type() != SchedulerType::NONE;
}

/** @} */ // end of InlineImplementations group

#endif // JOB_SCHEDULER_H