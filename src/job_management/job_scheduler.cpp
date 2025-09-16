#include "job_scheduler.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <algorithm>

// Main detection function
JobResources JobSchedulerDetector::detect_job_resources() {
    SchedulerType scheduler = get_scheduler_type();
    
    switch (scheduler) {
        case SchedulerType::SLURM:
            return detect_slurm_resources();
        case SchedulerType::PBS:
            return detect_pbs_resources();
        case SchedulerType::SGE:
            return detect_sge_resources();
        case SchedulerType::LSF:
            return detect_lsf_resources();
        default:
            JobResources resources;
            resources.scheduler_type = scheduler;
            return resources;
    }
}

SchedulerType JobSchedulerDetector::get_scheduler_type() {
    // Check SLURM
    if (!get_env_var("SLURM_JOB_ID").empty()) {
        return SchedulerType::SLURM;
    }
    
    // Check PBS/Torque
    if (!get_env_var("PBS_JOBID").empty() || !get_env_var("PBS_JOB_ID").empty()) {
        return SchedulerType::PBS;
    }
    
    // Check SGE/OGS
    if (!get_env_var("JOB_ID").empty() || !get_env_var("SGE_JOB_ID").empty()) {
        return SchedulerType::SGE;
    }
    
    // Check LSF
    if (!get_env_var("LSB_JOBID").empty() || !get_env_var("LSF_JOB_ID").empty()) {
        return SchedulerType::LSF;
    }
    
    // Check for other cluster indicators
    if (!get_env_var("BATCH_JOB_ID").empty() || 
        !get_env_var("QUEUE").empty() ||
        !get_env_var("CLUSTER_NAME").empty()) {
        return SchedulerType::UNKNOWN_CLUSTER;
    }
    
    return SchedulerType::NONE;
}

std::string JobSchedulerDetector::get_job_id(SchedulerType scheduler) {
    switch (scheduler) {
        case SchedulerType::SLURM:
            return get_env_var("SLURM_JOB_ID");
        case SchedulerType::PBS:
            {
                std::string id = get_env_var("PBS_JOBID");
                if (id.empty()) id = get_env_var("PBS_JOB_ID");
                return id;
            }
        case SchedulerType::SGE:
            {
                std::string id = get_env_var("JOB_ID");
                if (id.empty()) id = get_env_var("SGE_JOB_ID");
                return id;
            }
        case SchedulerType::LSF:
            {
                std::string id = get_env_var("LSB_JOBID");
                if (id.empty()) id = get_env_var("LSF_JOB_ID");
                return id;
            }
        default:
            return "";
    }
}

JobResources JobSchedulerDetector::detect_slurm_resources() {
    JobResources resources;
    resources.scheduler_type = SchedulerType::SLURM;
    resources.job_id = get_env_var("SLURM_JOB_ID");
    
    // CPU detection
    long cpus_per_task = get_env_long("SLURM_CPUS_PER_TASK", 0);
    long ntasks = get_env_long("SLURM_NTASKS", 1);
    long ntasks_per_node = get_env_long("SLURM_NTASKS_PER_NODE", 0);
    
    if (cpus_per_task > 0) {
        resources.allocated_cpus = static_cast<unsigned int>(cpus_per_task * ntasks);
        resources.has_cpu_limit = true;
    } else {
        // Try SLURM_JOB_CPUS_PER_NODE
        std::string cpus_per_node = get_env_var("SLURM_JOB_CPUS_PER_NODE");
        if (!cpus_per_node.empty()) {
            resources.allocated_cpus = parse_cpu_list(cpus_per_node);
            resources.has_cpu_limit = true;
        }
    }
    
    // Memory detection
    std::string mem_per_node = get_env_var("SLURM_MEM_PER_NODE");
    std::string mem_per_cpu = get_env_var("SLURM_MEM_PER_CPU");
    
    if (!mem_per_node.empty()) {
        resources.allocated_memory_mb = parse_slurm_memory(mem_per_node);
        resources.has_memory_limit = true;
    } else if (!mem_per_cpu.empty()) {
        size_t mem_per_cpu_mb = parse_slurm_memory(mem_per_cpu);
        if (resources.allocated_cpus > 0) {
            resources.allocated_memory_mb = mem_per_cpu_mb * resources.allocated_cpus;
        } else {
            resources.allocated_memory_mb = mem_per_cpu_mb * ntasks;
        }
        resources.has_memory_limit = true;
    }
    
    // Node information
    resources.nodes = static_cast<unsigned int>(get_env_long("SLURM_JOB_NUM_NODES", 1));
    if (ntasks_per_node > 0) {
        resources.tasks_per_node = static_cast<unsigned int>(ntasks_per_node);
    }
    
    // Additional info
    resources.partition = get_env_var("SLURM_JOB_PARTITION");
    resources.account = get_env_var("SLURM_JOB_ACCOUNT");
    
    return resources;
}

JobResources JobSchedulerDetector::detect_pbs_resources() {
    JobResources resources;
    resources.scheduler_type = SchedulerType::PBS;
    resources.job_id = get_job_id(SchedulerType::PBS);
    
    // CPU detection
    long ncpus = get_env_long("PBS_NUM_PPN", 0);  // Processors per node
    if (ncpus == 0) ncpus = get_env_long("PBS_NCPUS", 0);
    if (ncpus == 0) ncpus = get_env_long("NCPUS", 0);
    
    if (ncpus > 0) {
        resources.allocated_cpus = static_cast<unsigned int>(ncpus);
        resources.has_cpu_limit = true;
    }
    
    // Try to parse PBS_RESOURCE_LIST
    std::string resource_list = get_env_var("PBS_RESOURCE_LIST");
    if (!resource_list.empty()) {
        std::regex ncpus_regex(R"(ncpus=(\d+))");
        std::regex mem_regex(R"(mem=([0-9]+(?:\.[0-9]+)?[kmgtKMGT]?[bB]?))");
        std::smatch match;
        
        if (std::regex_search(resource_list, match, ncpus_regex)) {
            resources.allocated_cpus = static_cast<unsigned int>(std::stoul(match[1]));
            resources.has_cpu_limit = true;
        }
        
        if (std::regex_search(resource_list, match, mem_regex)) {
            resources.allocated_memory_mb = parse_pbs_memory(match[1]);
            resources.has_memory_limit = true;
        }
    }
    
    // Memory detection from environment
    std::string mem = get_env_var("PBS_RESOURCE_MEM");
    if (mem.empty()) mem = get_env_var("PBS_MEM");
    if (!mem.empty()) {
        resources.allocated_memory_mb = parse_pbs_memory(mem);
        resources.has_memory_limit = true;
    }
    
    // Node information
    resources.nodes = static_cast<unsigned int>(get_env_long("PBS_NUM_NODES", 1));
    resources.partition = get_env_var("PBS_QUEUE");
    resources.account = get_env_var("PBS_ACCOUNT");
    
    return resources;
}

JobResources JobSchedulerDetector::detect_sge_resources() {
    JobResources resources;
    resources.scheduler_type = SchedulerType::SGE;
    resources.job_id = get_job_id(SchedulerType::SGE);
    
    // CPU detection
    long nslots = get_env_long("NSLOTS", 0);
    if (nslots == 0) nslots = get_env_long("SGE_NSLOTS", 0);
    
    if (nslots > 0) {
        resources.allocated_cpus = static_cast<unsigned int>(nslots);
        resources.has_cpu_limit = true;
    }
    
    // Memory detection - SGE uses different formats
    std::string mem = get_env_var("SGE_MEM");
    if (mem.empty()) mem = get_env_var("MEMORY");
    if (!mem.empty()) {
        resources.allocated_memory_mb = parse_general_memory(mem);
        resources.has_memory_limit = true;
    }
    
    // PE (Parallel Environment) information
    std::string pe = get_env_var("PE");
    if (!pe.empty()) {
        resources.partition = pe;
    }
    
    resources.partition = get_env_var("QUEUE");
    resources.account = get_env_var("SGE_ACCOUNT");
    
    return resources;
}

JobResources JobSchedulerDetector::detect_lsf_resources() {
    JobResources resources;
    resources.scheduler_type = SchedulerType::LSF;
    resources.job_id = get_job_id(SchedulerType::LSF);
    
    // CPU detection
    long max_num_processors = get_env_long("LSB_MAX_NUM_PROCESSORS", 0);
    if (max_num_processors > 0) {
        resources.allocated_cpus = static_cast<unsigned int>(max_num_processors);
        resources.has_cpu_limit = true;
    }
    
    // Memory detection
    std::string mem = get_env_var("LSB_MEM");
    if (!mem.empty()) {
        resources.allocated_memory_mb = parse_general_memory(mem);
        resources.has_memory_limit = true;
    }
    
    // Additional LSF information
    resources.partition = get_env_var("LSB_QUEUE");
    resources.account = get_env_var("LSB_PROJECT_NAME");
    
    return resources;
}

unsigned int JobSchedulerDetector::get_safe_cpu_count(unsigned int requested, const JobResources& job_resources) {
    if (!job_resources.has_cpu_limit || job_resources.allocated_cpus == 0) {
        return requested; // No job limit, use requested amount
    }
    
    // Respect job allocation limit
    unsigned int job_limit = job_resources.allocated_cpus;
    
    if (requested > job_limit) {
        std::cerr << "Warning: Requested " << requested << " threads, but job allocation is only " 
                  << job_limit << " CPUs. Using " << job_limit << " threads." << std::endl;
        return job_limit;
    }
    
    return requested;
}

size_t JobSchedulerDetector::get_safe_memory_limit_mb(size_t requested, const JobResources& job_resources) {
    if (!job_resources.has_memory_limit || job_resources.allocated_memory_mb == 0) {
        return requested; // No job limit, use requested amount
    }
    
    // Respect job allocation limit
    size_t job_limit = job_resources.allocated_memory_mb;
    
    // Leave some headroom for system overhead (5% or 512MB, whichever is smaller)
    size_t overhead = std::min(job_limit / 20, static_cast<size_t>(512));
    size_t safe_limit = job_limit > overhead ? job_limit - overhead : job_limit;
    
    if (requested > safe_limit) {
        std::cerr << "Warning: Requested " << requested << " MB memory, but job allocation is only " 
                  << job_limit << " MB. Using " << safe_limit << " MB (with overhead reserved)." << std::endl;
        return safe_limit;
    }
    
    return requested;
}

size_t JobSchedulerDetector::parse_memory_string(const std::string& memory_str) {
    if (memory_str.empty()) return 0;
    
    // Try scheduler-specific parsers first
    size_t result = parse_slurm_memory(memory_str);
    if (result > 0) return result;
    
    result = parse_pbs_memory(memory_str);
    if (result > 0) return result;
    
    return parse_general_memory(memory_str);
}

size_t JobSchedulerDetector::parse_slurm_memory(const std::string& mem_str) {
    if (mem_str.empty()) return 0;
    
    std::string str = mem_str;
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    
    // Extract number
    std::regex mem_regex(R"((\d+(?:\.\d+)?)([kmgtKMGT]?))");
    std::smatch match;
    
    if (!std::regex_search(str, match, mem_regex)) {
        return 0;
    }
    
    double value = std::stod(match[1]);
    std::string unit = match[2];
    
    // SLURM default unit is MB if no suffix
    size_t multiplier = 1;
    if (unit.empty()) {
        multiplier = 1; // MB
    } else if (unit == "k") {
        multiplier = 1; // KB -> MB (divide by 1024, but we'll convert later)
        value /= 1024.0;
    } else if (unit == "m") {
        multiplier = 1; // MB
    } else if (unit == "g") {
        multiplier = 1024; // GB -> MB
    } else if (unit == "t") {
        multiplier = 1024 * 1024; // TB -> MB
    }
    
    return static_cast<size_t>(value * multiplier);
}

size_t JobSchedulerDetector::parse_pbs_memory(const std::string& mem_str) {
    if (mem_str.empty()) return 0;
    
    std::string str = mem_str;
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    
    std::regex mem_regex(R"((\d+(?:\.\d+)?)([kmgtKMGT]?)[bB]?)");
    std::smatch match;
    
    if (!std::regex_search(str, match, mem_regex)) {
        return 0;
    }
    
    double value = std::stod(match[1]);
    std::string unit = match[2];
    
    // PBS commonly uses bytes as default
    size_t bytes = static_cast<size_t>(value);
    
    if (unit == "k") {
        bytes *= 1024;
    } else if (unit == "m") {
        bytes *= 1024 * 1024;
    } else if (unit == "g") {
        bytes *= 1024 * 1024 * 1024;
    } else if (unit == "t") {
        bytes *= static_cast<size_t>(1024) * 1024 * 1024 * 1024;
    }
    
    return bytes / (1024 * 1024); // Convert to MB
}

size_t JobSchedulerDetector::parse_general_memory(const std::string& mem_str) {
    if (mem_str.empty()) return 0;
    
    std::string str = mem_str;
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    
    // Remove any spaces
    str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
    
    std::regex mem_regex(R"((\d+(?:\.\d+)?)([kmgtKMGT]?)[bB]?)");
    std::smatch match;
    
    if (!std::regex_search(str, match, mem_regex)) {
        // Try just a number (assume MB)
        try {
            return static_cast<size_t>(std::stod(str));
        } catch (...) {
            return 0;
        }
    }
    
    double value = std::stod(match[1]);
    std::string unit = match[2];
    
    // Assume MB if no unit specified
    if (unit.empty()) {
        return static_cast<size_t>(value);
    }
    
    if (unit == "k") {
        return static_cast<size_t>(value * 1024 / 1024); // KB to MB
    } else if (unit == "m") {
        return static_cast<size_t>(value);
    } else if (unit == "g") {
        return static_cast<size_t>(value * 1024);
    } else if (unit == "t") {
        return static_cast<size_t>(value * 1024 * 1024);
    }
    
    return static_cast<size_t>(value);
}

unsigned int JobSchedulerDetector::parse_cpu_list(const std::string& cpu_str) {
    if (cpu_str.empty()) return 0;
    
    // Handle formats like "4" or "2,2" or "1-4" or "(x4)" etc.
    std::string str = cpu_str;
    
    // Remove parentheses and 'x' prefix
    std::regex cleanup_regex(R"([()x])");
    str = std::regex_replace(str, cleanup_regex, "");
    
    unsigned int total_cpus = 0;
    std::istringstream iss(str);
    std::string token;
    
    while (std::getline(iss, token, ',')) {
        if (token.find('-') != std::string::npos) {
            total_cpus += parse_cpu_range(token);
        } else {
            try {
                total_cpus += static_cast<unsigned int>(std::stoul(token));
            } catch (...) {
                // Skip invalid tokens
            }
        }
    }
    
    return total_cpus;
}

unsigned int JobSchedulerDetector::parse_cpu_range(const std::string& range_str) {
    size_t dash_pos = range_str.find('-');
    if (dash_pos == std::string::npos) {
        try {
            return static_cast<unsigned int>(std::stoul(range_str));
        } catch (...) {
            return 0;
        }
    }
    
    try {
        unsigned int start = static_cast<unsigned int>(std::stoul(range_str.substr(0, dash_pos)));
        unsigned int end = static_cast<unsigned int>(std::stoul(range_str.substr(dash_pos + 1)));
        return (end >= start) ? (end - start + 1) : 0;
    } catch (...) {
        return 0;
    }
}