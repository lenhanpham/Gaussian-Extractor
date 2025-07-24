#ifndef JOB_SCHEDULER_H
#define JOB_SCHEDULER_H

#include <string>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <functional>

// Job scheduler types
enum class SchedulerType {
    NONE,           // Not running under a job scheduler
    SLURM,          // SLURM (Simple Linux Utility for Resource Management)
    PBS,            // PBS/Torque (Portable Batch System)
    SGE,            // SGE (Sun Grid Engine) / Open Grid Scheduler
    LSF,            // LSF (IBM Load Sharing Facility)
    UNKNOWN_CLUSTER // Detected cluster but unknown type
};

// Resource allocation information from job scheduler
struct JobResources {
    SchedulerType scheduler_type = SchedulerType::NONE;
    std::string job_id;
    unsigned int allocated_cpus = 0;        // Number of CPUs allocated to job
    size_t allocated_memory_mb = 0;         // Memory allocated to job in MB
    unsigned int nodes = 1;                 // Number of nodes allocated
    unsigned int tasks_per_node = 0;        // Tasks per node
    bool has_cpu_limit = false;             // Whether CPU limit is specified
    bool has_memory_limit = false;          // Whether memory limit is specified
    std::string partition;                  // Partition/queue name
    std::string account;                    // Account/project name
};

class JobSchedulerDetector {
public:
    // Detect current job scheduler and resource allocation
    static JobResources detect_job_resources();
    
    // Get scheduler type
    static SchedulerType get_scheduler_type();
    
    // Get job ID for current scheduler
    static std::string get_job_id(SchedulerType scheduler);
    
    // Scheduler-specific resource detection
    static JobResources detect_slurm_resources();
    static JobResources detect_pbs_resources();
    static JobResources detect_sge_resources();
    static JobResources detect_lsf_resources();
    
    // Utility functions
    static std::string scheduler_name(SchedulerType type);
    static bool is_running_in_job();
    
    // Resource validation and safety checks
    static unsigned int get_safe_cpu_count(unsigned int requested, const JobResources& job_resources);
    static size_t get_safe_memory_limit_mb(size_t requested, const JobResources& job_resources);
    
    // Environment variable helpers
    static std::string get_env_var(const char* name, const std::string& default_value = "");
    static bool get_env_bool(const char* name, bool default_value = false);
    static long get_env_long(const char* name, long default_value = 0);
    static size_t parse_memory_string(const std::string& memory_str);
    
private:
    // Helper functions for parsing different memory formats
    static size_t parse_slurm_memory(const std::string& mem_str);
    static size_t parse_pbs_memory(const std::string& mem_str);
    static size_t parse_general_memory(const std::string& mem_str);
    
    // CPU parsing helpers
    static unsigned int parse_cpu_list(const std::string& cpu_str);
    static unsigned int parse_cpu_range(const std::string& range_str);
};

// Inline implementations for simple functions
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

inline bool JobSchedulerDetector::is_running_in_job() {
    return get_scheduler_type() != SchedulerType::NONE;
}

#endif // JOB_SCHEDULER_H