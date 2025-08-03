#include "high_level_energy.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <filesystem>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <exception>

// Basic constructor
HighLevelEnergyCalculator::HighLevelEnergyCalculator(double temp, double concentration_m, int sort_column, bool is_au_format)
    : temperature_(temp), concentration_m_(concentration_m), sort_column_(sort_column), is_au_format_(is_au_format), context_(nullptr), has_context_(false) {
    concentration_mol_m3_ = concentration_m * 1000.0;
}

// Enhanced constructor with processing context
HighLevelEnergyCalculator::HighLevelEnergyCalculator(std::shared_ptr<ProcessingContext> context,
                                                   double temp, double concentration_m, int sort_column, bool is_au_format)
    : temperature_(temp), concentration_m_(concentration_m), sort_column_(sort_column), is_au_format_(is_au_format), context_(context), has_context_(true) {
    concentration_mol_m3_ = concentration_m * 1000.0;

    if (context_) {
        // Validate the processing context
        if (!validate_processing_context()) {
            context_->error_collector->add_warning("Processing context validation failed, falling back to basic mode");
            has_context_ = false;
        }
    } else {
        has_context_ = false;
    }
}

// Main calculation function
HighLevelEnergyData HighLevelEnergyCalculator::calculate_high_level_energy(const std::string& high_level_file) {
    HighLevelEnergyData data(high_level_file);

    try {
        // Memory guard for large calculations
        struct MemoryGuard {
            std::shared_ptr<MemoryMonitor> monitor;
            size_t bytes;
            MemoryGuard(std::shared_ptr<MemoryMonitor> m, size_t b) : monitor(m), bytes(b) {
                if (monitor) monitor->add_usage(bytes);
            }
            ~MemoryGuard() {
                if (monitor) monitor->remove_usage(bytes);
            }
        };

        std::unique_ptr<MemoryGuard> memory_guard;
        if (has_context_ && context_->memory_monitor) {
            if (!context_->memory_monitor->can_allocate(10 * 1024 * 1024)) { // 10MB safety check
                throw std::runtime_error("Insufficient memory to process " + high_level_file);
            }
            memory_guard = std::make_unique<MemoryGuard>(context_->memory_monitor, 10 * 1024 * 1024);
        }

        // Extract high-level electronic energies from current directory file
        data.scf_high = extract_value_from_file(high_level_file, "SCF Done", 5, -1);
        data.scf_td_high = extract_value_from_file(high_level_file, "Total Energy, E\\(CIS", 5, -1, false);
        data.scf_equi_high = extract_value_from_file(high_level_file, "After PCM corrections, the energy is", 7, -1, false);
        data.scf_clr_high = extract_value_from_file(high_level_file, "Total energy after correction", 6, -1, false);

        // Determine final high-level electronic energy (priority order from bash script)
        if (data.scf_equi_high != 0.0) {
            data.final_scf_high = data.scf_equi_high;
        } else if (data.scf_clr_high != 0.0) {
            data.final_scf_high = data.scf_clr_high;
        } else if (data.scf_td_high != 0.0) {
            data.final_scf_high = data.scf_td_high;
        } else {
            data.final_scf_high = data.scf_high;
        }

        // Extract low-level thermal data from parent directory
        std::string parent_file = get_parent_file(high_level_file);
        if (!extract_low_level_thermal_data(parent_file, data)) {
            throw std::runtime_error("Failed to extract thermal data from " + parent_file);
        }

        // Calculate derived quantities
        data.tc_only = calculate_thermal_only(data.tc_energy, data.zpe);
        data.ts_value = data.tc_enthalpy - data.tc_gibbs;

        // Calculate final thermodynamic quantities
        data.enthalpy_hartree = data.final_scf_high + data.tc_enthalpy;
        data.gibbs_hartree = data.final_scf_high + data.tc_gibbs;

        // Check for SCRF and apply phase correction
        std::string file_content = has_context_ ? safe_read_file(high_level_file) : read_file_content(high_level_file);
        data.has_scrf = (file_content.find("scrf") != std::string::npos);

        if (data.has_scrf) {
            data.phase_correction = calculate_phase_correction(data.temperature, concentration_mol_m3_);
            data.gibbs_hartree_corrected = data.gibbs_hartree + data.phase_correction;
            data.phase_corr_applied = true;
        } else {
            data.gibbs_hartree_corrected = data.gibbs_hartree;
            data.phase_corr_applied = false;
        }

        // Convert to other units
        data.gibbs_kj_mol = data.gibbs_hartree_corrected * HARTREE_TO_KJ_MOL;
        data.gibbs_ev = data.gibbs_hartree_corrected * HARTREE_TO_EV;

        // Extract additional data
        data.lowest_frequency = extract_lowest_frequency(parent_file);
        data.status = determine_job_status(high_level_file);

        // Validate final data
        if (has_context_ && !HighLevelEnergyUtils::validate_energy_data(data)) {
            if (context_->error_collector) {
                context_->error_collector->add_warning("Energy data validation failed for " + high_level_file);
            }
        }

    } catch (const std::exception& e) {
        if (has_context_ && context_->error_collector) {
            context_->error_collector->add_error("Error processing " + high_level_file + ": " + e.what());
        } else {
            std::cerr << "Error processing " << high_level_file << ": " << e.what() << std::endl;
        }
        data.status = "ERROR";
    }

    return data;
}

// Process entire directory
std::vector<HighLevelEnergyData> HighLevelEnergyCalculator::process_directory(const std::string& extension) {
    std::vector<HighLevelEnergyData> results;

    try {
        auto log_files = HighLevelEnergyUtils::find_log_files(".");

        for (const auto& file : log_files) {
            if (file.find(extension) != std::string::npos) {
                auto data = calculate_high_level_energy(file);
                results.push_back(data);
            }
        }

        // Sort by specified column
        std::sort(results.begin(), results.end(),
                 [this](const HighLevelEnergyData& a, const HighLevelEnergyData& b) {
                     return compare_results(a, b, sort_column_);
                 });

    } catch (const std::exception& e) {
        if (has_context_ && context_->error_collector) {
            context_->error_collector->add_error("Error processing directory: " + std::string(e.what()));
        } else {
            std::cerr << "Error processing directory: " << e.what() << std::endl;
        }
    }

    return results;
}

std::vector<HighLevelEnergyData> HighLevelEnergyCalculator::process_directory_parallel(const std::string& extension,
                                                                                      unsigned int thread_count,
                                                                                      bool quiet) {
    std::vector<HighLevelEnergyData> results;

    try {
        // Find all log files
        auto log_files = HighLevelEnergyUtils::find_log_files(".");

        // Filter by extension
        std::vector<std::string> filtered_files;
        std::copy_if(log_files.begin(), log_files.end(), std::back_inserter(filtered_files),
                    [&extension](const std::string& file) {
                        return file.find(extension) != std::string::npos;
                    });

        if (filtered_files.empty()) {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_warning("No files found with extension: " + extension);
            }
            return results;
        }

        // Validate and prepare files
        auto validated_files = validate_and_prepare_files(filtered_files);

        if (validated_files.empty()) {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_error("No valid files found for processing");
            }
            return results;
        }

        // Determine optimal thread count
        if (thread_count == 0) {
            size_t available_memory = has_context_ ? context_->memory_monitor->get_max_usage() / (1024 * 1024) : 1024;
            thread_count = calculate_optimal_threads(validated_files.size(), available_memory);
        }

        // Process files in parallel
        return process_files_with_limits(validated_files, thread_count, 0, quiet);

    } catch (const std::exception& e) {
        if (has_context_ && context_->error_collector) {
            context_->error_collector->add_error("Error in parallel processing: " + std::string(e.what()));
        } else {
            std::cerr << "Error in parallel processing: " << e.what() << std::endl;
        }
        return results;
    }
}

std::vector<HighLevelEnergyData> HighLevelEnergyCalculator::process_files_with_limits(
    const std::vector<std::string>& files,
    unsigned int thread_count,
    size_t memory_limit_mb,
    bool quiet) {

    std::vector<HighLevelEnergyData> results;

    if (files.empty()) {
        return results;
    }

    // Adjust memory limit if needed
    if (memory_limit_mb == 0 && has_context_) {
        memory_limit_mb = context_->memory_monitor->get_max_usage() / (1024 * 1024);
    } else if (memory_limit_mb == 0) {
        memory_limit_mb = 512; // Default 512MB
    }

    // Ensure we don't exceed system capabilities
    thread_count = std::min(thread_count, std::thread::hardware_concurrency());
    thread_count = std::min(thread_count, static_cast<unsigned int>(files.size()));

    if (thread_count <= 1) {
        // Fall back to sequential processing
        for (const auto& file : files) {
            try {
                auto data = calculate_high_level_energy(file);
                results.push_back(data);
            } catch (const std::exception& e) {
                if (has_context_ && context_->error_collector) {
                    context_->error_collector->add_error("Failed to process " + file + ": " + e.what());
                }
            }
        }
    } else {
        // Parallel processing
        results.resize(files.size());
        std::vector<std::thread> workers;
        std::mutex results_mutex;
        std::atomic<size_t> progress_counter{0};
        std::atomic<bool> should_stop{false};

        auto start_time = std::chrono::steady_clock::now();

        // Launch progress monitor
        std::thread monitor_thread;
        if (thread_count > 1) { // Only show progress for parallel processing
            monitor_thread = std::thread([this, &progress_counter, &should_stop, start_time, files, quiet]() {
                while (!should_stop.load() && progress_counter.load() < files.size()) {
                    monitor_progress(files.size(), progress_counter, start_time, quiet);
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            });
        }

        // Launch worker threads
        size_t files_per_thread = files.size() / thread_count;
        size_t remaining_files = files.size() % thread_count;

        for (unsigned int i = 0; i < thread_count; ++i) {
            size_t start_index = i * files_per_thread;
            size_t end_index = start_index + files_per_thread;

            // Distribute remaining files
            if (i < remaining_files) {
                end_index += 1;
                start_index += i;
            } else {
                start_index += remaining_files;
                end_index += remaining_files;
            }

            if (start_index < files.size()) {
                workers.emplace_back(&HighLevelEnergyCalculator::process_files_worker, this,
                                   std::cref(files), start_index,
                                   std::min(end_index, files.size()),
                                   std::ref(results), std::ref(results_mutex),
                                   std::ref(progress_counter));
            }
        }

        // Wait for all workers to complete
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        // Stop progress monitor
        should_stop.store(true);
        if (monitor_thread.joinable()) {
            monitor_thread.join();
        }

        // Remove any failed results (empty entries)
        results.erase(std::remove_if(results.begin(), results.end(),
                                   [](const HighLevelEnergyData& data) {
                                       return data.filename.empty();
                                   }), results.end());
    }

    // Sort results by specified column
    std::sort(results.begin(), results.end(),
             [this](const HighLevelEnergyData& a, const HighLevelEnergyData& b) {
                 return compare_results(a, b, sort_column_);
             });

    return results;
}

// Print Gibbs-focused format (first bash script)
void HighLevelEnergyCalculator::print_gibbs_format(const std::vector<HighLevelEnergyData>& results, bool quiet, std::ostream* output_file) {
    // Use dynamic formatting by default
    print_gibbs_format_dynamic(results, quiet, output_file);
}

// Print components breakdown format (second bash script)
void HighLevelEnergyCalculator::print_components_format(const std::vector<HighLevelEnergyData>& results, bool quiet, std::ostream* output_file) {
    // Use dynamic formatting by default
    print_components_format_dynamic(results, quiet, output_file);
}

// Print Gibbs-focused CSV format (high-kj command)
void HighLevelEnergyCalculator::print_gibbs_csv_format(const std::vector<HighLevelEnergyData>& results, bool quiet, std::ostream* output_file) {
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file) {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    if (!quiet && !results.empty()) {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    // CSV header for Gibbs format (matching first bash script output)
    out << "Output name,G kJ/mol,G a.u,G eV,LowFQ,Status,PhCorr\n";

    for (const auto& data : results) {
        std::string formatted_name = format_filename(data.filename, 52);
        std::string phase_corr = data.phase_corr_applied ? "YES" : "NO";

        out << "\"" << formatted_name << "\","
            << std::fixed << std::setprecision(6) << data.gibbs_kj_mol << ","
            << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected << ","
            << std::fixed << std::setprecision(6) << data.gibbs_ev << ","
            << std::fixed << std::setprecision(2) << data.lowest_frequency << ","
            << data.status << ","
            << phase_corr << "\n";
    }
}

// Print components breakdown CSV format (high-au command)
void HighLevelEnergyCalculator::print_components_csv_format(const std::vector<HighLevelEnergyData>& results, bool quiet, std::ostream* output_file) {
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file) {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    if (!quiet && !results.empty()) {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    // CSV header for components format (matching second bash script output)
    out << "Output name,E high a.u,E low a.u,ZPE a.u,TC a.u,TS a.u,H a.u,G a.u,LowFQ,PhaseCorr\n";

    for (const auto& data : results) {
        std::string formatted_name = format_filename(data.filename, 53);
        std::string phase_corr = data.phase_corr_applied ? "YES" : "NO";

        out << "\"" << formatted_name << "\","
            << std::fixed << std::setprecision(6) << data.final_scf_high << ","
            << std::fixed << std::setprecision(6) << data.final_scf_low << ","
            << std::fixed << std::setprecision(6) << data.zpe << ","
            << std::fixed << std::setprecision(6) << data.tc_only << ","
            << std::fixed << std::setprecision(6) << data.ts_value << ","
            << std::fixed << std::setprecision(6) << data.enthalpy_hartree << ","
            << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected << ","
            << std::fixed << std::setprecision(2) << data.lowest_frequency << ","
            << phase_corr << "\n";
    }
}

// Helper function implementations
double HighLevelEnergyCalculator::extract_value_from_file(const std::string& filename,
                                                         const std::string& pattern,
                                                         int field_index,
                                                         int occurrence,
                                                         bool warn_if_missing) {
    try {
        // Use enhanced file reading if context is available
        std::unique_ptr<FileHandleManager::FileGuard> file_guard;
        if (has_context_ && context_->file_manager) {
            file_guard = std::make_unique<FileHandleManager::FileGuard>(context_->file_manager->acquire());
            if (!file_guard->is_acquired()) {
                if (context_->error_collector) {
                    context_->error_collector->add_warning("No file handles available for: " + filename);
                }
                return 0.0;
            }
        }

        std::ifstream file(filename);
        if (!file.is_open()) {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_warning("Cannot open file: " + filename);
            }
            return 0.0;
        }

        std::vector<std::string> matches;
        std::string line;
        std::regex regex_pattern(pattern);

        while (std::getline(file, line)) {
            if (std::regex_search(line, regex_pattern)) {
                matches.push_back(line);
            }
        }

        if (matches.empty()) {
            if (warn_if_missing && has_context_ && context_->error_collector) {
                context_->error_collector->add_warning("Pattern '" + pattern + "' not found in: " + filename);
            }
            return 0.0;
        }

        // Get the specified occurrence (-1 means last)
        std::string target_line;
        if (occurrence == -1) {
            target_line = matches.back();
        } else if (occurrence > 0 && occurrence <= static_cast<int>(matches.size())) {
            target_line = matches[occurrence - 1];
        } else {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_warning("Invalid occurrence " + std::to_string(occurrence) +
                                                      " for pattern '" + pattern + "' in: " + filename);
            }
            return 0.0;
        }

        // Extract the field
        std::istringstream iss(target_line);
        std::string field;
        for (int i = 0; i < field_index && iss >> field; ++i) {
            // Continue reading until we reach the desired field
        }

        if (!field.empty()) {
            return safe_parse_energy(field, filename + " (pattern: " + pattern + ")");
        }

    } catch (const std::exception& e) {
        if (has_context_ && context_->error_collector) {
            context_->error_collector->add_error("Error extracting value from " + filename + ": " + e.what());
        }
    }

    return 0.0;
}

bool HighLevelEnergyCalculator::extract_low_level_thermal_data(const std::string& parent_file,
                                                              HighLevelEnergyData& data) {
    try {
        // Check if parent file exists
        if (!file_exists(parent_file)) {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_error("Parent file not found: " + parent_file);
            }
            return false;
        }

        // Extract thermal corrections from parent directory file
        data.scf_low = extract_value_from_file(parent_file, "SCF Done", 5, -1);
        data.scf_td_low = extract_value_from_file(parent_file, "Total Energy, E\\(CIS", 5, -1, false);
        data.zpe = extract_value_from_file(parent_file, "Zero-point correction", 3, -1);
        data.tc_enthalpy = extract_value_from_file(parent_file, "Thermal correction to Enthalpy", 5, -1);
        data.tc_gibbs = extract_value_from_file(parent_file, "Thermal correction to Gibbs Free Energy", 7, -1);
        data.tc_energy = extract_value_from_file(parent_file, "Thermal correction to Energy", 5, -1);
        data.entropy_total = extract_value_from_file(parent_file, "Total\\s+S", 2, -1, false);

        // Validate extracted thermal data
        if (data.zpe == 0.0 && data.tc_enthalpy == 0.0 && data.tc_gibbs == 0.0) {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_warning("No thermal corrections found in: " + parent_file);
            }
        }

        // Determine final low-level electronic energy
        if (data.scf_td_low != 0.0) {
            data.final_scf_low = data.scf_td_low;
        } else {
            data.final_scf_low = data.scf_low;
        }

        // Extract temperature from parent file
        double temp = extract_value_from_file(parent_file, "Kelvin\\.\\s+Pressure", 2, -1, false);
        if (temp > 0.0 && HighLevelEnergyUtils::validate_temperature(temp)) {
            data.temperature = temp;
        } else {
            data.temperature = temperature_;
            if (has_context_ && context_->error_collector && temp > 0.0) {
                context_->error_collector->add_warning("Invalid temperature (" + std::to_string(temp) +
                                                      ") found in " + parent_file + ", using default");
            }
        }

        return true;

    } catch (const std::exception& e) {
        if (has_context_ && context_->error_collector) {
            context_->error_collector->add_error("Failed to extract thermal data from " + parent_file + ": " + e.what());
        }
        return false;
    }
}

double HighLevelEnergyCalculator::calculate_phase_correction(double temp, double concentration_mol_m3) {
    // Delta_G_corr = RT*ln(C*RT/Po) * conversion_factor
    double rt = R_CONSTANT * temp;
    double ratio = concentration_mol_m3 * rt / PO_CONSTANT;
    return rt * std::log(ratio) * PHASE_CORR_FACTOR / 1000.0;
}

double HighLevelEnergyCalculator::calculate_thermal_only(double tc_with_zpe, double zpe) {
    return tc_with_zpe - zpe;
}

double HighLevelEnergyCalculator::calculate_ts_value(double entropy_total, double temp) {
    // TS = H_corr - G_corr (difference between enthalpy and Gibbs corrections)
    // This is calculated directly in the main function now
    return entropy_total * temp / 627509.6080305927;
}

double HighLevelEnergyCalculator::extract_lowest_frequency(const std::string& parent_file) {
    try {
        std::string content = read_file_content(parent_file);
        auto frequencies = HighLevelEnergyUtils::extract_frequencies(content);
        return HighLevelEnergyUtils::find_lowest_frequency(frequencies);
    } catch (const std::exception& e) {
        return 0.0;
    }
}

std::string HighLevelEnergyCalculator::determine_job_status(const std::string& filename) {
    try {
        std::string tail_content = read_file_tail(filename, 10);

        if (tail_content.find("Normal") != std::string::npos) {
            return "DONE";
        }

        // Check for error patterns
        bool has_error = false;
        bool has_error_on = false;

        std::istringstream iss(tail_content);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("Error") != std::string::npos) {
                has_error = true;
                if (line.find("Error on") != std::string::npos) {
                    has_error_on = true;
                }
            }
        }

        if (has_error && !has_error_on) {
            return "ERROR";
        }

        return "UNDONE";

    } catch (const std::exception& e) {
        return "UNKNOWN";
    }
}

std::string HighLevelEnergyCalculator::get_parent_file(const std::string& high_level_file) {
    return "../" + high_level_file;
}

bool HighLevelEnergyCalculator::file_exists(const std::string& filename) {
    return std::filesystem::exists(filename);
}

std::string HighLevelEnergyCalculator::read_file_content(const std::string& filename) {
    // Use enhanced file reading if context is available
    if (has_context_) {
        return safe_read_file(filename, 100); // 100MB limit
    }

    // Fallback to basic file reading
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string HighLevelEnergyCalculator::read_file_tail(const std::string& filename, int lines) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::vector<std::string> all_lines;
    std::string line;
    while (std::getline(file, line)) {
        all_lines.push_back(line);
    }

    std::ostringstream result;
    size_t start_index = (all_lines.size() > static_cast<size_t>(lines)) ?
                        all_lines.size() - lines : 0;

    for (size_t i = start_index; i < all_lines.size(); ++i) {
        result << all_lines[i] << "\n";
    }

    return result.str();
}

void HighLevelEnergyCalculator::print_gibbs_header(std::ostream* output_file) {
    std::ostream& out = output_file ? *output_file : std::cout;

    out << std::setw(52) << "Output name"
        << std::setw(18) << "G kJ/mol"
        << std::setw(15) << "G a.u"
        << std::setw(15) << "G eV"
        << std::setw(12) << "LowFQ"
        << std::setw(8) << "Status"
        << std::setw(8) << "PhCorr" << std::endl;

    out << std::setw(52) << "-----------"
        << std::setw(18) << "----------"
        << std::setw(15) << "---------"
        << std::setw(15) << "----------"
        << std::setw(12) << "-----"
        << std::setw(8) << "------"
        << std::setw(8) << "------" << std::endl;
}

void HighLevelEnergyCalculator::print_components_header(std::ostream* output_file) {
    std::ostream& out = output_file ? *output_file : std::cout;

    out << std::setw(53) << "Output name"
        << std::setw(15) << "E high a.u"
        << std::setw(15) << "E low a.u"
        << std::setw(10) << "ZPE a.u"
        << std::setw(10) << "TC a.u"
        << std::setw(10) << "TS a.u"
        << std::setw(15) << "H a.u"
        << std::setw(15) << "G a.u"
        << std::setw(10) << "LowFQ"
        << std::setw(11) << "PhaseCorr" << std::endl;

    out << std::setw(53) << "-----------"
        << std::setw(15) << "----------"
        << std::setw(15) << "----------"
        << std::setw(10) << "------"
        << std::setw(10) << "-------"
        << std::setw(10) << "------"
        << std::setw(15) << "------"
        << std::setw(15) << "------"
        << std::setw(10) << "------"
        << std::setw(11) << "------" << std::endl;
}

std::string HighLevelEnergyCalculator::format_filename(const std::string& filename, int max_length) {
    if (static_cast<int>(filename.length()) <= max_length) {
        return filename;
    }

    // Truncate from the beginning, keeping the end
    int start_pos = filename.length() - max_length;
    return filename.substr(start_pos);
}

void HighLevelEnergyCalculator::print_summary_info(const std::string& last_parent_file, std::ostream* output_file) {
    std::ostream& out = output_file ? *output_file : std::cout;

    try {
        double last_temp = extract_value_from_file(last_parent_file, "Kelvin\\.\\s+Pressure", 2, -1, false);
        if (last_temp == 0.0) last_temp = temperature_;

        double last_phase_corr = calculate_phase_correction(last_temp, concentration_mol_m3_);

        out << "Temperature in " << last_parent_file << ": " << std::fixed << std::setprecision(3)
            << last_temp << " K. Make sure that temperature has been used in your input." << std::endl;
        out << "The concentration for phase correction: " << std::fixed << std::setprecision(0)
            << concentration_m_ << " M or " << concentration_mol_m3_ << " mol/m3" << std::endl;
        out << "Last Gibbs free correction for phase changing from 1 atm to 1 M: "
            << std::fixed << std::setprecision(6) << last_phase_corr << " au" << std::endl;
    } catch (const std::exception& e) {
        out << "Error printing summary: " << e.what() << std::endl;
    }
}

// Dynamic formatting implementations
std::vector<int> HighLevelEnergyCalculator::calculate_gibbs_column_widths(const std::vector<HighLevelEnergyData>& results) {
    // Column order: filename, G kJ/mol, G a.u, G eV, LowFQ, Status, PhCorr
    std::vector<int> widths(7);

    // Initialize with header lengths + 3 spaces padding
    widths[0] = std::max(11, 52); // "Output name" or minimum filename width
    widths[1] = std::max(8, 15);  // "G kJ/mol"
    widths[2] = std::max(6, 12);  // "G a.u"
    widths[3] = std::max(4, 12);  // "G eV"
    widths[4] = std::max(5, 10);  // "LowFQ"
    widths[5] = std::max(6, 8);   // "Status"
    widths[6] = std::max(6, 8);   // "PhCorr"

    for (const auto& data : results) {
        // Check filename length (limited to reasonable max)
        std::string formatted_name = format_filename(data.filename, 70);
        widths[0] = std::max(widths[0], static_cast<int>(formatted_name.length()) + 3);

        // Check G kJ/mol length
        std::ostringstream ss1;
        ss1 << std::fixed << std::setprecision(6) << data.gibbs_kj_mol;
        widths[1] = std::max(widths[1], static_cast<int>(ss1.str().length()) + 3);

        // Check G a.u length
        std::ostringstream ss2;
        ss2 << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected;
        widths[2] = std::max(widths[2], static_cast<int>(ss2.str().length()) + 3);

        // Check G eV length
        std::ostringstream ss3;
        ss3 << std::fixed << std::setprecision(6) << data.gibbs_ev;
        widths[3] = std::max(widths[3], static_cast<int>(ss3.str().length()) + 3);

        // Check LowFQ length
        std::ostringstream ss4;
        ss4 << std::fixed << std::setprecision(4) << data.lowest_frequency;
        widths[4] = std::max(widths[4], static_cast<int>(ss4.str().length()) + 3);

        // Check Status length
        widths[5] = std::max(widths[5], static_cast<int>(data.status.length()) + 3);

        // Check PhCorr length
        std::string phase_corr = data.phase_corr_applied ? "YES" : "NO";
        widths[6] = std::max(widths[6], static_cast<int>(phase_corr.length()) + 3);
    }

    return widths;
}

std::vector<int> HighLevelEnergyCalculator::calculate_components_column_widths(const std::vector<HighLevelEnergyData>& results) {
    // Column order: filename, E high a.u, E low a.u, ZPE a.u, TC a.u, TS a.u, H a.u, G a.u, LowFQ, PhaseCorr
    std::vector<int> widths(10);

    // Initialize with header lengths + 3 spaces padding
    widths[0] = std::max(11, 52); // "Output name"
    widths[1] = std::max(10, 15); // "E high a.u"
    widths[2] = std::max(9, 15);  // "E low a.u"
    widths[3] = std::max(7, 10);  // "ZPE a.u"
    widths[4] = std::max(6, 10);  // "TC a.u"
    widths[5] = std::max(6, 10);  // "TS a.u"
    widths[6] = std::max(5, 15);  // "H a.u"
    widths[7] = std::max(5, 15);  // "G a.u"
    widths[8] = std::max(5, 10);  // "LowFQ"
    widths[9] = std::max(9, 11);  // "PhaseCorr"

    for (const auto& data : results) {
        // Check filename length (limited to reasonable max)
        std::string formatted_name = format_filename(data.filename, 70);
        widths[0] = std::max(widths[0], static_cast<int>(formatted_name.length()) + 3);

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
        widths[9] = std::max(widths[9], static_cast<int>(phase_corr.length()) + 3);
    }

    return widths;
}

void HighLevelEnergyCalculator::print_gibbs_header_dynamic(const std::vector<int>& column_widths, std::ostream* output_file) {
    std::ostream& out = output_file ? *output_file : std::cout;

    out << std::setw(column_widths[0]) << "Output name"
        << std::setw(column_widths[1]) << "G kJ/mol"
        << std::setw(column_widths[2]) << "G a.u"
        << std::setw(column_widths[3]) << "G eV"
        << std::setw(column_widths[4]) << "LowFQ"
        << std::setw(column_widths[5]) << "Status"
        << std::setw(column_widths[6]) << "PhCorr" << std::endl;

    // Print separator line
    out << std::setw(column_widths[0]) << std::string(column_widths[0]-3, '-')
        << std::setw(column_widths[1]) << std::string(column_widths[1]-3, '-')
        << std::setw(column_widths[2]) << std::string(column_widths[2]-3, '-')
        << std::setw(column_widths[3]) << std::string(column_widths[3]-3, '-')
        << std::setw(column_widths[4]) << std::string(column_widths[4]-3, '-')
        << std::setw(column_widths[5]) << std::string(column_widths[5]-3, '-')
        << std::setw(column_widths[6]) << std::string(column_widths[6]-3, '-') << std::endl;
}

void HighLevelEnergyCalculator::print_components_header_dynamic(const std::vector<int>& column_widths, std::ostream* output_file) {
    std::ostream& out = output_file ? *output_file : std::cout;

    out << std::setw(column_widths[0]) << "Output name"
        << std::setw(column_widths[1]) << "E high a.u"
        << std::setw(column_widths[2]) << "E low a.u"
        << std::setw(column_widths[3]) << "ZPE a.u"
        << std::setw(column_widths[4]) << "TC a.u"
        << std::setw(column_widths[5]) << "TS a.u"
        << std::setw(column_widths[6]) << "H a.u"
        << std::setw(column_widths[7]) << "G a.u"
        << std::setw(column_widths[8]) << "LowFQ"
        << std::setw(column_widths[9]) << "PhaseCorr" << std::endl;

    // Print separator line
    out << std::setw(column_widths[0]) << std::string(column_widths[0]-3, '-')
        << std::setw(column_widths[1]) << std::string(column_widths[1]-3, '-')
        << std::setw(column_widths[2]) << std::string(column_widths[2]-3, '-')
        << std::setw(column_widths[3]) << std::string(column_widths[3]-3, '-')
        << std::setw(column_widths[4]) << std::string(column_widths[4]-3, '-')
        << std::setw(column_widths[5]) << std::string(column_widths[5]-3, '-')
        << std::setw(column_widths[6]) << std::string(column_widths[6]-3, '-')
        << std::setw(column_widths[7]) << std::string(column_widths[7]-3, '-')
        << std::setw(column_widths[8]) << std::string(column_widths[8]-3, '-')
        << std::setw(column_widths[9]) << std::string(column_widths[9]-3, '-') << std::endl;
}

void HighLevelEnergyCalculator::print_gibbs_format_dynamic(const std::vector<HighLevelEnergyData>& results, bool quiet, std::ostream* output_file) {
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file) {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    if (!quiet && !results.empty()) {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    auto column_widths = calculate_gibbs_column_widths(results);
    print_gibbs_header_dynamic(column_widths, output_file);

    for (const auto& data : results) {
        std::string formatted_name = format_filename(data.filename, column_widths[0]-3);
        std::string phase_corr = data.phase_corr_applied ? "YES" : "NO";

        out << std::setw(column_widths[0]) << formatted_name
            << std::setw(column_widths[1]) << std::fixed << std::setprecision(6) << data.gibbs_kj_mol
            << std::setw(column_widths[2]) << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected
            << std::setw(column_widths[3]) << std::fixed << std::setprecision(6) << data.gibbs_ev
            << std::setw(column_widths[4]) << std::fixed << std::setprecision(4) << data.lowest_frequency
            << std::setw(column_widths[5]) << data.status
            << std::setw(column_widths[6]) << phase_corr << std::endl;
    }
}

void HighLevelEnergyCalculator::print_components_format_dynamic(const std::vector<HighLevelEnergyData>& results, bool quiet, std::ostream* output_file) {
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file) {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    if (!quiet && !results.empty()) {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    auto column_widths = calculate_components_column_widths(results);
    print_components_header_dynamic(column_widths, output_file);

    for (const auto& data : results) {
        std::string formatted_name = format_filename(data.filename, column_widths[0]-3);
        std::string phase_corr = data.phase_corr_applied ? "YES" : "NO";

        out << std::setw(column_widths[0]) << formatted_name
            << std::setw(column_widths[1]) << std::fixed << std::setprecision(6) << data.final_scf_high
            << std::setw(column_widths[2]) << std::fixed << std::setprecision(6) << data.final_scf_low
            << std::setw(column_widths[3]) << std::fixed << std::setprecision(6) << data.zpe
            << std::setw(column_widths[4]) << std::fixed << std::setprecision(6) << data.tc_only
            << std::setw(column_widths[5]) << std::fixed << std::setprecision(6) << data.ts_value
            << std::setw(column_widths[6]) << std::fixed << std::setprecision(6) << data.enthalpy_hartree
            << std::setw(column_widths[7]) << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected
            << std::setw(column_widths[8]) << std::fixed << std::setprecision(4) << data.lowest_frequency
            << std::setw(column_widths[9]) << phase_corr << std::endl;
    }
}

// Static formatting functions (for backwards compatibility and consistent alignment)
void HighLevelEnergyCalculator::print_gibbs_format_static(const std::vector<HighLevelEnergyData>& results, bool quiet, std::ostream* output_file) {
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file) {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    if (!quiet && !results.empty()) {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    print_gibbs_header(output_file);

    for (const auto& data : results) {
        std::string formatted_name = format_filename(data.filename, 52);
        std::string phase_corr = data.phase_corr_applied ? "YES" : "NO";

        out << std::setw(52) << formatted_name
            << std::setw(18) << std::fixed << std::setprecision(6) << data.gibbs_kj_mol
            << std::setw(15) << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected
            << std::setw(15) << std::fixed << std::setprecision(6) << data.gibbs_ev
            << std::setw(12) << std::fixed << std::setprecision(4) << data.lowest_frequency
            << std::setw(8) << data.status
            << std::setw(8) << phase_corr << std::endl;
    }
}

void HighLevelEnergyCalculator::print_components_format_static(const std::vector<HighLevelEnergyData>& results, bool quiet, std::ostream* output_file) {
    // In quiet mode, only write to file if output_file is provided
    if (quiet && !output_file) {
        return;
    }

    std::ostream& out = output_file ? *output_file : std::cout;

    if (!quiet && !results.empty()) {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    print_components_header(output_file);

    for (const auto& data : results) {
        std::string formatted_name = format_filename(data.filename, 53);
        std::string phase_corr = data.phase_corr_applied ? "YES" : "NO";

        out << std::setw(53) << formatted_name
            << std::setw(15) << std::fixed << std::setprecision(6) << data.final_scf_high
            << std::setw(15) << std::fixed << std::setprecision(6) << data.final_scf_low
            << std::setw(10) << std::fixed << std::setprecision(6) << data.zpe
            << std::setw(10) << std::fixed << std::setprecision(6) << data.tc_only
            << std::setw(10) << std::fixed << std::setprecision(6) << data.ts_value
            << std::setw(15) << std::fixed << std::setprecision(6) << data.enthalpy_hartree
            << std::setw(15) << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected
            << std::setw(11) << std::fixed << std::setprecision(4) << data.lowest_frequency
            << std::setw(10) << phase_corr << std::endl;
    }
}
// Enhanced worker function for parallel processing
void HighLevelEnergyCalculator::process_files_worker(const std::vector<std::string>& files,
                                                     size_t start_index, size_t end_index,
                                                     std::vector<HighLevelEnergyData>& results,
                                                     std::mutex& results_mutex,
                                                     std::atomic<size_t>& progress_counter) {
    for (size_t i = start_index; i < end_index; ++i) {
        try {
            // Memory check before processing
            if (has_context_ && context_->memory_monitor) {
                if (!context_->memory_monitor->can_allocate(50 * 1024 * 1024)) { // 50MB safety margin
                    if (context_->error_collector) {
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
                if (i < results.size()) {
                    results[i] = data;
                }
            }

        } catch (const std::exception& e) {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_error("Worker failed to process " + files[i] + ": " + e.what());
            }

            // Store empty result to maintain indexing
            {
                std::lock_guard<std::mutex> lock(results_mutex);
                if (i < results.size()) {
                    results[i] = HighLevelEnergyData(files[i]);
                    results[i].status = "ERROR";
                }
            }
        }

        // Update progress
        progress_counter.fetch_add(1);
    }
}

unsigned int HighLevelEnergyCalculator::calculate_optimal_threads(size_t file_count, size_t available_memory) {
    // Basic thread calculation
    unsigned int max_threads = std::thread::hardware_concurrency();
    if (max_threads == 0) max_threads = 4; // Fallback

    // Memory-based limitation (assume ~50MB per thread)
    unsigned int memory_limited_threads = static_cast<unsigned int>(available_memory / 50);

    // File-based limitation
    unsigned int file_limited_threads = static_cast<unsigned int>(std::min(file_count, static_cast<size_t>(max_threads)));

    // Take the minimum of all constraints
    unsigned int optimal = std::min({max_threads, memory_limited_threads, file_limited_threads});

    // Ensure at least 1 thread
    return std::max(1u, optimal);
}

std::vector<std::string> HighLevelEnergyCalculator::validate_and_prepare_files(const std::vector<std::string>& files) {
    std::vector<std::string> validated_files;

    // Get max file size from context or use default
    size_t max_file_size_bytes = 100 * 1024 * 1024; // Default 100MB
    if (has_context_) {
        max_file_size_bytes = context_->max_file_size_mb * 1024 * 1024;
    }

    for (const auto& file : files) {
        try {
            // Check file existence and accessibility
            if (!file_exists(file)) {
                if (has_context_ && context_->error_collector) {
                    context_->error_collector->add_warning("File not found: " + file);
                }
                continue;
            }

            // Check file size using CommandContext limit
            std::filesystem::path file_path(file);
            if (std::filesystem::exists(file_path)) {
                auto file_size = std::filesystem::file_size(file_path);
                if (file_size > max_file_size_bytes) {
                    if (has_context_ && context_->error_collector) {
                        context_->error_collector->add_warning("File too large (" +
                            std::to_string(file_size / (1024*1024)) + "MB > " +
                            std::to_string(max_file_size_bytes / (1024*1024)) + "MB), skipping: " + file);
                    }
                    continue;
                }
            }

            validated_files.push_back(file);

        } catch (const std::exception& e) {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_warning("File validation failed for " + file + ": " + e.what());
            }
        }
    }

    return validated_files;
}

bool HighLevelEnergyCalculator::monitor_progress(size_t total_files,
                                               const std::atomic<size_t>& progress_counter,
                                               std::chrono::steady_clock::time_point start_time,
                                               bool quiet) {
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);

    size_t completed = progress_counter.load();
    double percentage = (static_cast<double>(completed) / total_files) * 100.0;

    // Estimate time remaining
    double files_per_second = completed > 0 ? static_cast<double>(completed) / elapsed.count() : 0.0;
    auto estimated_remaining = files_per_second > 0 ?
                              std::chrono::seconds(static_cast<long>((total_files - completed) / files_per_second)) :
                              std::chrono::seconds(0);
    std::cout << "Estimated time remaining: " << estimated_remaining.count() << " seconds\n";

    // Print progress only if not in quiet mode
    if (!quiet) {
        std::cout << "Processed " << completed << "/" << total_files
                  << " files (" << std::fixed << std::setprecision(1) << percentage << "%)" << std::endl;
    }

    // Memory monitoring
    if (has_context_ && context_->memory_monitor) {
        auto current_usage = context_->memory_monitor->get_current_usage();
        auto max_usage = context_->memory_monitor->get_max_usage();
        if (current_usage > max_usage * 0.9) { // 90% memory usage warning
            if (context_->error_collector) {
                context_->error_collector->add_warning("High memory usage detected: " +
                                                      std::to_string(current_usage / (1024*1024)) + "MB");
            }
        }
    }

    return completed < total_files;
}

std::string HighLevelEnergyCalculator::safe_read_file(const std::string& filename, size_t max_size_mb) {
    std::string content;

    try {
        // Check file size first
        std::filesystem::path file_path(filename);
        if (!std::filesystem::exists(file_path)) {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_error("File not found: " + filename);
            }
            return content;
        }

        auto file_size = std::filesystem::file_size(file_path);

        // Use max_file_size_mb from context if available, otherwise use parameter
        size_t effective_max_size_mb = max_size_mb;
        if (has_context_) {
            effective_max_size_mb = context_->max_file_size_mb;
        }
        size_t max_size_bytes = effective_max_size_mb * 1024 * 1024;

        if (file_size > max_size_bytes) {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_warning("File too large (" + std::to_string(file_size / (1024*1024)) +
                                                      "MB > " + std::to_string(effective_max_size_mb) +
                                                      "MB), truncating: " + filename);
            }
            file_size = max_size_bytes;
        }

        // Memory allocation check
        if (has_context_ && context_->memory_monitor) {
            if (!context_->memory_monitor->can_allocate(file_size)) {
                if (context_->error_collector) {
                    context_->error_collector->add_error("Insufficient memory to read file: " + filename);
                }
                return content;
            }
        }

        // File handle management
        if (has_context_ && context_->file_manager) {
            auto file_guard = context_->file_manager->acquire();
            if (!file_guard.is_acquired()) {
                if (context_->error_collector) {
                    context_->error_collector->add_warning("No file handles available for: " + filename);
                }
                return content;
            }

            std::ifstream file(filename, std::ios::binary);
            if (file.is_open()) {
                content.resize(file_size);
                file.read(&content[0], file_size);
                content.resize(file.gcount()); // Adjust to actual read size
            }
        } else {
            // Fallback without file handle management
            std::ifstream file(filename, std::ios::binary);
            if (file.is_open()) {
                content.resize(file_size);
                file.read(&content[0], file_size);
                content.resize(file.gcount());
            }
        }

    } catch (const std::exception& e) {
        if (has_context_ && context_->error_collector) {
            context_->error_collector->add_error("Failed to read file " + filename + ": " + e.what());
        }
        content.clear();
    }

    return content;
}

double HighLevelEnergyCalculator::safe_parse_energy(const std::string& value_str, const std::string& context_info) {
    try {
        if (value_str.empty()) {
            return 0.0;
        }

        double value = std::stod(value_str);

        // Validate range (reasonable energy values)
        if (std::isnan(value) || std::isinf(value)) {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_warning("Invalid energy value (NaN/Inf) in " + context_info);
            }
            return 0.0;
        }

        // Check reasonable energy range (in Hartree)
        if (std::abs(value) > 10000.0) {
            if (has_context_ && context_->error_collector) {
                context_->error_collector->add_warning("Unusually large energy value (" + std::to_string(value) +
                                                      ") in " + context_info);
            }
        }

        return value;

    } catch (const std::exception& e) {
        if (has_context_ && context_->error_collector) {
            context_->error_collector->add_error("Failed to parse energy '" + value_str + "' in " + context_info +
                                                ": " + e.what());
        }
        return 0.0;
    }
}

bool HighLevelEnergyCalculator::validate_processing_context() {
    if (!context_) {
        return false;
    }

    try {
        // Check memory monitor
        if (!context_->memory_monitor) {
            return false;
        }

        // Check file manager
        if (!context_->file_manager) {
            return false;
        }

        // Check error collector
        if (!context_->error_collector) {
            return false;
        }

        // Test basic functionality
        auto test_guard = context_->file_manager->acquire();
        bool can_allocate = context_->memory_monitor->can_allocate(1024); // 1KB test

        return can_allocate; // If we can allocate, context is likely valid

    } catch (const std::exception&) {
        return false;
    }
}

// Comparison function for sorting based on visual column numbers
bool HighLevelEnergyCalculator::compare_results(const HighLevelEnergyData& a, const HighLevelEnergyData& b, int column) {
    if (is_au_format_) {
        // High-AU format: Output name, E high a.u, E low a.u, ZPE a.u, TC a.u, TS a.u, H a.u, G a.u, LowFQ, PhaseCorr
        switch (column) {
            case 1: // Output name
                return a.filename < b.filename;
            case 2: // E high a.u
                return a.final_scf_high < b.final_scf_high;
            case 3: // E low a.u
                return a.final_scf_low < b.final_scf_low;
            case 4: // ZPE a.u
                return a.zpe < b.zpe;
            case 5: // TC a.u
                return a.tc_only < b.tc_only;
            case 6: // TS a.u
                return a.ts_value < b.ts_value;
            case 7: // H a.u
                return a.enthalpy_hartree < b.enthalpy_hartree;
            case 8: // G a.u
                return a.gibbs_hartree_corrected < b.gibbs_hartree_corrected;
            case 9: // LowFQ
                return a.lowest_frequency < b.lowest_frequency;
            case 10: // PhaseCorr (YES comes before NO)
                return a.phase_corr_applied > b.phase_corr_applied;
            default: // Default to G a.u
                return a.gibbs_hartree_corrected < b.gibbs_hartree_corrected;
        }
    } else {
        // High-KJ format: Output name, G kJ/mol, G a.u, G eV, LowFQ, Status, PhCorr
        switch (column) {
            case 1: // Output name
                return a.filename < b.filename;
            case 2: // G kJ/mol
                return a.gibbs_kj_mol < b.gibbs_kj_mol;
            case 3: // G a.u
                return a.gibbs_hartree_corrected < b.gibbs_hartree_corrected;
            case 4: // G eV
                return a.gibbs_ev < b.gibbs_ev;
            case 5: // LowFQ
                return a.lowest_frequency < b.lowest_frequency;
            case 6: // Status (alphabetical)
                return a.status < b.status;
            case 7: // PhCorr (YES comes before NO)
                return a.phase_corr_applied > b.phase_corr_applied;
            default: // Default to G kJ/mol
                return a.gibbs_kj_mol < b.gibbs_kj_mol;
        }
    }
}

// Utility namespace implementations
namespace HighLevelEnergyUtils {

std::vector<std::string> find_log_files(const std::string& directory) {
    std::vector<std::string> log_files;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".log") {
                log_files.push_back(entry.path().filename().string());
            }
        }

        std::sort(log_files.begin(), log_files.end());
    } catch (const std::exception& e) {
        std::cerr << "Error finding log files: " << e.what() << std::endl;
    }

    return log_files;
}

std::string get_current_directory_name() {
    try {
        return std::filesystem::current_path().filename().string();
    } catch (const std::exception& e) {
        return "unknown";
    }
}

bool is_valid_high_level_directory() {
    // Check if current directory has .log files and parent directory exists
    auto log_files = find_log_files(".");
    bool has_parent = std::filesystem::exists("../");
    return !log_files.empty() && has_parent;
}

double parse_energy_value(const std::string& line, int field_index) {
    std::istringstream iss(line);
    std::string field;
    for (int i = 0; i < field_index && iss >> field; ++i) {
        // Continue reading
    }

    try {
        return std::stod(field);
    } catch (const std::exception& e) {
        return 0.0;
    }
}

std::vector<double> extract_frequencies(const std::string& content) {
    std::vector<double> frequencies;
    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.find("Frequencies") != std::string::npos) {
            // Parse frequencies from line like: "Frequencies --   -26.1161    19.3167    45.8905"
            std::istringstream line_iss(line);
            std::string word;
            bool found_frequencies = false;

            while (line_iss >> word) {
                if (word == "--") {
                    found_frequencies = true;
                    continue;
                }
                if (found_frequencies) {
                    try {
                        double freq = std::stod(word);
                        frequencies.push_back(freq);
                    } catch (const std::exception& e) {
                        // Skip non-numeric words
                    }
                }
            }
        }
    }

    return frequencies;
}

double find_lowest_frequency(const std::vector<double>& frequencies) {
    if (frequencies.empty()) {
        return 0.0;
    }

    // Return the lowest frequency (most negative if any, otherwise smallest positive)
    return *std::min_element(frequencies.begin(), frequencies.end());
}

double hartree_to_kj_mol(double hartree) {
    return hartree * HARTREE_TO_KJ_MOL;
}

double hartree_to_ev(double hartree) {
    return hartree * HARTREE_TO_EV;
}

bool validate_temperature(double temp) {
    return temp > 0.0 && temp < 10000.0; // Reasonable temperature range
}

bool validate_concentration(double conc) {
    return conc > 0.0 && conc < 1000.0; // Reasonable concentration range
}

bool validate_energy_data(const HighLevelEnergyData& data) {
    return !data.filename.empty() &&
           data.final_scf_high != 0.0 &&
           data.temperature > 0.0;
}

} // namespace HighLevelEnergyUtils
