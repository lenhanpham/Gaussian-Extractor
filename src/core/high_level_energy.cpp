#include "high_level_energy.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <filesystem>
#include <vector>

// Constructor
HighLevelEnergyCalculator::HighLevelEnergyCalculator(double temp, double concentration_m)
    : temperature_(temp), concentration_m_(concentration_m) {
    concentration_mol_m3_ = concentration_m * 1000.0;
}

// Main calculation function
HighLevelEnergyData HighLevelEnergyCalculator::calculate_high_level_energy(const std::string& high_level_file) {
    HighLevelEnergyData data(high_level_file);

    try {
        // Extract high-level electronic energies from current directory file
        data.scf_high = extract_value_from_file(high_level_file, "SCF Done", 5, -1);
        data.scf_td_high = extract_value_from_file(high_level_file, "Total Energy, E\\(CIS", 5, -1);
        data.scf_equi_high = extract_value_from_file(high_level_file, "After PCM corrections, the energy is", 7, -1);
        data.scf_clr_high = extract_value_from_file(high_level_file, "Total energy after correction", 6, -1);

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
        std::string file_content = read_file_content(high_level_file);
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

    } catch (const std::exception& e) {
        std::cerr << "Error processing " << high_level_file << ": " << e.what() << std::endl;
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

        // Sort by Gibbs free energy (ascending)
        std::sort(results.begin(), results.end(),
                 [](const HighLevelEnergyData& a, const HighLevelEnergyData& b) {
                     return a.gibbs_hartree_corrected < b.gibbs_hartree_corrected;
                 });

    } catch (const std::exception& e) {
        std::cerr << "Error processing directory: " << e.what() << std::endl;
    }

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

// Helper function implementations
double HighLevelEnergyCalculator::extract_value_from_file(const std::string& filename,
                                                         const std::string& pattern,
                                                         int field_index,
                                                         int occurrence) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
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
            return 0.0;
        }

        // Get the specified occurrence (-1 means last)
        std::string target_line;
        if (occurrence == -1) {
            target_line = matches.back();
        } else if (occurrence > 0 && occurrence <= static_cast<int>(matches.size())) {
            target_line = matches[occurrence - 1];
        } else {
            return 0.0;
        }

        // Extract the field
        std::istringstream iss(target_line);
        std::string field;
        for (int i = 0; i < field_index && iss >> field; ++i) {
            // Continue reading until we reach the desired field
        }

        if (!field.empty()) {
            return std::stod(field);
        }

    } catch (const std::exception& e) {
        // Return 0.0 for any parsing errors
    }

    return 0.0;
}

bool HighLevelEnergyCalculator::extract_low_level_thermal_data(const std::string& parent_file,
                                                              HighLevelEnergyData& data) {
    try {
        // Extract thermal corrections from parent directory file
        data.scf_low = extract_value_from_file(parent_file, "SCF Done", 5, -1);
        data.scf_td_low = extract_value_from_file(parent_file, "Total Energy, E\\(CIS", 5, -1);
        data.zpe = extract_value_from_file(parent_file, "Zero-point correction", 3, -1);
        data.tc_enthalpy = extract_value_from_file(parent_file, "Thermal correction to Enthalpy", 5, -1);
        data.tc_gibbs = extract_value_from_file(parent_file, "Thermal correction to Gibbs Free Energy", 7, -1);
        data.tc_energy = extract_value_from_file(parent_file, "Thermal correction to Energy", 5, -1);
        data.entropy_total = extract_value_from_file(parent_file, "Total\\s+S", 2, -1);

        // Determine final low-level electronic energy
        if (data.scf_td_low != 0.0) {
            data.final_scf_low = data.scf_td_low;
        } else {
            data.final_scf_low = data.scf_low;
        }

        // Extract temperature from parent file
        double temp = extract_value_from_file(parent_file, "Kelvin\\.\\s+Pressure", 2, -1);
        if (temp > 0.0) {
            data.temperature = temp;
            temperature_ = temp; // Update calculator temperature
        } else {
            data.temperature = temperature_;
        }

        return true;

    } catch (const std::exception& e) {
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
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::ostringstream buffer;
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
        double last_temp = extract_value_from_file(last_parent_file, "Kelvin\\.\\s+Pressure", 2, -1);
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
            << std::setw(column_widths[1]) << std::fixed << std::setprecision(8) << data.final_scf_high
            << std::setw(column_widths[2]) << std::fixed << std::setprecision(8) << data.final_scf_low
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
    std::ostream& out = output_file ? *output_file : std::cout;
    
    if (!quiet && !results.empty()) {
        print_summary_info(get_parent_file(results[0].filename), output_file);
    }

    print_components_header(output_file);

    for (const auto& data : results) {
        std::string formatted_name = format_filename(data.filename, 53);
        std::string phase_corr = data.phase_corr_applied ? "YES" : "NO";

        out << std::setw(53) << formatted_name
            << std::setw(15) << std::fixed << std::setprecision(8) << data.final_scf_high
            << std::setw(15) << std::fixed << std::setprecision(8) << data.final_scf_low
            << std::setw(10) << std::fixed << std::setprecision(6) << data.zpe
            << std::setw(10) << std::fixed << std::setprecision(6) << data.tc_only
            << std::setw(10) << std::fixed << std::setprecision(6) << data.ts_value
            << std::setw(15) << std::fixed << std::setprecision(6) << data.enthalpy_hartree
            << std::setw(15) << std::fixed << std::setprecision(6) << data.gibbs_hartree_corrected
            << std::setw(10) << std::fixed << std::setprecision(4) << data.lowest_frequency
            << std::setw(11) << phase_corr << std::endl;
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
