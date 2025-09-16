#include "high_level_energy.h"
#include "extraction/gaussian_extractor.h"
#include "utilities/metadata.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <regex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

// System-specific headers for memory detection
#ifdef _WIN32
    #include <windows.h>
// Note: GlobalMemoryStatusEx is available through windows.h (kernel32.dll)
// psapi.h is not needed for this function
#else
    #include <sys/sysinfo.h>
    #include <unistd.h>
#endif

// =============================================================================
// ThreadPool Implementation
// =============================================================================

/**
 * @brief ThreadPool constructor implementation
 */
ThreadPool::ThreadPool(size_t num_threads)
{
    for (size_t i = 0; i < num_threads; ++i)
    {
        workers_.emplace_back([this] {
            for (;;)
            {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    condition_.wait(lock, [this] {
                        return stop_flag_ || !tasks_.empty();
                    });

                    if (stop_flag_ && tasks_.empty())
                    {
                        return;
                    }

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                active_tasks_.fetch_add(1);
                try
                {
                    task();
                }
                catch (...)
                {
                    // Task exceptions are handled by the future mechanism
                }
                active_tasks_.fetch_sub(1);
            }
        });
    }
}

/**
 * @brief ThreadPool destructor implementation
 */
ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_flag_.store(true);
    }
    condition_.notify_all();

    for (std::thread& worker : workers_)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

// Template method implementation moved to header file for proper instantiation

/**
 * @brief Wait for all tasks to complete implementation
 */
void ThreadPool::wait_for_completion()
{
    while (active_tasks_.load() > 0 || !tasks_.empty())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// =============================================================================
// Pre-compiled Regex Patterns for Performance
// =============================================================================

struct RegexPatterns
{
    std::regex scf_done{"SCF Done"};
    std::regex cis_energy{"Total Energy, E\\(CIS"};
    std::regex pcm_correction{"After PCM corrections, the energy is"};
    std::regex clr_correction{"Total energy after correction"};
    std::regex zero_point{"Zero-point correction"};
    std::regex thermal_enthalpy{"Thermal correction to Enthalpy"};
    std::regex thermal_gibbs{"Thermal correction to Gibbs Free Energy"};
    std::regex thermal_energy{"Thermal correction to Energy"};
    std::regex entropy_total{"Total\\s+S"};
    std::regex temperature_pattern{"Kelvin\\.\\s+Pressure"};
    std::regex frequencies_pattern{"Frequencies"};
    std::regex normal_termination{"Normal"};
    std::regex error_pattern{"Error"};
    std::regex error_on_pattern{"Error on"};
    std::regex scrf_pattern{"scrf"};

    static RegexPatterns& get_instance()
    {
        static RegexPatterns instance;
        return instance;
    }

private:
    RegexPatterns() = default;
};

// =============================================================================
// File Content Cache for Reducing I/O
// =============================================================================

class FileContentCache
{
private:
    mutable std::unordered_map<std::string, std::string> cache_;
    mutable std::mutex                                   cache_mutex_;
    size_t                                               max_cache_size_mb_;
    size_t                                               current_size_bytes_;

public:
    FileContentCache(size_t max_cache_mb = 500) : max_cache_size_mb_(max_cache_mb), current_size_bytes_(0) {}

    std::string get_or_read(const std::string& filename)
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);

        auto it = cache_.find(filename);
        if (it != cache_.end())
        {
            return it->second;
        }

        // Read file
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return "";
        }

        auto file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        // Check cache size limit
        if (current_size_bytes_ + file_size > max_cache_size_mb_ * 1024 * 1024)
        {
            // Cache is full, don't cache this file
            std::string content(file_size, '\0');
            file.read(&content[0], file_size);
            return content;
        }

        std::string content(file_size, '\0');
        file.read(&content[0], file_size);

        // Cache the content
        cache_[filename] = content;
        current_size_bytes_ += content.size();

        return content;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_.clear();
        current_size_bytes_ = 0;
    }
};

// Global file cache instance
static FileContentCache g_file_cache(500);  // 500MB cache

// =============================================================================
// HighLevelEnergyCalculator Implementation
// =============================================================================

// Delegate header printing to Metadata module
static void print_header_box(std::ostream& out)
{
    out << Metadata::header();
}

// Basic constructor
HighLevelEnergyCalculator::HighLevelEnergyCalculator(double temp,
                                                     double concentration_m,
                                                     int    sort_column,
                                                     bool   is_au_format)
    : temperature_(temp), concentration_m_(concentration_m), sort_column_(sort_column), is_au_format_(is_au_format),
      context_(nullptr), has_context_(false), thread_pool_(nullptr)
{
    concentration_mol_m3_ = concentration_m * 1000.0;
}

// Enhanced constructor with processing context
HighLevelEnergyCalculator::HighLevelEnergyCalculator(std::shared_ptr<ProcessingContext> context,
                                                     double                             temp,
                                                     double                             concentration_m,
                                                     int                                sort_column,
                                                     bool                               is_au_format)
    : temperature_(temp), concentration_m_(concentration_m), sort_column_(sort_column), is_au_format_(is_au_format),
      context_(context), has_context_(true), thread_pool_(nullptr)
{
    concentration_mol_m3_ = concentration_m * 1000.0;

    if (context_)
    {
        // Validate the processing context
        if (!validate_processing_context())
        {
            context_->error_collector->add_warning("Processing context validation failed, falling back to basic mode");
            has_context_ = false;
        }
    }
    else
    {
        has_context_ = false;
    }
}

// Main calculation function
HighLevelEnergyData HighLevelEnergyCalculator::calculate_high_level_energy(const std::string& high_level_file)
{
    HighLevelEnergyData data(high_level_file);

    try
    {
        // Memory guard for large calculations
        struct MemoryGuard
        {
            std::shared_ptr<MemoryMonitor> monitor;
            size_t                         bytes;
            MemoryGuard(std::shared_ptr<MemoryMonitor> m, size_t b) : monitor(m), bytes(b)
            {
                if (monitor)
                    monitor->add_usage(bytes);
            }
            ~MemoryGuard()
            {
                if (monitor)
                    monitor->remove_usage(bytes);
            }
        };

        std::unique_ptr<MemoryGuard> memory_guard;
        if (has_context_ && context_->memory_monitor)
        {
            if (!context_->memory_monitor->can_allocate(10 * 1024 * 1024))
            {  // 10MB safety check
                throw std::runtime_error("Insufficient memory to process " + high_level_file);
            }
            memory_guard = std::make_unique<MemoryGuard>(context_->memory_monitor, 10 * 1024 * 1024);
        }

        // Extract high-level electronic energies from current directory file
        data.scf_high    = extract_value_from_file(high_level_file, "SCF Done", 5, -1);
        data.scf_td_high = extract_value_from_file(high_level_file, "Total Energy, E\\(CIS", 5, -1, false);
        data.scf_equi_high =
            extract_value_from_file(high_level_file, "After PCM corrections, the energy is", 7, -1, false);
        data.scf_clr_high = extract_value_from_file(high_level_file, "Total energy after correction", 6, -1, false);

        // Determine final high-level electronic energy (priority order from bash script)
        if (data.scf_equi_high != 0.0)
        {
            data.final_scf_high = data.scf_equi_high;
        }
        else if (data.scf_clr_high != 0.0)
        {
            data.final_scf_high = data.scf_clr_high;
        }
        else if (data.scf_td_high != 0.0)
        {
            data.final_scf_high = data.scf_td_high;
        }
        else
        {
            data.final_scf_high = data.scf_high;
        }

        // Extract low-level thermal data from parent directory
        std::string parent_file = get_parent_file(high_level_file);
        if (!extract_low_level_thermal_data(parent_file, data))
        {
            throw std::runtime_error("Failed to extract thermal data from " + parent_file);
        }

        // Calculate derived quantities
        data.tc_only  = calculate_thermal_only(data.tc_energy, data.zpe);
        data.ts_value = data.tc_enthalpy - data.tc_gibbs;

        // Calculate final thermodynamic quantities
        data.enthalpy_hartree = data.final_scf_high + data.tc_enthalpy;
        data.gibbs_hartree    = data.final_scf_high + data.tc_gibbs;

        // Check for SCRF and apply phase correction
        std::string file_content = has_context_ ? safe_read_file(high_level_file) : read_file_content(high_level_file);
        data.has_scrf            = (file_content.find("scrf") != std::string::npos);

        if (data.has_scrf)
        {
            data.phase_correction        = calculate_phase_correction(data.temperature, concentration_mol_m3_);
            data.gibbs_hartree_corrected = data.gibbs_hartree + data.phase_correction;
            data.phase_corr_applied      = true;
        }
        else
        {
            data.gibbs_hartree_corrected = data.gibbs_hartree;
            data.phase_corr_applied      = false;
        }

        // Convert to other units
        data.gibbs_kj_mol = data.gibbs_hartree_corrected * HARTREE_TO_KJ_MOL;
        data.gibbs_ev     = data.gibbs_hartree_corrected * HARTREE_TO_EV;

        // Extract additional data
        data.lowest_frequency = extract_lowest_frequency(parent_file);
        data.status           = determine_job_status(high_level_file);

        // Validate final data
        if (has_context_ && !HighLevelEnergyUtils::validate_energy_data(data))
        {
            if (context_->error_collector)
            {
                context_->error_collector->add_warning("Energy data validation failed for " + high_level_file);
            }
        }
    }
    catch (const std::exception& e)
    {
        if (has_context_ && context_->error_collector)
        {
            context_->error_collector->add_error("Error processing " + high_level_file + ": " + e.what());
        }
        else
        {
            std::cerr << "Error processing " << high_level_file << ": " << e.what() << std::endl;
        }
        data.status = "ERROR";
    }

    return data;
}

// Process entire directory
std::vector<HighLevelEnergyData> HighLevelEnergyCalculator::process_directory(const std::string& extension)
{
    std::vector<HighLevelEnergyData> results;

    try
    {
        // Use extension and limits from shared ProcessingContext when available
        const std::string& effective_extension  = has_context_ ? context_->extension : extension;
        const size_t effective_max_file_size_mb = has_context_ ? context_->max_file_size_mb : DEFAULT_MAX_FILE_SIZE_MB;

        std::vector<std::string> log_files;
        // If using default extension (.log), search for both .log and .out files (case-insensitive)
        bool is_log_ext = (effective_extension.length() == 4 && std::tolower(effective_extension[1]) == 'l' &&
                           std::tolower(effective_extension[2]) == 'o' && std::tolower(effective_extension[3]) == 'g');
        if (is_log_ext)
        {
            std::vector<std::string> extensions = {".log", ".out", ".LOG", ".OUT", ".Log", ".Out"};
            log_files                           = findLogFiles(extensions, effective_max_file_size_mb);
        }
        else
        {
            log_files = findLogFiles(effective_extension, effective_max_file_size_mb);
        }

        for (const auto& file : log_files)
        {
            auto data = calculate_high_level_energy(file);
            results.push_back(data);
        }

        // Sort by specified column
        std::sort(results.begin(), results.end(), [this](const HighLevelEnergyData& a, const HighLevelEnergyData& b) {
            return compare_results(a, b, sort_column_);
        });
    }
    catch (const std::exception& e)
    {
        if (has_context_ && context_->error_collector)
        {
            context_->error_collector->add_error("Error processing directory: " + std::string(e.what()));
        }
        else
        {
            std::cerr << "Error processing directory: " << e.what() << std::endl;
        }
    }

    return results;
}

std::vector<HighLevelEnergyData> HighLevelEnergyCalculator::process_directory_parallel(const std::string& extension,
                                                                                       unsigned int       thread_count,
                                                                                       bool               quiet)
{
    std::vector<HighLevelEnergyData> results;

    try
    {
        // Use extension and limits from shared ProcessingContext when available
        const std::string& effective_extension  = has_context_ ? context_->extension : extension;
        const size_t effective_max_file_size_mb = has_context_ ? context_->max_file_size_mb : DEFAULT_MAX_FILE_SIZE_MB;

        // Find all log files using the effective extension
        std::vector<std::string> filtered_files;
        // If using default extension (.log), search for both .log and .out files (case-insensitive)
        bool is_log_ext = (effective_extension.length() == 4 && std::tolower(effective_extension[1]) == 'l' &&
                           std::tolower(effective_extension[2]) == 'o' && std::tolower(effective_extension[3]) == 'g');
        if (is_log_ext)
        {
            std::vector<std::string> extensions = {".log", ".out", ".LOG", ".OUT", ".Log", ".Out"};
            filtered_files                      = findLogFiles(extensions, effective_max_file_size_mb);
        }
        else
        {
            filtered_files = findLogFiles(effective_extension, effective_max_file_size_mb);
        }

        if (filtered_files.empty())
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_warning("No files found with extension: " + effective_extension);
            }
            return results;
        }

        // Validate and prepare files
        auto validated_files = validate_and_prepare_files(filtered_files);

        if (validated_files.empty())
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_error("No valid files found for processing");
            }
            return results;
        }

        // Determine optimal thread count
        if (thread_count == 0)
        {
            size_t available_memory = has_context_ ? context_->memory_monitor->get_max_usage() / (1024 * 1024) : 1024;
            thread_count            = calculate_optimal_threads(validated_files.size(), available_memory);
        }

        // Process files using enhanced thread pool approach
        return process_files_with_thread_pool(validated_files, thread_count, 0, quiet);
    }
    catch (const std::exception& e)
    {
        if (has_context_ && context_->error_collector)
        {
            context_->error_collector->add_error("Error in parallel processing: " + std::string(e.what()));
        }
        else
        {
            std::cerr << "Error in parallel processing: " << e.what() << std::endl;
        }
        return results;
    }
}

std::vector<HighLevelEnergyData>
HighLevelEnergyCalculator::process_files_with_limits(const std::vector<std::string>& files,
                                                     unsigned int                    thread_count,
                                                     size_t                          memory_limit_mb,
                                                     bool                            quiet)
{

    std::vector<HighLevelEnergyData> results;

    if (files.empty())
    {
        return results;
    }

    // Adjust memory limit if needed
    if (memory_limit_mb == 0 && has_context_)
    {
        memory_limit_mb = context_->memory_monitor->get_max_usage() / (1024 * 1024);
    }
    else if (memory_limit_mb == 0)
    {
        memory_limit_mb = 512;  // Default 512MB
    }

    // Ensure we don't exceed system capabilities
    thread_count = std::min(thread_count, std::thread::hardware_concurrency());
    thread_count = std::min(thread_count, static_cast<unsigned int>(files.size()));

    if (thread_count <= 1)
    {
        // Fall back to sequential processing
        for (const auto& file : files)
        {
            try
            {
                auto data = calculate_high_level_energy(file);
                results.push_back(data);
            }
            catch (const std::exception& e)
            {
                if (has_context_ && context_->error_collector)
                {
                    context_->error_collector->add_error("Failed to process " + file + ": " + e.what());
                }
            }
        }
    }
    else
    {
        // Enhanced parallel processing with thread pool
        if (files.size() > 50)
        {
            // Use optimized thread pool for large datasets
            return process_files_with_thread_pool(files, thread_count, memory_limit_mb, quiet);
        }

        // Use original approach for smaller datasets to avoid thread pool overhead
        results.resize(files.size());
        std::vector<std::thread> workers;
        std::mutex               results_mutex;
        std::atomic<size_t>      progress_counter{0};
        std::atomic<bool>        should_stop{false};

        auto start_time = std::chrono::steady_clock::now();

        // Launch progress monitor
        std::thread monitor_thread;
        if (thread_count > 1)
        {  // Only show progress for parallel processing
            monitor_thread = std::thread([this, &progress_counter, &should_stop, start_time, files, quiet]() {
                while (!should_stop.load() && progress_counter.load() < files.size())
                {
                    monitor_progress(files.size(), progress_counter, start_time, quiet);
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Faster updates
                }
            });
        }

        // Launch worker threads
        size_t files_per_thread = files.size() / thread_count;
        size_t remaining_files  = files.size() % thread_count;

        for (unsigned int i = 0; i < thread_count; ++i)
        {
            size_t start_index = i * files_per_thread;
            size_t end_index   = start_index + files_per_thread;

            // Distribute remaining files
            if (i < remaining_files)
            {
                end_index += 1;
                start_index += i;
            }
            else
            {
                start_index += remaining_files;
                end_index += remaining_files;
            }

            if (start_index < files.size())
            {
                workers.emplace_back(&HighLevelEnergyCalculator::process_files_worker,
                                     this,
                                     std::cref(files),
                                     start_index,
                                     std::min(end_index, files.size()),
                                     std::ref(results),
                                     std::ref(results_mutex),
                                     std::ref(progress_counter));
            }
        }

        // Wait for all workers to complete
        for (auto& worker : workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }

        // Stop progress monitor
        should_stop.store(true);
        if (monitor_thread.joinable())
        {
            monitor_thread.join();
        }

        // Remove any failed results (empty entries)
        results.erase(std::remove_if(results.begin(),
                                     results.end(),
                                     [](const HighLevelEnergyData& data) {
                                         return data.filename.empty();
                                     }),
                      results.end());
    }

    // Sort results by specified column
    std::sort(results.begin(), results.end(), [this](const HighLevelEnergyData& a, const HighLevelEnergyData& b) {
        return compare_results(a, b, sort_column_);
    });

    return results;
}

// Print Gibbs-focused format (first bash script)
void HighLevelEnergyCalculator::print_gibbs_format(const std::vector<HighLevelEnergyData>& results,
                                                   bool                                    quiet,
                                                   std::ostream*                           output_file)
{
    // Use dynamic formatting by default
    print_gibbs_format_dynamic(results, quiet, output_file);
}

// Print components breakdown format (second bash script)
void HighLevelEnergyCalculator::print_components_format(const std::vector<HighLevelEnergyData>& results,
                                                        bool                                    quiet,
                                                        std::ostream*                           output_file)
{
    // Use dynamic formatting by default
    print_components_format_dynamic(results, quiet, output_file);
}

// Print Gibbs-focused CSV format (high-kj command)
void HighLevelEnergyCalculator::print_gibbs_csv_format(const std::vector<HighLevelEnergyData>& results,
                                                       bool                                    quiet,
                                                       std::ostream*                           output_file)
{
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file)
    {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    // Header box
    print_header_box(out);

    if (!quiet && !results.empty())
    {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    // CSV header for Gibbs format (matching first bash script output)
    out << "Output name,G kJ/mol,G a.u,G eV,LowFQ,Status,PhCorr\n";

    for (const auto& data : results)
    {
        std::string formatted_name = format_filename(data.filename, 52);
        std::string phase_corr     = data.phase_corr_applied ? "YES" : "NO";

        out << "\"" << formatted_name << "\"," << std::fixed << std::setprecision(6) << data.gibbs_kj_mol << ","
            << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected << "," << std::fixed
            << std::setprecision(6) << data.gibbs_ev << "," << std::fixed << std::setprecision(2)
            << data.lowest_frequency << "," << data.status << "," << phase_corr << "\n";
    }
}

// Print components breakdown CSV format (high-au command)
void HighLevelEnergyCalculator::print_components_csv_format(const std::vector<HighLevelEnergyData>& results,
                                                            bool                                    quiet,
                                                            std::ostream*                           output_file)
{
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file)
    {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    // Header box
    print_header_box(out);

    if (!quiet && !results.empty())
    {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    // CSV header for components format (matching second bash script output)
    out << "Output name,E high a.u,E low a.u,ZPE a.u,TC a.u,TS a.u,H a.u,G a.u,LowFQ,PhaseCorr\n";

    for (const auto& data : results)
    {
        std::string formatted_name = format_filename(data.filename, 53);
        std::string phase_corr     = data.phase_corr_applied ? "YES" : "NO";

        out << "\"" << formatted_name << "\"," << std::fixed << std::setprecision(6) << data.final_scf_high << ","
            << std::fixed << std::setprecision(6) << data.final_scf_low << "," << std::fixed << std::setprecision(6)
            << data.zpe << "," << std::fixed << std::setprecision(6) << data.tc_only << "," << std::fixed
            << std::setprecision(6) << data.ts_value << "," << std::fixed << std::setprecision(6)
            << data.enthalpy_hartree << "," << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected << ","
            << std::fixed << std::setprecision(2) << data.lowest_frequency << "," << phase_corr << "\n";
    }
}


// Optimized helper function with file caching and pre-compiled regex
double HighLevelEnergyCalculator::extract_value_from_file(const std::string& filename,
                                                          const std::string& pattern,
                                                          int                field_index,
                                                          int                occurrence,
                                                          bool               warn_if_missing)
{
    try
    {
        // Use cached file content to avoid redundant I/O
        std::string file_content = g_file_cache.get_or_read(filename);
        if (file_content.empty())
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_warning("Cannot read file: " + filename);
            }
            return 0.0;
        }

        // Get pre-compiled regex pattern
        const RegexPatterns& patterns  = RegexPatterns::get_instance();
        const std::regex*    regex_ptr = nullptr;

        // Map pattern strings to pre-compiled regex
        if (pattern == "SCF Done")
        {
            regex_ptr = &patterns.scf_done;
        }
        else if (pattern == "Total Energy, E\\(CIS")
        {
            regex_ptr = &patterns.cis_energy;
        }
        else if (pattern == "After PCM corrections, the energy is")
        {
            regex_ptr = &patterns.pcm_correction;
        }
        else if (pattern == "Total energy after correction")
        {
            regex_ptr = &patterns.clr_correction;
        }
        else if (pattern == "Zero-point correction")
        {
            regex_ptr = &patterns.zero_point;
        }
        else if (pattern == "Thermal correction to Enthalpy")
        {
            regex_ptr = &patterns.thermal_enthalpy;
        }
        else if (pattern == "Thermal correction to Gibbs Free Energy")
        {
            regex_ptr = &patterns.thermal_gibbs;
        }
        else if (pattern == "Thermal correction to Energy")
        {
            regex_ptr = &patterns.thermal_energy;
        }
        else if (pattern == "Total\\s+S")
        {
            regex_ptr = &patterns.entropy_total;
        }
        else if (pattern == "Kelvin\\.\\s+Pressure")
        {
            regex_ptr = &patterns.temperature_pattern;
        }
        else
        {
            // Fallback to runtime compilation for unknown patterns
            std::regex runtime_pattern(pattern);
            regex_ptr = &runtime_pattern;
        }

        std::vector<std::string> matches;
        std::istringstream       stream(file_content);
        std::string              line;

        while (std::getline(stream, line))
        {
            if (std::regex_search(line, *regex_ptr))
            {
                matches.push_back(line);
            }
        }

        if (matches.empty())
        {
            if (warn_if_missing && has_context_ && context_->error_collector)
            {
                context_->error_collector->add_warning("Pattern '" + pattern + "' not found in: " + filename);
            }
            return 0.0;
        }

        // Get the specified occurrence (-1 means last)
        std::string target_line;
        if (occurrence == -1)
        {
            target_line = matches.back();
        }
        else if (occurrence > 0 && occurrence <= static_cast<int>(matches.size()))
        {
            target_line = matches[occurrence - 1];
        }
        else
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_warning("Invalid occurrence " + std::to_string(occurrence) +
                                                       " for pattern '" + pattern + "' in: " + filename);
            }
            return 0.0;
        }

        // Extract the field
        std::istringstream iss(target_line);
        std::string        field;
        for (int i = 0; i < field_index && iss >> field; ++i)
        {
            // Continue reading until we reach the desired field
        }

        if (!field.empty())
        {
            return safe_parse_energy(field, filename + " (pattern: " + pattern + ")");
        }
    }
    catch (const std::exception& e)
    {
        if (has_context_ && context_->error_collector)
        {
            context_->error_collector->add_error("Error extracting value from " + filename + ": " + e.what());
        }
    }

    return 0.0;
}


bool HighLevelEnergyCalculator::extract_low_level_thermal_data(const std::string&   parent_file,
                                                               HighLevelEnergyData& data)
{
    try
    {
        // Check if parent file exists
        if (!file_exists(parent_file))
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_error("Parent file not found: " + parent_file);
            }
            return false;
        }

        // Extract thermal corrections from parent directory file
        data.scf_low       = extract_value_from_file(parent_file, "SCF Done", 5, -1);
        data.scf_td_low    = extract_value_from_file(parent_file, "Total Energy, E\\(CIS", 5, -1, false);
        data.zpe           = extract_value_from_file(parent_file, "Zero-point correction", 3, -1);
        data.tc_enthalpy   = extract_value_from_file(parent_file, "Thermal correction to Enthalpy", 5, -1);
        data.tc_gibbs      = extract_value_from_file(parent_file, "Thermal correction to Gibbs Free Energy", 7, -1);
        data.tc_energy     = extract_value_from_file(parent_file, "Thermal correction to Energy", 5, -1);
        data.entropy_total = extract_value_from_file(parent_file, "Total\\s+S", 2, -1, false);

        // Validate extracted thermal data
        if (data.zpe == 0.0 && data.tc_enthalpy == 0.0 && data.tc_gibbs == 0.0)
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_warning("No thermal corrections found in: " + parent_file);
            }
        }

        // Determine final low-level electronic energy
        if (data.scf_td_low != 0.0)
        {
            data.final_scf_low = data.scf_td_low;
        }
        else
        {
            data.final_scf_low = data.scf_low;
        }

        // Extract temperature from parent file
        double temp = extract_value_from_file(parent_file, "Kelvin\\.\\s+Pressure", 2, -1, false);
        if (temp > 0.0 && HighLevelEnergyUtils::validate_temperature(temp))
        {
            data.temperature = temp;
        }
        else
        {
            data.temperature = temperature_;
            if (has_context_ && context_->error_collector && temp > 0.0)
            {
                context_->error_collector->add_warning("Invalid temperature (" + std::to_string(temp) + ") found in " +
                                                       parent_file + ", using default");
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        if (has_context_ && context_->error_collector)
        {
            context_->error_collector->add_error("Failed to extract thermal data from " + parent_file + ": " +
                                                 e.what());
        }
        return false;
    }
}

double HighLevelEnergyCalculator::calculate_phase_correction(double temp, double concentration_mol_m3)
{
    // Delta_G_corr = RT*ln(C*RT/Po) * conversion_factor
    double rt    = R_CONSTANT * temp;
    double ratio = concentration_mol_m3 * rt / PO_CONSTANT;
    return rt * std::log(ratio) * PHASE_CORR_FACTOR / 1000.0;
}

double HighLevelEnergyCalculator::calculate_thermal_only(double tc_with_zpe, double zpe)
{
    return tc_with_zpe - zpe;
}

double HighLevelEnergyCalculator::calculate_ts_value(double entropy_total, double temp)
{
    // TS = H_corr - G_corr (difference between enthalpy and Gibbs corrections)
    // This is calculated directly in the main function now
    return entropy_total * temp / 627509.6080305927;
}

double HighLevelEnergyCalculator::extract_lowest_frequency(const std::string& parent_file)
{
    try
    {
        std::string content     = read_file_content(parent_file);
        auto        frequencies = HighLevelEnergyUtils::extract_frequencies(content);
        return HighLevelEnergyUtils::find_lowest_frequency(frequencies);
    }
    catch (const std::exception& e)
    {
        return 0.0;
    }
}

std::string HighLevelEnergyCalculator::determine_job_status(const std::string& filename)
{
    try
    {
        std::string tail_content = read_file_tail(filename, 10);

        if (tail_content.find("Normal") != std::string::npos)
        {
            return "DONE";
        }

        // Check for error patterns
        bool has_error    = false;
        bool has_error_on = false;

        std::istringstream iss(tail_content);
        std::string        line;
        while (std::getline(iss, line))
        {
            if (line.find("Error") != std::string::npos)
            {
                has_error = true;
                if (line.find("Error on") != std::string::npos)
                {
                    has_error_on = true;
                }
            }
        }

        if (has_error && !has_error_on)
        {
            return "ERROR";
        }

        return "UNDONE";
    }
    catch (const std::exception& e)
    {
        return "UNKNOWN";
    }
}

std::string HighLevelEnergyCalculator::get_parent_file(const std::string& high_level_file)
{
    return "../" + high_level_file;
}

bool HighLevelEnergyCalculator::file_exists(const std::string& filename)
{
    return std::filesystem::exists(filename);
}

std::string HighLevelEnergyCalculator::read_file_content(const std::string& filename)
{
    // Use file cache for better performance
    return g_file_cache.get_or_read(filename);
}

std::string HighLevelEnergyCalculator::read_file_tail(const std::string& filename, int lines)
{
    // Use cached content when possible
    std::string content = g_file_cache.get_or_read(filename);
    if (content.empty())
    {
        throw std::runtime_error("Cannot read file: " + filename);
    }

    // Efficiently extract last N lines
    std::vector<std::string> all_lines;
    all_lines.reserve(lines * 2);  // Pre-allocate for efficiency

    std::istringstream stream(content);
    std::string        line;
    while (std::getline(stream, line))
    {
        all_lines.push_back(line);
        // Keep only the last 'lines' entries for memory efficiency
        if (all_lines.size() > static_cast<size_t>(lines * 2))
        {
            all_lines.erase(all_lines.begin(), all_lines.begin() + lines);
        }
    }

    std::ostringstream result;
    size_t             start_index = (all_lines.size() > static_cast<size_t>(lines)) ? all_lines.size() - lines : 0;

    for (size_t i = start_index; i < all_lines.size(); ++i)
    {
        result << all_lines[i] << "\n";
    }

    return result.str();
}

void HighLevelEnergyCalculator::print_gibbs_header(std::ostream* output_file)
{
    std::ostream& out = output_file ? *output_file : std::cout;

    out << std::setw(52) << "Output name" << std::setw(18) << "G kJ/mol" << std::setw(15) << "G a.u" << std::setw(15)
        << "G eV" << std::setw(12) << "LowFQ" << std::setw(8) << "Status" << std::setw(8) << "PhCorr" << std::endl;

    out << std::setw(52) << "-----------" << std::setw(18) << "----------" << std::setw(15) << "---------"
        << std::setw(15) << "----------" << std::setw(12) << "-----" << std::setw(8) << "------" << std::setw(8)
        << "------" << std::endl;
}

void HighLevelEnergyCalculator::print_components_header(std::ostream* output_file)
{
    std::ostream& out = output_file ? *output_file : std::cout;

    out << std::setw(53) << "Output name" << std::setw(15) << "E high a.u" << std::setw(15) << "E low a.u"
        << std::setw(10) << "ZPE a.u" << std::setw(10) << "TC a.u" << std::setw(10) << "TS a.u" << std::setw(15)
        << "H a.u" << std::setw(15) << "G a.u" << std::setw(10) << "LowFQ" << std::setw(11) << "PhaseCorr" << std::endl;

    out << std::setw(53) << "-----------" << std::setw(15) << "----------" << std::setw(15) << "----------"
        << std::setw(10) << "------" << std::setw(10) << "-------" << std::setw(10) << "------" << std::setw(15)
        << "------" << std::setw(15) << "------" << std::setw(10) << "------" << std::setw(11) << "------" << std::endl;
}

std::string HighLevelEnergyCalculator::format_filename(const std::string& filename, int max_length)
{
    if (static_cast<int>(filename.length()) <= max_length)
    {
        return filename;
    }

    // Truncate from the beginning, keeping the end
    int start_pos = filename.length() - max_length;
    return filename.substr(start_pos);
}

void HighLevelEnergyCalculator::print_summary_info(const std::string& last_parent_file, std::ostream* output_file)
{
    std::ostream& out = output_file ? *output_file : std::cout;

    try
    {
        double last_temp = extract_value_from_file(last_parent_file, "Kelvin\\.\\s+Pressure", 2, -1, false);
        if (last_temp == 0.0)
            last_temp = temperature_;

        double last_phase_corr = calculate_phase_correction(last_temp, concentration_mol_m3_);

        out << "Temperature in " << last_parent_file << ": " << std::fixed << std::setprecision(3) << last_temp
            << " K. Make sure that temperature has been used in your input." << std::endl;
        out << "The concentration for phase correction: " << std::fixed << std::setprecision(0) << concentration_m_
            << " M or " << concentration_mol_m3_ << " mol/m3" << std::endl;
        out << "Last Gibbs free correction for phase changing from 1 atm to 1 M: " << std::fixed << std::setprecision(6)
            << last_phase_corr << " au" << std::endl;
    }
    catch (const std::exception& e)
    {
        out << "Error printing summary: " << e.what() << std::endl;
    }
}

// Dynamic formatting implementations
std::vector<int>
HighLevelEnergyCalculator::calculate_gibbs_column_widths(const std::vector<HighLevelEnergyData>& results)
{
    // Calculate optimal column widths for Gibbs format output
    std::vector<int> widths(7);

    // Initialize with header lengths + 3 spaces padding
    widths[0] = std::max(11, 52);  // "Output name" or minimum filename width
    widths[1] = std::max(8, 15);   // "G kJ/mol"
    widths[2] = std::max(6, 12);   // "G a.u"
    widths[3] = std::max(4, 12);   // "G eV"
    widths[4] = std::max(5, 10);   // "LowFQ"
    widths[5] = std::max(6, 8);    // "Status"
    widths[6] = std::max(6, 8);    // "PhCorr"

    // Use faster formatting with pre-allocated buffer
    char buffer[64];
    for (const auto& data : results)
    {
        // Check filename length (limited to reasonable max)
        std::string formatted_name = format_filename(data.filename, 70);
        widths[0]                  = std::max(widths[0], static_cast<int>(formatted_name.length()) + 3);

        // Use snprintf for faster formatting
        int len;

        // Check G kJ/mol length
        len       = snprintf(buffer, sizeof(buffer), "%.6f", data.gibbs_kj_mol);
        widths[1] = std::max(widths[1], len + 3);

        // Check G a.u length
        len       = snprintf(buffer, sizeof(buffer), "%.6f", data.gibbs_hartree_corrected);
        widths[2] = std::max(widths[2], len + 3);

        // Check G eV length
        len       = snprintf(buffer, sizeof(buffer), "%.6f", data.gibbs_ev);
        widths[3] = std::max(widths[3], len + 3);

        // Check LowFQ length
        len       = snprintf(buffer, sizeof(buffer), "%.4f", data.lowest_frequency);
        widths[4] = std::max(widths[4], len + 3);

        // Check Status length
        widths[5] = std::max(widths[5], static_cast<int>(data.status.length()) + 3);

        // Check PhCorr length
        std::string phase_corr = data.phase_corr_applied ? "YES" : "NO";
        widths[6]              = std::max(widths[6], static_cast<int>(phase_corr.length()) + 3);
    }

    return widths;
}

std::vector<int>
HighLevelEnergyCalculator::calculate_components_column_widths(const std::vector<HighLevelEnergyData>& results)
{
    // Column order: filename, E high a.u, E low a.u, ZPE a.u, TC a.u, TS a.u, H a.u, G a.u, LowFQ, PhaseCorr
    std::vector<int> widths(10);

    // Initialize with header lengths + 3 spaces padding
    widths[0] = std::max(11, 52);  // "Output name"
    widths[1] = std::max(10, 15);  // "E high a.u"
    widths[2] = std::max(9, 15);   // "E low a.u"
    widths[3] = std::max(7, 10);   // "ZPE a.u"
    widths[4] = std::max(6, 10);   // "TC a.u"
    widths[5] = std::max(6, 10);   // "TS a.u"
    widths[6] = std::max(5, 15);   // "H a.u"
    widths[7] = std::max(5, 15);   // "G a.u"
    widths[8] = std::max(5, 10);   // "LowFQ"
    widths[9] = std::max(9, 11);   // "PhaseCorr"

    for (const auto& data : results)
    {
        // Check filename length (limited to reasonable max)
        std::string formatted_name = format_filename(data.filename, 70);
        widths[0]                  = std::max(widths[0], static_cast<int>(formatted_name.length()) + 3);

        // Check E high a.u length
        std::ostringstream ss1;
        ss1 << std::fixed << std::setprecision(8) << data.final_scf_high;
        widths[1] = std::max(widths[1], static_cast<int>(ss1.str().length()) + 3);

        // Check E low a.u length
        std::ostringstream ss2;
        ss2 << std::fixed << std::setprecision(8) << data.final_scf_low;
        widths[2] = std::max(widths[2], static_cast<int>(ss2.str().length()) + 3);

        // Check ZPE a.u length
        std::ostringstream ss3;
        ss3 << std::fixed << std::setprecision(6) << data.zpe;
        widths[3] = std::max(widths[3], static_cast<int>(ss3.str().length()) + 3);

        // Check TC a.u length
        std::ostringstream ss4;
        ss4 << std::fixed << std::setprecision(6) << data.tc_only;
        widths[4] = std::max(widths[4], static_cast<int>(ss4.str().length()) + 3);

        // Check TS a.u length
        std::ostringstream ss5;
        ss5 << std::fixed << std::setprecision(6) << data.ts_value;
        widths[5] = std::max(widths[5], static_cast<int>(ss5.str().length()) + 3);

        // Check H a.u length
        std::ostringstream ss6;
        ss6 << std::fixed << std::setprecision(6) << data.enthalpy_hartree;
        widths[6] = std::max(widths[6], static_cast<int>(ss6.str().length()) + 3);

        // Check G a.u length
        std::ostringstream ss7;
        ss7 << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected;
        widths[7] = std::max(widths[7], static_cast<int>(ss7.str().length()) + 3);

        // Check LowFQ length
        std::ostringstream ss8;
        ss8 << std::fixed << std::setprecision(4) << data.lowest_frequency;
        widths[8] = std::max(widths[8], static_cast<int>(ss8.str().length()) + 3);

        // Check PhaseCorr length
        std::string phase_corr = data.phase_corr_applied ? "YES" : "NO";
        widths[9]              = std::max(widths[9], static_cast<int>(phase_corr.length()) + 3);
    }

    return widths;
}

void HighLevelEnergyCalculator::print_gibbs_header_dynamic(const std::vector<int>& column_widths,
                                                           std::ostream*           output_file)
{
    std::ostream& out = output_file ? *output_file : std::cout;

    out << std::setw(column_widths[0]) << "Output name" << std::setw(column_widths[1]) << "G kJ/mol"
        << std::setw(column_widths[2]) << "G a.u" << std::setw(column_widths[3]) << "G eV"
        << std::setw(column_widths[4]) << "LowFQ" << std::setw(column_widths[5]) << "Status"
        << std::setw(column_widths[6]) << "PhCorr" << std::endl;

    // Print separator line
    out << std::setw(column_widths[0]) << std::string(column_widths[0] - 3, '-') << std::setw(column_widths[1])
        << std::string(column_widths[1] - 3, '-') << std::setw(column_widths[2])
        << std::string(column_widths[2] - 3, '-') << std::setw(column_widths[3])
        << std::string(column_widths[3] - 3, '-') << std::setw(column_widths[4])
        << std::string(column_widths[4] - 3, '-') << std::setw(column_widths[5])
        << std::string(column_widths[5] - 3, '-') << std::setw(column_widths[6])
        << std::string(column_widths[6] - 3, '-') << std::endl;
}

void HighLevelEnergyCalculator::print_components_header_dynamic(const std::vector<int>& column_widths,
                                                                std::ostream*           output_file)
{
    std::ostream& out = output_file ? *output_file : std::cout;

    out << std::setw(column_widths[0]) << "Output name" << std::setw(column_widths[1]) << "E high a.u"
        << std::setw(column_widths[2]) << "E low a.u" << std::setw(column_widths[3]) << "ZPE a.u"
        << std::setw(column_widths[4]) << "TC a.u" << std::setw(column_widths[5]) << "TS a.u"
        << std::setw(column_widths[6]) << "H a.u" << std::setw(column_widths[7]) << "G a.u"
        << std::setw(column_widths[8]) << "LowFQ" << std::setw(column_widths[9]) << "PhaseCorr" << std::endl;

    // Print separator line
    out << std::setw(column_widths[0]) << std::string(column_widths[0] - 3, '-') << std::setw(column_widths[1])
        << std::string(column_widths[1] - 3, '-') << std::setw(column_widths[2])
        << std::string(column_widths[2] - 3, '-') << std::setw(column_widths[3])
        << std::string(column_widths[3] - 3, '-') << std::setw(column_widths[4])
        << std::string(column_widths[4] - 3, '-') << std::setw(column_widths[5])
        << std::string(column_widths[5] - 3, '-') << std::setw(column_widths[6])
        << std::string(column_widths[6] - 3, '-') << std::setw(column_widths[7])
        << std::string(column_widths[7] - 3, '-') << std::setw(column_widths[8])
        << std::string(column_widths[8] - 3, '-') << std::setw(column_widths[9])
        << std::string(column_widths[9] - 3, '-') << std::endl;
}

void HighLevelEnergyCalculator::print_gibbs_format_dynamic(const std::vector<HighLevelEnergyData>& results,
                                                           bool                                    quiet,
                                                           std::ostream*                           output_file)
{
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file)
    {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    // Header box
    print_header_box(out);

    if (!quiet && !results.empty())
    {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    auto column_widths = calculate_gibbs_column_widths(results);
    print_gibbs_header_dynamic(column_widths, output_file);

    for (const auto& data : results)
    {
        std::string formatted_name = format_filename(data.filename, column_widths[0] - 3);
        std::string phase_corr     = data.phase_corr_applied ? "YES" : "NO";

        out << std::setw(column_widths[0]) << formatted_name << std::setw(column_widths[1]) << std::fixed
            << std::setprecision(6) << data.gibbs_kj_mol << std::setw(column_widths[2]) << std::fixed
            << std::setprecision(6) << data.gibbs_hartree_corrected << std::setw(column_widths[3]) << std::fixed
            << std::setprecision(6) << data.gibbs_ev << std::setw(column_widths[4]) << std::fixed
            << std::setprecision(4) << data.lowest_frequency << std::setw(column_widths[5]) << data.status
            << std::setw(column_widths[6]) << phase_corr << std::endl;
    }
}

void HighLevelEnergyCalculator::print_components_format_dynamic(const std::vector<HighLevelEnergyData>& results,
                                                                bool                                    quiet,
                                                                std::ostream*                           output_file)
{
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file)
    {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    // Header box
    print_header_box(out);

    if (!quiet && !results.empty())
    {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    auto column_widths = calculate_components_column_widths(results);
    print_components_header_dynamic(column_widths, output_file);

    for (const auto& data : results)
    {
        std::string formatted_name = format_filename(data.filename, column_widths[0] - 3);
        std::string phase_corr     = data.phase_corr_applied ? "YES" : "NO";

        out << std::setw(column_widths[0]) << formatted_name << std::setw(column_widths[1]) << std::fixed
            << std::setprecision(6) << data.final_scf_high << std::setw(column_widths[2]) << std::fixed
            << std::setprecision(6) << data.final_scf_low << std::setw(column_widths[3]) << std::fixed
            << std::setprecision(6) << data.zpe << std::setw(column_widths[4]) << std::fixed << std::setprecision(6)
            << data.tc_only << std::setw(column_widths[5]) << std::fixed << std::setprecision(6) << data.ts_value
            << std::setw(column_widths[6]) << std::fixed << std::setprecision(6) << data.enthalpy_hartree
            << std::setw(column_widths[7]) << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected
            << std::setw(column_widths[8]) << std::fixed << std::setprecision(4) << data.lowest_frequency
            << std::setw(column_widths[9]) << phase_corr << std::endl;
    }
}

// Static formatting functions (for backwards compatibility and consistent alignment)
void HighLevelEnergyCalculator::print_gibbs_format_static(const std::vector<HighLevelEnergyData>& results,
                                                          bool                                    quiet,
                                                          std::ostream*                           output_file)
{
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file)
    {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    // Header box
    print_header_box(out);

    if (!quiet && !results.empty())
    {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    print_gibbs_header(output_file);

    for (const auto& data : results)
    {
        std::string formatted_name = format_filename(data.filename, 52);
        std::string phase_corr     = data.phase_corr_applied ? "YES" : "NO";

        out << std::setw(52) << formatted_name << std::setw(18) << std::fixed << std::setprecision(6)
            << data.gibbs_kj_mol << std::setw(15) << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected
            << std::setw(15) << std::fixed << std::setprecision(6) << data.gibbs_ev << std::setw(12) << std::fixed
            << std::setprecision(4) << data.lowest_frequency << std::setw(8) << data.status << std::setw(8)
            << phase_corr << std::endl;
    }
}

void HighLevelEnergyCalculator::print_components_format_static(const std::vector<HighLevelEnergyData>& results,
                                                               bool                                    quiet,
                                                               std::ostream*                           output_file)
{
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file)
    {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    // Header box
    print_header_box(out);

    if (!quiet && !results.empty())
    {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    print_components_header(output_file);

    for (const auto& data : results)
    {
        std::string formatted_name = format_filename(data.filename, 53);
        std::string phase_corr     = data.phase_corr_applied ? "YES" : "NO";

        out << std::setw(53) << formatted_name << std::setw(15) << std::fixed << std::setprecision(6)
            << data.final_scf_high << std::setw(15) << std::fixed << std::setprecision(6) << data.final_scf_low
            << std::setw(10) << std::fixed << std::setprecision(6) << data.zpe << std::setw(10) << std::fixed
            << std::setprecision(6) << data.tc_only << std::setw(10) << std::fixed << std::setprecision(6)
            << data.ts_value << std::setw(15) << std::fixed << std::setprecision(6) << data.enthalpy_hartree
            << std::setw(15) << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected << std::setw(11)
            << std::fixed << std::setprecision(4) << data.lowest_frequency << std::setw(10) << phase_corr << std::endl;
    }
}
// Enhanced worker function for parallel processing
void HighLevelEnergyCalculator::process_files_worker(const std::vector<std::string>&   files,
                                                     size_t                            start_index,
                                                     size_t                            end_index,
                                                     std::vector<HighLevelEnergyData>& results,
                                                     std::mutex&                       results_mutex,
                                                     std::atomic<size_t>&              progress_counter)
{
    for (size_t i = start_index; i < end_index; ++i)
    {
        try
        {
            // Memory check before processing
            if (has_context_ && context_->memory_monitor)
            {
                if (!context_->memory_monitor->can_allocate(50 * 1024 * 1024))
                {  // 50MB safety margin
                    if (context_->error_collector)
                    {
                        context_->error_collector->add_warning("Memory limit reached, skipping " + files[i]);
                    }
                    continue;
                }
            }

            // Process the file
            auto data = calculate_high_level_energy(files[i]);

            // Thread-safe result storage
            {
                std::lock_guard<std::mutex> lock(results_mutex);
                if (i < results.size())
                {
                    results[i] = data;
                }
            }
        }
        catch (const std::exception& e)
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_error("Worker failed to process " + files[i] + ": " + e.what());
            }

            // Store empty result to maintain indexing
            {
                std::lock_guard<std::mutex> lock(results_mutex);
                if (i < results.size())
                {
                    results[i]        = HighLevelEnergyData(files[i]);
                    results[i].status = "ERROR";
                }
            }
        }

        // Update progress
        progress_counter.fetch_add(1);
    }
}

unsigned int HighLevelEnergyCalculator::calculate_optimal_threads(size_t file_count, size_t available_memory)
{
    // Basic thread calculation
    unsigned int max_threads = std::thread::hardware_concurrency();
    if (max_threads == 0)
        max_threads = 4;  // Fallback

    // Memory-based limitation (assume ~50MB per thread)
    unsigned int memory_limited_threads = static_cast<unsigned int>(available_memory / 50);

    // File-based limitation
    unsigned int file_limited_threads =
        static_cast<unsigned int>(std::min(file_count, static_cast<size_t>(max_threads)));

    // Take the minimum of all constraints
    unsigned int optimal = std::min({max_threads, memory_limited_threads, file_limited_threads});

    // Ensure at least 1 thread
    return std::max(1u, optimal);
}

std::vector<std::string> HighLevelEnergyCalculator::validate_and_prepare_files(const std::vector<std::string>& files)
{
    std::vector<std::string> validated_files;

    // Get max file size from context or use default
    size_t max_file_size_bytes = 100 * 1024 * 1024;  // Default 100MB
    if (has_context_)
    {
        max_file_size_bytes = context_->max_file_size_mb * 1024 * 1024;
    }

    for (const auto& file : files)
    {
        try
        {
            // Check file existence and accessibility
            if (!file_exists(file))
            {
                if (has_context_ && context_->error_collector)
                {
                    context_->error_collector->add_warning("File not found: " + file);
                }
                continue;
            }

            // Check file size using CommandContext limit
            std::filesystem::path file_path(file);
            if (std::filesystem::exists(file_path))
            {
                auto file_size = std::filesystem::file_size(file_path);
                if (file_size > max_file_size_bytes)
                {
                    if (has_context_ && context_->error_collector)
                    {
                        context_->error_collector->add_warning(
                            "File too large (" + std::to_string(file_size / (1024 * 1024)) + "MB > " +
                            std::to_string(max_file_size_bytes / (1024 * 1024)) + "MB), skipping: " + file);
                    }
                    continue;
                }
            }

            validated_files.push_back(file);
        }
        catch (const std::exception& e)
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_warning("File validation failed for " + file + ": " + e.what());
            }
        }
    }

    return validated_files;
}

bool HighLevelEnergyCalculator::monitor_progress(size_t                                total_files,
                                                 const std::atomic<size_t>&            progress_counter,
                                                 std::chrono::steady_clock::time_point start_time,
                                                 bool                                  quiet)
{
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed      = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);

    size_t completed  = progress_counter.load();
    double percentage = (static_cast<double>(completed) / total_files) * 100.0;

    // Estimate time remaining
    double files_per_second = completed > 0 ? static_cast<double>(completed) / elapsed.count() : 0.0;
    auto   estimated_remaining =
        files_per_second > 0 ? std::chrono::seconds(static_cast<long>((total_files - completed) / files_per_second))
                               : std::chrono::seconds(0);
    std::cout << "Estimated time remaining: " << estimated_remaining.count() << " seconds\n";

    // Print progress only if not in quiet mode
    if (!quiet)
    {
        std::cout << "Processed " << completed << "/" << total_files << " files (" << std::fixed << std::setprecision(1)
                  << percentage << "%)" << std::endl;
    }

    // Memory monitoring
    if (has_context_ && context_->memory_monitor)
    {
        auto current_usage = context_->memory_monitor->get_current_usage();
        auto max_usage     = context_->memory_monitor->get_max_usage();
        if (current_usage > max_usage * 0.9)
        {  // 90% memory usage warning
            if (context_->error_collector)
            {
                context_->error_collector->add_warning(
                    "High memory usage detected: " + std::to_string(current_usage / (1024 * 1024)) + "MB");
            }
        }
    }

    return completed < total_files;
}

std::string HighLevelEnergyCalculator::safe_read_file(const std::string& filename, size_t max_size_mb)
{
    std::string content;

    try
    {
        // Check file size first
        std::filesystem::path file_path(filename);
        if (!std::filesystem::exists(file_path))
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_error("File not found: " + filename);
            }
            return content;
        }

        auto file_size = std::filesystem::file_size(file_path);

        // Use max_file_size_mb from context if available, otherwise use parameter
        size_t effective_max_size_mb = max_size_mb;
        if (has_context_)
        {
            effective_max_size_mb = context_->max_file_size_mb;
        }
        size_t max_size_bytes = effective_max_size_mb * 1024 * 1024;

        if (file_size > max_size_bytes)
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_warning("File too large (" + std::to_string(file_size / (1024 * 1024)) +
                                                       "MB > " + std::to_string(effective_max_size_mb) +
                                                       "MB), truncating: " + filename);
            }
            file_size = max_size_bytes;
        }

        // Memory allocation check
        if (has_context_ && context_->memory_monitor)
        {
            if (!context_->memory_monitor->can_allocate(file_size))
            {
                if (context_->error_collector)
                {
                    context_->error_collector->add_error("Insufficient memory to read file: " + filename);
                }
                return content;
            }
        }

        // File handle management
        if (has_context_ && context_->file_manager)
        {
            auto file_guard = context_->file_manager->acquire();
            if (!file_guard.is_acquired())
            {
                if (context_->error_collector)
                {
                    context_->error_collector->add_warning("No file handles available for: " + filename);
                }
                return content;
            }

            std::ifstream file(filename, std::ios::binary);
            if (file.is_open())
            {
                content.resize(file_size);
                file.read(&content[0], file_size);
                content.resize(file.gcount());  // Adjust to actual read size
            }
        }
        else
        {
            // Fallback without file handle management
            std::ifstream file(filename, std::ios::binary);
            if (file.is_open())
            {
                content.resize(file_size);
                file.read(&content[0], file_size);
                content.resize(file.gcount());
            }
        }
    }
    catch (const std::exception& e)
    {
        if (has_context_ && context_->error_collector)
        {
            context_->error_collector->add_error("Failed to read file " + filename + ": " + e.what());
        }
        content.clear();
    }

    return content;
}

double HighLevelEnergyCalculator::safe_parse_energy(const std::string& value_str, const std::string& context_info)
{
    try
    {
        if (value_str.empty())
        {
            return 0.0;
        }

        double value = std::stod(value_str);

        // Validate range (reasonable energy values)
        if (std::isnan(value) || std::isinf(value))
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_warning("Invalid energy value (NaN/Inf) in " + context_info);
            }
            return 0.0;
        }

        // Check reasonable energy range (in Hartree)
        if (std::abs(value) > 10000.0)
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_warning("Unusually large energy value (" + std::to_string(value) +
                                                       ") in " + context_info);
            }
        }

        return value;
    }
    catch (const std::exception& e)
    {
        if (has_context_ && context_->error_collector)
        {
            context_->error_collector->add_error("Failed to parse energy '" + value_str + "' in " + context_info +
                                                 ": " + e.what());
        }
        return 0.0;
    }
}

bool HighLevelEnergyCalculator::validate_processing_context()
{
    if (!context_)
    {
        return false;
    }

    try
    {
        // Check memory monitor
        if (!context_->memory_monitor)
        {
            return false;
        }

        // Check file manager
        if (!context_->file_manager)
        {
            return false;
        }

        // Check error collector
        if (!context_->error_collector)
        {
            return false;
        }

        // Test basic functionality
        auto test_guard   = context_->file_manager->acquire();
        bool can_allocate = context_->memory_monitor->can_allocate(1024);  // 1KB test

        return can_allocate;  // If we can allocate, context is likely valid
    }
    catch (const std::exception&)
    {
        return false;
    }
}

// Comparison function for sorting based on visual column numbers
bool HighLevelEnergyCalculator::compare_results(const HighLevelEnergyData& a, const HighLevelEnergyData& b, int column)
{
    if (is_au_format_)
    {
        // High-AU format: Output name, E high a.u, E low a.u, ZPE a.u, TC a.u, TS a.u, H a.u, G a.u, LowFQ, PhaseCorr
        switch (column)
        {
            case 1:  // Output name
                return a.filename < b.filename;
            case 2:  // E high a.u
                return a.final_scf_high < b.final_scf_high;
            case 3:  // E low a.u
                return a.final_scf_low < b.final_scf_low;
            case 4:  // ZPE a.u
                return a.zpe < b.zpe;
            case 5:  // TC a.u
                return a.tc_only < b.tc_only;
            case 6:  // TS a.u
                return a.ts_value < b.ts_value;
            case 7:  // H a.u
                return a.enthalpy_hartree < b.enthalpy_hartree;
            case 8:  // G a.u
                return a.gibbs_hartree_corrected < b.gibbs_hartree_corrected;
            case 9:  // LowFQ
                return a.lowest_frequency < b.lowest_frequency;
            case 10:  // PhaseCorr (YES comes before NO)
                return a.phase_corr_applied > b.phase_corr_applied;
            default:  // Default to G a.u
                return a.gibbs_hartree_corrected < b.gibbs_hartree_corrected;
        }
    }
    else
    {
        // High-KJ format: Output name, G kJ/mol, G a.u, G eV, LowFQ, Status, PhCorr
        switch (column)
        {
            case 1:  // Output name
                return a.filename < b.filename;
            case 2:  // G kJ/mol
                return a.gibbs_kj_mol < b.gibbs_kj_mol;
            case 3:  // G a.u
                return a.gibbs_hartree_corrected < b.gibbs_hartree_corrected;
            case 4:  // G eV
                return a.gibbs_ev < b.gibbs_ev;
            case 5:  // LowFQ
                return a.lowest_frequency < b.lowest_frequency;
            case 6:  // Status (alphabetical)
                return a.status < b.status;
            case 7:  // PhCorr (YES comes before NO)
                return a.phase_corr_applied > b.phase_corr_applied;
            default:  // Default to G kJ/mol
                return a.gibbs_kj_mol < b.gibbs_kj_mol;
        }
    }
}

// Utility namespace implementations
namespace HighLevelEnergyUtils
{

    // This function needs to be improved to handle larger numbers of files and support more extensions such as .out
    // This function is replaced by the function findLogFiles in gaussian_extractor.cpp
    // std::vector<std::string> find_log_files(const std::string& directory) {
    //    std::vector<std::string> log_files;
    //
    //    try {
    //        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    //            if (entry.is_regular_file() && entry.path().extension() == ".log") {
    //                log_files.push_back(entry.path().filename().string());
    //            }
    //        }
    //
    //        std::sort(log_files.begin(), log_files.end());
    //    } catch (const std::exception& e) {
    //        std::cerr << "Error finding log files: " << e.what() << std::endl;
    //    }
    //
    //    return log_files;
    //}

    std::string get_current_directory_name()
    {
        try
        {
            return std::filesystem::current_path().filename().string();
        }
        catch (const std::exception& e)
        {
            return "unknown";
        }
    }

    bool is_valid_high_level_directory()
    {
        // Default to configured extension and size via common command defaults
        // Fallbacks: ".log" and DEFAULT_MAX_FILE_SIZE_MB if config is not available here
        std::string default_ext    = ".log";
        size_t      default_max_mb = DEFAULT_MAX_FILE_SIZE_MB;

        std::vector<std::string> filtered_files = findLogFiles(default_ext, default_max_mb);
        bool                     has_parent     = std::filesystem::exists("../");
        return !filtered_files.empty() && has_parent;
    }

    bool is_valid_high_level_directory(const std::string& extension, size_t max_file_size_mb)
    {
        std::vector<std::string> filtered_files = findLogFiles(extension, max_file_size_mb);
        bool                     has_parent     = std::filesystem::exists("../");
        return !filtered_files.empty() && has_parent;
    }

    double parse_energy_value(const std::string& line, int field_index)
    {
        std::istringstream iss(line);
        std::string        field;
        for (int i = 0; i < field_index && iss >> field; ++i)
        {
            // Continue reading
        }

        try
        {
            return std::stod(field);
        }
        catch (const std::exception& e)
        {
            return 0.0;
        }
    }

    std::vector<double> extract_frequencies(const std::string& content)
    {
        std::vector<double> frequencies;
        std::istringstream  iss(content);
        std::string         line;

        while (std::getline(iss, line))
        {
            if (line.find("Frequencies") != std::string::npos)
            {
                // Parse frequencies from line like: "Frequencies --   -26.1161    19.3167    45.8905"
                std::istringstream line_iss(line);
                std::string        word;
                bool               found_frequencies = false;

                while (line_iss >> word)
                {
                    if (word == "--")
                    {
                        found_frequencies = true;
                        continue;
                    }
                    if (found_frequencies)
                    {
                        try
                        {
                            double freq = std::stod(word);
                            frequencies.push_back(freq);
                        }
                        catch (const std::exception& e)
                        {
                            // Skip non-numeric words
                        }
                    }
                }
            }
        }

        return frequencies;
    }

    double find_lowest_frequency(const std::vector<double>& frequencies)
    {
        if (frequencies.empty())
        {
            return 0.0;
        }

        // Return the lowest frequency (most negative if any, otherwise smallest positive)
        return *std::min_element(frequencies.begin(), frequencies.end());
    }

    double hartree_to_kj_mol(double hartree)
    {
        return hartree * HARTREE_TO_KJ_MOL;
    }

    double hartree_to_ev(double hartree)
    {
        return hartree * HARTREE_TO_EV;
    }

    bool validate_temperature(double temp)
    {
        return temp > 0.0 && temp < 10000.0;  // Reasonable temperature range
    }

    bool validate_concentration(double conc)
    {
        return conc > 0.0 && conc < 1000.0;  // Reasonable concentration range
    }

    bool validate_energy_data(const HighLevelEnergyData& data)
    {
        return !data.filename.empty() && data.final_scf_high != 0.0 && data.temperature > 0.0;
    }

}  // namespace HighLevelEnergyUtils

// =============================================================================
// Enhanced Parallel Processing Implementation
// =============================================================================

std::vector<HighLevelEnergyData>
HighLevelEnergyCalculator::process_files_with_thread_pool(const std::vector<std::string>& files,
                                                          unsigned int                    thread_count,
                                                          size_t                          memory_limit_mb,
                                                          bool                            quiet)
{

    std::vector<HighLevelEnergyData> results;

    if (files.empty())
    {
        return results;
    }

    // Clear file cache at the beginning for fresh processing
    g_file_cache.clear();

    // Enhanced memory limit calculation
    if (memory_limit_mb == 0)
    {
        if (has_context_ && context_->memory_monitor)
        {
            // Get actual available memory instead of max usage
            size_t system_memory_mb = 4096;  // Default fallback
            try
            {
// Try to get system memory info
#ifdef _WIN32
                MEMORYSTATUSEX statex;
                statex.dwLength = sizeof(statex);
                if (GlobalMemoryStatusEx(&statex))
                {
                    system_memory_mb = static_cast<size_t>(statex.ullAvailPhys / (1024 * 1024));
                }
#else
                long pages     = sysconf(_SC_PHYS_PAGES);
                long page_size = sysconf(_SC_PAGE_SIZE);
                if (pages > 0 && page_size > 0)
                {
                    system_memory_mb = (pages * page_size) / (1024 * 1024);
                }
#endif
            }
            catch (...)
            {
                // Use fallback value
            }

            // Use 30-60% of system memory based on thread count
            double memory_fraction = 0.3 + (thread_count / 48.0) * 0.3;  // 30% to 60%
            memory_limit_mb        = static_cast<size_t>(system_memory_mb * memory_fraction);

            if (!quiet)
            {
                std::cout << "Auto-detected memory limit: " << memory_limit_mb << " MB" << std::endl;
            }
        }
        else
        {
            memory_limit_mb = 2048;  // Default 2GB
        }
    }

    // Optimize thread count
    thread_count = std::min(thread_count, std::thread::hardware_concurrency());
    thread_count = std::max(1u, thread_count);  // Minimum 1 thread

    if (!quiet)
    {
        std::cout << "Processing " << files.size() << " files with " << thread_count
                  << " threads (Memory limit: " << memory_limit_mb << " MB)" << std::endl;
    }

    // Optimized work-stealing approach instead of thread pool for better performance
    std::atomic<size_t> file_index{0};
    results.resize(files.size());
    std::atomic<size_t> progress_counter{0};
    auto                start_time = std::chrono::steady_clock::now();

    // Worker function with work stealing
    auto worker = [&]() {
        while (true)
        {
            size_t idx = file_index.fetch_add(1);
            if (idx >= files.size())
                break;

            try
            {
                // Process with cached file reading
                results[idx] = calculate_high_level_energy(files[idx]);
                progress_counter.fetch_add(1);
            }
            catch (const std::exception& e)
            {
                if (has_context_ && context_->error_collector)
                {
                    context_->error_collector->add_error("Failed to process " + files[idx] + ": " + e.what());
                }
                results[idx]        = HighLevelEnergyData(files[idx]);
                results[idx].status = "ERROR";
                progress_counter.fetch_add(1);
            }
        }
    };

    // Start progress monitor thread
    std::atomic<bool> should_stop{false};
    std::thread       monitor_thread;
    if (!quiet && files.size() > 5)
    {
        monitor_thread = std::thread([&]() {
            while (!should_stop.load() && progress_counter.load() < files.size())
            {
                monitor_progress(files.size(), progress_counter, start_time, quiet);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    }

    // Launch worker threads
    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    for (unsigned i = 0; i < thread_count; ++i)
    {
        workers.emplace_back(worker);
    }

    // Wait for all workers to complete
    for (auto& w : workers)
    {
        if (w.joinable())
        {
            w.join();
        }
    }

    // Remove any failed results (empty entries)
    results.erase(std::remove_if(results.begin(),
                                 results.end(),
                                 [](const HighLevelEnergyData& data) {
                                     return data.filename.empty();
                                 }),
                  results.end());

    // Stop progress monitor
    should_stop.store(true);
    if (monitor_thread.joinable())
    {
        monitor_thread.join();
    }

// Sort results using parallel sort if available
#ifdef __cpp_lib_execution
    if (results.size() > 100)
    {
    #if HAS_EXECUTION_POLICIES
        std::sort(std::execution::par,
                  results.begin(),
                  results.end(),
                  [this](const HighLevelEnergyData& a, const HighLevelEnergyData& b) {
                      return compare_results(a, b, sort_column_);
                  });
    #else
        // Fallback to sequential sort if parallel execution policies are not available
        std::sort(results.begin(), results.end(), [this](const HighLevelEnergyData& a, const HighLevelEnergyData& b) {
            return compare_results(a, b, sort_column_);
        });
    #endif
    }
    else
    {
        std::sort(results.begin(), results.end(), [this](const HighLevelEnergyData& a, const HighLevelEnergyData& b) {
            return compare_results(a, b, sort_column_);
        });
    }
#else
    std::sort(results.begin(), results.end(), [this](const HighLevelEnergyData& a, const HighLevelEnergyData& b) {
        return compare_results(a, b, sort_column_);
    });
#endif

    if (!quiet)
    {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "Completed processing " << results.size() << " files in " << duration.count() << " ms"
                  << std::endl;
    }

    return results;
}

std::unordered_map<std::string, std::string>
HighLevelEnergyCalculator::batch_read_files(const std::vector<std::string>& filenames)
{

    std::unordered_map<std::string, std::string> file_contents;

    // Pre-allocate with expected size
    file_contents.reserve(filenames.size());

    for (const auto& filename : filenames)
    {
        try
        {
            // Check file size first
            auto file_size = std::filesystem::file_size(filename);
            if (file_size > 500 * 1024 * 1024)
            {  // Skip files > 500MB
                if (has_context_ && context_->error_collector)
                {
                    context_->error_collector->add_warning("Skipping large file: " + filename);
                }
                continue;
            }

            // Use the enhanced safe_read_file method
            std::string content = has_context_ ? safe_read_file(filename) : read_file_content(filename);
            if (!content.empty())
            {
                file_contents[filename] = std::move(content);
            }
        }
        catch (const std::exception& e)
        {
            if (has_context_ && context_->error_collector)
            {
                context_->error_collector->add_error("Failed to read " + filename + ": " + e.what());
            }
        }
    }

    return file_contents;
}

HighLevelEnergyData HighLevelEnergyCalculator::process_single_file_enhanced(const std::string& filename,
                                                                            const std::string& file_content)
{

    try
    {
        // If content is provided, we can avoid duplicate file reading
        if (!file_content.empty())
        {
            // This would require modifying calculate_high_level_energy to accept content
            // For now, fall back to standard processing
            return calculate_high_level_energy(filename);
        }
        else
        {
            return calculate_high_level_energy(filename);
        }
    }
    catch (const std::exception& e)
    {
        if (has_context_ && context_->error_collector)
        {
            context_->error_collector->add_error("Enhanced processing failed for " + filename + ": " + e.what());
        }

        HighLevelEnergyData error_data(filename);
        error_data.status = "ERROR";
        return error_data;
    }
}

ThreadPool& HighLevelEnergyCalculator::get_thread_pool(unsigned int thread_count)
{
    if (!thread_pool_ || thread_pool_->is_shutting_down())
    {
        // Adjust thread count for cluster safety
        if (has_context_ && context_->job_resources.scheduler_type != SchedulerType::NONE)
        {
            // More conservative on clusters - use half of allocated CPUs
            thread_count = std::min(thread_count, context_->job_resources.allocated_cpus / 2);
            thread_count = std::max(1u, thread_count);
        }

        thread_pool_ = std::make_unique<ThreadPool>(thread_count);
    }
    return *thread_pool_;
}
