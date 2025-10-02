#include "coord_extractor.h"
#include "job_management/job_checker.h"
#include "utilities/utils.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <unordered_map>
#include <vector>

// External global for shutdown
extern std::atomic<bool> g_shutdown_requested;

CoordExtractor::CoordExtractor(std::shared_ptr<ProcessingContext> ctx, bool quiet) : context(ctx), quiet_mode(quiet) {}

ExtractSummary CoordExtractor::extract_coordinates(const std::vector<std::string>& log_files)
{
    ExtractSummary summary;
    summary.total_files = log_files.size();

    auto start_time = std::chrono::high_resolution_clock::now();

    if (!quiet_mode)
    {
        std::cout << "Found " << log_files.size() << " " << context->extension << " files" << std::endl;
        std::cout << "Extracting coordinates..." << std::endl;
    }

    // First, identify conflicting base names
    std::unordered_map<std::string, std::vector<std::string>> base_name_map;
    for (const auto& log_file : log_files)
    {
        std::filesystem::path path(log_file);
        std::string           base_name = path.stem().string();
        std::string           extension = path.extension().string();
        base_name_map[base_name].push_back(extension);
    }

    // Identify which base names have conflicts (multiple extensions)
    std::unordered_set<std::string> conflicting_base_names;
    for (const auto& [base_name, extensions] : base_name_map)
    {
        if (extensions.size() > 1)
        {
            conflicting_base_names.insert(base_name);
        }
    }

    // Thread-safe containers
    std::vector<std::pair<std::string, JobStatus>> successful_extractions;  // xyz_file, status
    std::mutex                                     results_mutex;
    std::atomic<size_t>                            file_index{0};

    // Calculate safe thread count
    unsigned int num_threads = calculateSafeThreadCount(
        context->requested_threads ? context->requested_threads : 0, log_files.size(), context->job_resources);

    if (!quiet_mode)
    {
        std::cout << "Using " << num_threads << " threads" << std::endl;
    }

    // Process files in parallel
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&]() {
            size_t index;
            while ((index = file_index.fetch_add(1)) < log_files.size())
            {
                if (g_shutdown_requested.load())
                    break;

                try
                {
                    auto file_guard = context->file_manager->acquire();
                    if (!file_guard.is_acquired())
                        continue;

                    std::string error_msg;
                    auto [success, status] = extract_from_file(log_files[index], conflicting_base_names, error_msg);

                    {
                        std::lock_guard<std::mutex> lock(results_mutex);
                        summary.processed_files++;
                        if (success)
                        {
                            std::string xyz_file = generate_xyz_filename(log_files[index], conflicting_base_names);
                            successful_extractions.emplace_back(xyz_file, status);
                            summary.extracted_files++;
                        }
                        else
                        {
                            summary.failed_files++;
                            if (!error_msg.empty())
                            {
                                summary.errors.push_back("Error extracting " + log_files[index] + ": " + error_msg);
                            }
                        }
                    }

                    // Report progress
                    if (!quiet_mode && summary.processed_files % 50 == 0)
                    {
                        report_progress(summary.processed_files, summary.total_files);
                    }
                }
                catch (const std::exception& e)
                {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    summary.errors.push_back("Exception extracting " + log_files[index] + ": " + e.what());
                }
            }
        });
    }

    // Wait for completion
    for (auto& thread : threads)
    {
        thread.join();
    }

    if (!quiet_mode && summary.processed_files > 0)
    {
        report_progress(summary.processed_files, summary.total_files);
        std::cout << std::endl;
    }

    // Move extracted XYZ files (avoid duplicates)
    std::unordered_set<std::string> moved_files;
    for (const auto& [xyz_file, status] : successful_extractions)
    {
        // Skip if this XYZ file has already been moved
        if (moved_files.find(xyz_file) != moved_files.end())
        {
            continue;
        }

        std::string move_error;
        if (move_xyz_file(xyz_file, status, move_error))
        {
            moved_files.insert(xyz_file);
            if (status == JobStatus::COMPLETED)
            {
                summary.moved_to_final++;
            }
            else
            {
                summary.moved_to_running++;
            }
        }
        else
        {
            summary.failed_files++;
            if (!move_error.empty())
            {
                summary.errors.push_back(move_error);
            }
        }
    }

    auto end_time          = std::chrono::high_resolution_clock::now();
    summary.execution_time = std::chrono::duration<double>(end_time - start_time).count();

    return summary;
}

std::pair<bool, JobStatus>
CoordExtractor::extract_from_file(const std::string&                     log_file,
                                  const std::unordered_set<std::string>& conflicting_base_names,
                                  std::string&                           error_msg)
{
    try
    {
        // Use SMART mode to read file, looking for orientation section
        std::string content = Utils::read_file_unified(log_file, FileReadMode::SMART, 1000, "Standard orientation:");

        // Parse into lines
        std::vector<std::string> lines;
        std::istringstream       iss(content);
        std::string              line;
        while (std::getline(iss, line))
        {
            lines.push_back(line);
        }

        // Search backward for the last orientation header
        int start = -1;
        for (int i = static_cast<int>(lines.size()) - 1; i >= 0; --i)
        {
            if (lines[i].find("Standard orientation:") != std::string::npos ||
                lines[i].find("Input orientation:") != std::string::npos)
            {
                start = i;
                break;
            }
        }

        if (start == -1)
        {
            error_msg = "No orientation section found";
            return {false, JobStatus::UNKNOWN};
        }

        // Find end of section
        int end = -1;
        for (int m = start + 5; m < static_cast<int>(lines.size()); ++m)
        {
            if (lines[m].find("----") != std::string::npos)
            {
                end = m;
                break;
            }
        }

        if (end == -1)
        {
            error_msg = "No end delimiter found for orientation section";
            return {false, JobStatus::UNKNOWN};
        }

        int num_atoms = end - start - 5;
        if (num_atoms <= 0)
        {
            error_msg = "Invalid number of atoms";
            return {false, JobStatus::UNKNOWN};
        }

        // Generate output filename
        std::string xyz_file = generate_xyz_filename(log_file, conflicting_base_names);

        // Write XYZ file
        std::ofstream out(xyz_file);
        if (!out.is_open())
        {
            error_msg = "Failed to open output file: " + xyz_file;
            return {false, JobStatus::UNKNOWN};
        }

        out << num_atoms << std::endl;
        out << std::filesystem::path(log_file).stem().string() << std::endl;

        // Parse and write each atom line
        for (int l = start + 5; l < end; ++l)
        {
            std::istringstream ss(lines[l]);
            int                center, atomic_num, type;
            double             x, y, z;
            if (!(ss >> center >> atomic_num >> type >> x >> y >> z))
            {
                error_msg = "Failed to parse coordinate line: " + lines[l];
                out.close();
                std::filesystem::remove(xyz_file);  // Cleanup partial file
                return {false, JobStatus::UNKNOWN};
            }

            std::string symbol = get_atomic_symbol(atomic_num);

            out << std::left << std::setw(10) << symbol << std::right << std::setw(20) << std::fixed
                << std::setprecision(10) << x << std::setw(20) << std::fixed << std::setprecision(10) << y
                << std::setw(20) << std::fixed << std::setprecision(10) << z << std::endl;
        }

        out.close();

        // Determine job status (read last 10 lines)
        std::string              tail_content = Utils::read_file_unified(log_file, FileReadMode::TAIL, 10);
        std::vector<std::string> last_lines;
        std::istringstream       last_iss(tail_content);
        std::string              last_line;
        while (std::getline(last_iss, last_line))
        {
            last_lines.push_back(last_line);
        }

        JobStatus status = JobStatus::RUNNING;  // Default UNDONE

        bool is_completed = false;
        for (const auto& l : last_lines)
        {
            if (l.find("Normal termination of Gaussian") != std::string::npos)
            {
                is_completed = true;
                break;
            }
        }

        if (is_completed)
        {
            status = JobStatus::COMPLETED;
        }
        else
        {
            status = JobStatus::RUNNING;  // or ERROR, but grouped as UNDONE
        }

        return {true, status};
    }
    catch (const std::exception& e)
    {
        error_msg = e.what();
        return {false, JobStatus::UNKNOWN};
    }
}

std::string CoordExtractor::get_atomic_symbol(int atomic_num)
{
    static const std::vector<std::string> symbols = {
        "",   "H",  "He", "Li", "Be", "B",  "C",  "N",  "O",  "F",  "Ne", "Na", "Mg", "Al", "Si", "P",  "S",
        "Cl", "Ar", "K",  "Ca", "Sc", "Ti", "V",  "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As",
        "Se", "Br", "Kr", "Rb", "Sr", "Y",  "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn",
        "Sb", "Te", "I",  "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho",
        "Er", "Tm", "Yb", "Lu", "Hf", "Ta", "W",  "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po",
        "At", "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U",  "Np", "Pu", "Am", "Cm", "Bk", "Cf", "Es", "Fm", "Md",
        "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds", "Rg", "Cn", "Nh", "Fl", "Mc", "Lv", "Ts", "Og"};

    if (atomic_num >= 1 && atomic_num < static_cast<int>(symbols.size()))
    {
        return symbols[atomic_num];
    }
    return "X";  // Unknown
}

std::string CoordExtractor::generate_xyz_filename(const std::string&                     log_file,
                                                  const std::unordered_set<std::string>& conflicting_base_names)
{
    std::filesystem::path path(log_file);
    std::string           stem      = path.stem().string();
    std::string           extension = path.extension().string();

    // Check if this base name has conflicts
    if (conflicting_base_names.find(stem) != conflicting_base_names.end())
    {
        // Include extension for conflicting base names
        return stem + extension + ".xyz";
    }
    else
    {
        // Use simple naming for unique base names
        return stem + ".xyz";
    }
}

bool CoordExtractor::move_xyz_file(const std::string& xyz_file, JobStatus status, std::string& error_msg)
{
    try
    {
        std::string dir_suffix = (status == JobStatus::COMPLETED) ? "_final_coord" : "_running_coord";
        std::string target_dir = get_current_directory_name() + dir_suffix;

        if (!create_target_directory(target_dir))
        {
            error_msg = "Failed to create target directory: " + target_dir;
            return false;
        }

        std::filesystem::path src  = xyz_file;
        std::filesystem::path dest = std::filesystem::path(target_dir) / src.filename();

        std::filesystem::rename(src, dest);
        return true;
    }
    catch (const std::exception& e)
    {
        error_msg = "Failed to move " + xyz_file + ": " + e.what();
        return false;
    }
}

bool CoordExtractor::create_target_directory(const std::string& target_dir)
{
    try
    {
        if (!std::filesystem::exists(target_dir))
        {
            std::filesystem::create_directory(target_dir);
        }
        return true;
    }
    catch (const std::exception& e)
    {
        log_error("Failed to create directory " + target_dir + ": " + e.what());
        return false;
    }
}

std::string CoordExtractor::get_current_directory_name()
{
    return std::filesystem::current_path().filename().string();
}

void CoordExtractor::report_progress(size_t current, size_t total)
{
    if (quiet_mode)
        return;

    double percentage = (static_cast<double>(current) / total) * 100.0;
    std::cout << "\rExtracting: " << current << "/" << total << " files (" << std::fixed << std::setprecision(0)
              << percentage << "%)" << std::flush;
}

void CoordExtractor::print_summary(const ExtractSummary& summary, const std::string& operation)
{
    if (quiet_mode)
        return;

    std::cout << "\n" << operation << " completed:" << std::endl;
    std::cout << "Files processed: " << summary.processed_files << "/" << summary.total_files << std::endl;
    std::cout << "Files extracted: " << summary.extracted_files << std::endl;
    std::cout << "Moved to final: " << summary.moved_to_final << std::endl;
    std::cout << "Moved to running: " << summary.moved_to_running << std::endl;
    std::cout << "Files failed: " << summary.failed_files << std::endl;
    std::cout << "Execution time: " << std::fixed << std::setprecision(3) << summary.execution_time << " seconds"
              << std::endl;

    if (!summary.errors.empty())
    {
        std::cout << "\nErrors encountered:" << std::endl;
        for (const auto& error : summary.errors)
        {
            std::cout << "  " << error << std::endl;
        }
    }
}

void CoordExtractor::log_message(const std::string& message)
{
    if (!quiet_mode)
    {
        std::cout << message << std::endl;
    }
}

void CoordExtractor::log_error(const std::string& error)
{
    context->error_collector->add_error(error);
}
