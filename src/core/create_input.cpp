/**
 * @file create_input.cpp
 * @brief Implementation of Gaussian input file creation from XYZ files
 * @author Le Nhan Pham
 * @date 2025
 *
 * This file contains the implementation of the CreateInput class for
 * creating Gaussian input files from XYZ coordinate files.
 */

#include "create_input.h"
#include "parameter_parser.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

// External global for shutdown
extern std::atomic<bool> g_shutdown_requested;

CreateInput::CreateInput(std::shared_ptr<ProcessingContext> ctx, bool quiet)
    : context(ctx), quiet_mode(quiet), calc_type_(CalculationType::SP), functional_("UwB97XD"), basis_("Def2SVPP"),
      large_basis_(""), solvent_(""), solvent_model_("smd"), print_level_(""), extra_keywords_(""), charge_(0),
      mult_(1), tail_(""), extension_(".gau"), tschk_path_(""), freeze_atoms_({0, 0}), scf_maxcycle_(-1),
      opt_maxcycles_(-1), irc_maxpoints_(-1), irc_recalc_(-1), irc_maxcycle_(-1), irc_stepsize_(-1)
{}

std::string CreateInput::select_basis_for_calculation() const
{
    // Smart basis selection based on calculation type
    switch (calc_type_)
    {
        case CalculationType::HIGH_SP:
            // For HIGH_SP, prefer large_basis if available, otherwise use special fallback logic
            if (!large_basis_.empty())
            {
                // GEN/GENECP basis names are already normalized to uppercase in loadParameters
                return large_basis_;
            }
            else
            {
                // Special fallback: use Def2TZVP if basis is Def2SVPP, otherwise use basis
                return (basis_ == "Def2SVPP" ? "Def2TZVP" : basis_);
            }

        default:
            // For all other calculation types, use the regular basis
            // GEN/GENECP basis names are already normalized to uppercase in loadParameters
            return basis_;
    }
}

bool CreateInput::is_gen_basis(const std::string& basis_str) const
{
    // Check if basis is GEN or GENECP (case insensitive)
    std::string upper_basis = basis_str;
    std::transform(upper_basis.begin(), upper_basis.end(), upper_basis.begin(), ::toupper);
    return (upper_basis == "GEN" || upper_basis == "GENECP");
}

void CreateInput::validate_gen_basis_requirements() const
{
    // For calculations with GEN/GENECP basis, tail is required
    std::string selected_basis = select_basis_for_calculation();


    if (is_gen_basis(selected_basis))
    {
        if (tail_.empty())
        {
            std::string calc_type_name;
            switch (calc_type_)
            {
                case CalculationType::SP:
                    calc_type_name = "SP";
                    break;
                case CalculationType::OPT_FREQ:
                    calc_type_name = "OPT_FREQ";
                    break;
                case CalculationType::TS_FREQ:
                    calc_type_name = "TS_FREQ";
                    break;
                case CalculationType::OSS_TS_FREQ:
                    calc_type_name = "OSS_TS_FREQ";
                    break;
                case CalculationType::OSS_CHECK_SP:
                    calc_type_name = "OSS_CHECK_SP";
                    break;
                case CalculationType::HIGH_SP:
                    calc_type_name = "HIGH_SP";
                    break;
                case CalculationType::IRC_FORWARD:
                    calc_type_name = "IRC_FORWARD";
                    break;
                case CalculationType::IRC_REVERSE:
                    calc_type_name = "IRC_REVERSE";
                    break;
                case CalculationType::IRC:
                    calc_type_name = "IRC";
                    break;
                case CalculationType::MODRE_TS_FREQ:
                    calc_type_name = "MODRE_TS_FREQ";
                    break;
                default:
                    calc_type_name = "Unknown";
                    break;
            }

            throw std::runtime_error(
                "Error: " + calc_type_name +
                " calculation with GEN/GENECP basis requires external basis set (tail parameter).\n"
                "Please provide the external basis set using --tail or in the parameter file.\n"
                "Example: --tail \"H 0\\nS    3 1.00\\n  0.1873113696D+02  0.3349460434D-01\\n****\"");
        }
    }
}


CreateInput::CreateInput(std::shared_ptr<ProcessingContext> ctx, const std::string& param_file, bool quiet)
    : context(ctx), quiet_mode(quiet), calc_type_(CalculationType::SP), functional_("UwB97XD"), basis_("Def2SVPP"),
      large_basis_(""), solvent_(""), solvent_model_("smd"), print_level_(""), extra_keywords_(""), charge_(0),
      mult_(1), tail_(""), extension_(".gau"), tschk_path_(""), freeze_atoms_({0, 0}), scf_maxcycle_(-1),
      opt_maxcycles_(-1), irc_maxpoints_(-1), irc_recalc_(-1), irc_maxcycle_(-1), irc_stepsize_(-1)
{
    if (!loadParameters(param_file))
    {
        throw std::runtime_error("Failed to load parameters from file: " + param_file);
    }
}

bool CreateInput::loadParameters(const std::string& param_file)
{
    ParameterParser parser;
    if (!parser.loadFromFile(param_file))
    {
        return false;
    }

    // Load parameters from the parser, using defaults if not specified
    // Note: We load all parameters but only use relevant ones based on calc_type

    // Basic parameters (always loaded)
    functional_ = parser.getString("functional", "UWB97XD");
    basis_      = parser.getString("basis", "Def2SVPP");
    charge_     = parser.getInt("charge", 0);
    mult_       = parser.getInt("mult", 1);
    extension_  = parser.getString("extension", ".gau");

    // Convert functional and basis to uppercase for better readability
    std::transform(functional_.begin(), functional_.end(), functional_.begin(), ::toupper);
    std::transform(basis_.begin(), basis_.end(), basis_.begin(), ::toupper);

    // Solvent parameters
    solvent_       = parser.getString("solvent", "");
    solvent_model_ = parser.getString("solvent_model", "smd");

    // Advanced parameters
    print_level_    = parser.getString("print_level", "");
    extra_keywords_ = parser.getString("extra_keywords", "");

    // Tail section
    tail_ = parser.getString("tail", "");

    // TS-specific parameters (loaded but only used for relevant calc types)
    large_basis_ = parser.getString("large_basis", "");
    // Convert large_basis to uppercase as well
    std::transform(large_basis_.begin(), large_basis_.end(), large_basis_.begin(), ::toupper);
    tschk_path_ = parser.getString("tschk_path", "");

    // Freeze atoms for TS calculations
    // First try to parse freeze_atoms parameter (supports comma-separated, space-separated, etc.)
    std::string freeze_atoms_str = parser.getString("freeze_atoms", "");
    if (!freeze_atoms_str.empty())
    {
        // Parse freeze_atoms string to extract atom indices
        std::vector<int> atoms = parseFreezeAtomsString(freeze_atoms_str);
        if (atoms.size() >= 2)
        {
            freeze_atoms_ = {atoms[0], atoms[1]};
        }
    }
    else
    {
        // Fall back to separate freeze_atom1 and freeze_atom2 parameters
        int freeze1 = parser.getInt("freeze_atom1", 0);
        int freeze2 = parser.getInt("freeze_atom2", 0);
        if (freeze1 != 0 && freeze2 != 0)
        {
            freeze_atoms_ = {freeze1, freeze2};
        }
    }

    // Set calculation type if specified in the file
    std::string calc_type_str = parser.getString("calc_type", "");
    if (!calc_type_str.empty())
    {
        // Map string to enum
        if (calc_type_str == "sp")
            calc_type_ = CalculationType::SP;
        else if (calc_type_str == "opt_freq")
            calc_type_ = CalculationType::OPT_FREQ;
        else if (calc_type_str == "ts_freq")
            calc_type_ = CalculationType::TS_FREQ;
        else if (calc_type_str == "oss_ts_freq")
            calc_type_ = CalculationType::OSS_TS_FREQ;
        else if (calc_type_str == "oss_check_sp")
            calc_type_ = CalculationType::OSS_CHECK_SP;
        else if (calc_type_str == "high_sp")
            calc_type_ = CalculationType::HIGH_SP;
        else if (calc_type_str == "irc_forward")
            calc_type_ = CalculationType::IRC_FORWARD;
        else if (calc_type_str == "irc_reverse")
            calc_type_ = CalculationType::IRC_REVERSE;
        else if (calc_type_str == "irc")
            calc_type_ = CalculationType::IRC;
        else if (calc_type_str == "modre_ts_freq")
            calc_type_ = CalculationType::MODRE_TS_FREQ;
        // If invalid, keep default SP
    }

    // Custom cycle and optimization parameters
    scf_maxcycle_  = parser.getInt("scf_maxcycle", -1);
    opt_maxcycles_ = parser.getInt("opt_maxcycles", -1);
    irc_maxpoints_ = parser.getInt("irc_maxpoints", -1);
    irc_recalc_    = parser.getInt("irc_recalc", -1);
    irc_maxcycle_  = parser.getInt("irc_maxcycle", -1);
    irc_stepsize_  = parser.getInt("irc_stepsize", -1);

    return true;
}

CreateSummary CreateInput::create_inputs(const std::vector<std::string>& xyz_files)
{
    CreateSummary summary;
    summary.total_files = xyz_files.size();

    auto start_time = std::chrono::high_resolution_clock::now();

    if (!quiet_mode)
    {
        std::cout << "Found " << xyz_files.size() << " " << ".xyz" << " files" << std::endl;
        std::cout << "Creating Gaussian input files..." << std::endl;
    }

    // Thread-safe containers
    std::vector<std::string> successful_creations;  // input_file paths
    std::mutex               results_mutex;
    std::atomic<size_t>      file_index{0};

    // Calculate safe thread count
    unsigned int num_threads = calculateSafeThreadCount(
        context->requested_threads ? context->requested_threads : 0, xyz_files.size(), context->job_resources);

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
            while ((index = file_index.fetch_add(1)) < xyz_files.size())
            {
                if (g_shutdown_requested.load())
                    break;

                try
                {
                    auto file_guard = context->file_manager->acquire();
                    if (!file_guard.is_acquired())
                        continue;

                    std::string error_msg;
                    if (create_from_file(xyz_files[index], error_msg))
                    {
                        {
                            std::lock_guard<std::mutex> lock(results_mutex);
                            std::vector<std::string>    input_files = generate_input_filename(xyz_files[index]);
                            for (const auto& input_file : input_files)
                            {
                                successful_creations.push_back(input_file);
                            }
                            summary.processed_files++;
                            summary.created_files += input_files.size();
                        }
                    }
                    else
                    {
                        {
                            std::lock_guard<std::mutex> lock(results_mutex);
                            summary.processed_files++;
                            summary.failed_files++;
                            if (!error_msg.empty())
                            {
                                summary.errors.push_back("Error creating input for " + xyz_files[index] + ": " +
                                                         error_msg);
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
                    summary.errors.push_back("Exception creating input for " + xyz_files[index] + ": " + e.what());
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

    auto end_time          = std::chrono::high_resolution_clock::now();
    summary.execution_time = std::chrono::duration<double>(end_time - start_time).count();

    return summary;
}

bool CreateInput::create_from_file(const std::string& xyz_file, std::string& error_msg)
{
    try
    {
        // Extract isomer name from filename
        std::filesystem::path path(xyz_file);
        std::string           isomer_name = path.stem().string();

        std::string coordinates;
        if (calc_type_ != CalculationType::IRC_FORWARD && calc_type_ != CalculationType::IRC_REVERSE)
        {
            // Read coordinates from XYZ file for non-IRC calculations
            coordinates = read_xyz_coordinates(xyz_file);
            if (coordinates.empty())
            {
                error_msg = "Failed to read coordinates from XYZ file";
                return false;
            }
        }
        else
        {
            // For IRC calculations, check if TS checkpoint exists
            std::string ts_chk_path;
            if (!tschk_path_.empty())
            {
                std::filesystem::path chk_path = std::filesystem::path(tschk_path_) / (isomer_name + ".chk");
                ts_chk_path                    = chk_path.string();
            }
            else
            {
                // Default to parent directory
                std::filesystem::path current_path = std::filesystem::current_path();
                std::filesystem::path parent_path  = current_path.parent_path();
                std::filesystem::path chk_path     = parent_path / (isomer_name + ".chk");
                ts_chk_path                        = chk_path.string();
            }

            if (!std::filesystem::exists(ts_chk_path))
            {
                error_msg = "TS checkpoint file not found: " + ts_chk_path +
                            ". Please specify --tschk-path or ensure the TS checkpoint exists in the parent directory.";
                return false;
            }
        }

        // Generate output filename(s)
        std::vector<std::string> input_files = generate_input_filename(xyz_file);

        bool all_success = true;
        for (size_t i = 0; i < input_files.size(); ++i)
        {
            const auto& input_file = input_files[i];

            // Generate appropriate content for IRC calculations
            std::string content;
            if (calc_type_ == CalculationType::IRC)
            {
                // For combined IRC, determine if this is forward or reverse based on filename
                bool            is_forward = input_file.find("F" + extension_) != std::string::npos;
                CalculationType temp_type  = is_forward ? CalculationType::IRC_FORWARD : CalculationType::IRC_REVERSE;

                // Temporarily set calculation type for content generation
                CalculationType original_type = calc_type_;
                calc_type_                    = temp_type;
                content                       = generate_input_content(isomer_name, coordinates);
                calc_type_                    = original_type;
            }
            else if (calc_type_ == CalculationType::IRC_FORWARD || calc_type_ == CalculationType::IRC_REVERSE)
            {
                content = generate_input_content(isomer_name, coordinates);
            }
            else
            {
                content = generate_input_content(isomer_name, coordinates);
            }

            // Check if input file already exists
            if (std::filesystem::exists(input_file))
            {
                if (!quiet_mode)
                {
                    std::cout << input_file << " exists and will not be overwritten." << std::endl;
                }
                continue;  // Skip this file
            }

            // Write input file
            if (!write_input_file(input_file, content))
            {
                error_msg   = "Failed to write input file: " + input_file;
                all_success = false;
            }
            else if (!quiet_mode)
            {
                std::cout << input_file << " was newly created." << std::endl;
            }
        }

        return all_success;
    }
    catch (const std::exception& e)
    {
        error_msg = e.what();
        return false;
    }
}

std::string CreateInput::generate_input_content(const std::string& isomer_name, const std::string& coordinates)
{
    // Validate GEN/GENECP basis requirements for all relevant calculation types
    if (calc_type_ == CalculationType::SP || calc_type_ == CalculationType::OPT_FREQ ||
        calc_type_ == CalculationType::TS_FREQ || calc_type_ == CalculationType::OSS_TS_FREQ ||
        calc_type_ == CalculationType::OSS_CHECK_SP || calc_type_ == CalculationType::MODRE_TS_FREQ ||
        calc_type_ == CalculationType::HIGH_SP || calc_type_ == CalculationType::IRC_FORWARD ||
        calc_type_ == CalculationType::IRC_REVERSE || calc_type_ == CalculationType::IRC)
    {
        validate_gen_basis_requirements();
    }

    std::string route_section    = generate_route_section(isomer_name);
    std::string molecule_section = generate_molecule_section(coordinates);

    std::ostringstream content;

    // Determine defaults for configurable parameters (for multi-section)
    int scf_mc = (scf_maxcycle_ != -1) ? scf_maxcycle_ : 300;  // Default 300 for multi-section types
    int opt_mc = (opt_maxcycles_ != -1) ? opt_maxcycles_ : 300;

    // For multi-section inputs (OSS_TS_FREQ, MODRE_TS_FREQ), handle differently
    if (calc_type_ == CalculationType::OSS_TS_FREQ)
    {
        // Pound sign for multi-section
        std::string pound1, pound2, pound3;
        if (print_level_.empty())
        {
            pound1 = pound2 = pound3 = "# ";
        }
        else if (print_level_ == "N" || print_level_ == "P" || print_level_ == "T")
        {
            pound1 = pound2 = pound3 = "#" + print_level_ + " ";
        }
        else
        {
            pound1 = pound2 = pound3 = "#" + print_level_ + " ";
        }

        // Section 1: Stable Opt
        content << generate_checkpoint_section(isomer_name + "-StableOpt") << "\n";
        content << pound1 << "Stable=Opt scf(maxcycle=" << scf_mc << ",xqc) " << functional_ << "/"
                << select_basis_for_calculation();
        if (!solvent_.empty())
        {
            content << " scrf=(" << solvent_model_ << ",solvent=" << solvent_ << ")";
        }
        content << " " << extra_keywords_ << "\n\n";
        content << "Stable Opt to check openshell singlet\n\n";
        content << molecule_section;  // This already ends with \n
        if (!tail_.empty())
        {
            content << "\n" << tail_ << "\n";  // Add blank line before tail
        }
        content << "\n";  // One blank line after everything

        // Section 2: Modredundant
        content << "--Link1--\n";
        content << "%OldChk=" << isomer_name << "-StableOpt.chk\n";
        content << "%chk=" << isomer_name << "-modre.chk\n";
        std::string basis_to_use2 = select_basis_for_calculation();
        content << pound2 << "opt(maxcycles=" << opt_mc << ",modredundant) scf(maxcycle=" << scf_mc << ",xqc) "
                << functional_ << "/" << basis_to_use2;
        if (!solvent_.empty())
        {
            content << " scrf=(" << solvent_model_ << ",solvent=" << solvent_ << ")";
        }
        content << " Guess(read) " << extra_keywords_ << "\n\n";
        content << "Modredundant calculation to freeze the transition state bond\n\n";
        content << molecule_section;  // This already ends with \n

        if (freeze_atoms_.first != 0 && freeze_atoms_.second != 0)
        {
            content << "\n";  // Blank line before B line
            content << "B " << freeze_atoms_.first << " " << freeze_atoms_.second << " F\n";
        }

        if (!tail_.empty())
        {
            content << "\n" << tail_ << "\n";  // Blank line before tail
        }
        content << "\n";  // One blank line after everything

        // Section 3: TS optimization
        content << "--Link1--\n";
        content << "%OldChk=" << isomer_name << "-modre.chk\n";
        content << "%Chk=" << isomer_name << ".chk\n";
        std::string basis_to_use3 = select_basis_for_calculation();
        content << pound3 << "opt(maxcycles=" << opt_mc
                << ",ts,noeigen,calcfc,NoFreeze,MaxStep=5) freq scf(maxcycle=" << scf_mc << ",xqc) " << functional_
                << "/" << basis_to_use3;
        if (!solvent_.empty())
        {
            content << " scrf=(" << solvent_model_ << ",solvent=" << solvent_ << ")";
        }
        content << " Guess(read) Geom(Allcheck) " << extra_keywords_ << "\n\n";
        if (!tail_.empty())
        {
            content << tail_ << "\n";
        }
    }
    else if (calc_type_ == CalculationType::MODRE_TS_FREQ)
    {
        // MODRE_TS_FREQ case
        // Pound sign for multi-section
        std::string pound1, pound2;
        if (print_level_.empty())
        {
            pound1 = pound2 = "# ";
        }
        else if (print_level_ == "N" || print_level_ == "P" || print_level_ == "T")
        {
            pound1 = pound2 = "#" + print_level_ + " ";
        }
        else
        {
            pound1 = pound2 = "#" + print_level_ + " ";
        }

        // Section 1: Modredundant
        content << "%chk=" << isomer_name << "-modre.chk\n";
        std::string basis_to_use1 = select_basis_for_calculation();
        content << pound1 << "opt(maxcycles=" << opt_mc << ",modredundant) scf(maxcycle=" << scf_mc << ",xqc) "
                << functional_ << "/" << basis_to_use1;
        if (!solvent_.empty())
        {
            content << " scrf=(" << solvent_model_ << ",solvent=" << solvent_ << ")";
        }
        content << " " << extra_keywords_ << "\n\n";
        content << "Modredundant calculation to freeze the transition state bond\n\n";
        content << molecule_section;  // This already ends with \n

        if (freeze_atoms_.first != 0 && freeze_atoms_.second != 0)
        {
            content << "\n";  // Blank line before B line
            content << "B " << freeze_atoms_.first << " " << freeze_atoms_.second << " F\n";
        }

        if (!tail_.empty())
        {
            content << "\n" << tail_ << "\n";  // Blank line before tail
        }
        content << "\n";  // One blank line after everything

        // Section 2: TS optimization
        content << "--Link1--\n";
        content << "%OldChk=" << isomer_name << "-modre.chk\n";
        content << "%Chk=" << isomer_name << ".chk\n";
        content << pound2 << "opt(maxcycles=" << opt_mc
                << ",ts,noeigen,calcfc,NoFreeze,MaxStep=5) freq scf(maxcycle=" << scf_mc << ",xqc) " << functional_
                << "/" << basis_;
        if (!solvent_.empty())
        {
            content << " scrf=(" << solvent_model_ << ",solvent=" << solvent_ << ")";
        }
        content << " Guess(read) Geom(Allcheck) " << extra_keywords_ << "\n\n";
        if (!tail_.empty())
        {
            content << tail_ << "\n";
        }
    }
    else
    {
        // Single section inputs
        content << route_section << "\n\n";

        // For calculations with Geom(AllCheck), don't include title, charge, multiplicity, or coordinates
        if (route_section.find("Geom(AllCheck)") != std::string::npos)
        {

            // For HIGH_SP with GEN/GENECP, include tail with proper spacing
            if (calc_type_ == CalculationType::HIGH_SP && is_gen_basis(select_basis_for_calculation()))
            {
                content << tail_ << "\n\n";  // Changed: Removed extra "\n" before tail_ to avoid extra blank line
            }
            else
            {
                // Just add blank lines at the end
                content << "\n";  // Changed: Reduced to "\n" for two blank lines at end (improved consistency)
            }
        }
        else
        {
            // Validate GEN/GENECP basis requirements for non-Geom(AllCheck) calculations
            if (calc_type_ == CalculationType::SP || calc_type_ == CalculationType::OPT_FREQ ||
                calc_type_ == CalculationType::TS_FREQ || calc_type_ == CalculationType::OSS_TS_FREQ ||
                calc_type_ == CalculationType::OSS_CHECK_SP || calc_type_ == CalculationType::MODRE_TS_FREQ)
            {
                validate_gen_basis_requirements();
            }

            content << generate_title() << "\n\n";
            content << molecule_section;
            if (!tail_.empty())
                content << "\n" << tail_ << "\n\n";
        }
    }

    return content.str();
}

std::string CreateInput::generate_route_section(const std::string& isomer_name)
{
    std::ostringstream route;

    // Add checkpoint first (at the top)
    if (calc_type_ == CalculationType::IRC_FORWARD || calc_type_ == CalculationType::IRC_REVERSE)
    {
        // For IRC, use OldChk pointing to TS checkpoint
        std::string ts_chk_path;
        if (!tschk_path_.empty())
        {
            ts_chk_path = tschk_path_ + "/" + isomer_name + ".chk";
        }
        else
        {
            // Default to parent directory
            std::filesystem::path current_path = std::filesystem::current_path();
            std::filesystem::path parent_path  = current_path.parent_path();
            ts_chk_path                        = parent_path.string() + "/" + isomer_name + ".chk";
        }
        route << "%OldChk=" << ts_chk_path << "\n";
        route << "%chk=" << isomer_name << (calc_type_ == CalculationType::IRC_FORWARD ? "F" : "R") << ".chk\n";
    }
    else if (calc_type_ != CalculationType::HIGH_SP && calc_type_ != CalculationType::OSS_TS_FREQ)
    {
        route << "%chk=" << isomer_name << ".chk\n";
    }
    else if (calc_type_ == CalculationType::HIGH_SP)
    {
        std::string ts_chk_path;
        if (!tschk_path_.empty())
        {
            ts_chk_path = tschk_path_ + "/" + isomer_name + ".chk";
        }
        else
        {
            // Default to parent directory
            std::filesystem::path current_path = std::filesystem::current_path();
            std::filesystem::path parent_path  = current_path.parent_path();
            ts_chk_path                        = parent_path.string() + "/" + isomer_name + ".chk";
        }
        route << "%OldChk=" << ts_chk_path << "\n";
        route << "%chk=" << isomer_name << ".chk\n";
    }

    // Pound sign - N, P, T should be directly after #, others get space
    std::string pound;
    if (print_level_.empty())
    {
        pound = "#";
    }
    else if (print_level_ == "N" || print_level_ == "P" || print_level_ == "T")
    {
        pound = "#" + print_level_;
    }
    else
    {
        pound = "#" + print_level_ + " ";
    }

    // Determine defaults for configurable parameters
    int default_scf = (calc_type_ == CalculationType::SP || calc_type_ == CalculationType::OPT_FREQ ||
                       calc_type_ == CalculationType::HIGH_SP)
                          ? 500
                          : 300;
    int scf_mc      = (scf_maxcycle_ != -1) ? scf_maxcycle_ : default_scf;
    int opt_mc      = (opt_maxcycles_ != -1) ? opt_maxcycles_ : 300;

    // Base route components
    route << pound;

    switch (calc_type_)
    {
        case CalculationType::SP:
            route << " scf(maxcycle=" << scf_mc << ",xqc) " << functional_ << "/" << basis_;
            break;

        case CalculationType::OPT_FREQ:
            route << " opt(maxcycles=" << opt_mc << ") freq scf(maxcycle=" << scf_mc << ",xqc) " << functional_ << "/"
                  << basis_;
            break;

        case CalculationType::TS_FREQ: {
            std::string basis_to_use = select_basis_for_calculation();
            route << " opt(maxcycles=" << opt_mc << ",ts,noeigen,calcfc) freq scf(maxcycle=" << scf_mc << ",xqc) "
                  << functional_ << "/" << basis_to_use;
        }
        break;

        case CalculationType::OSS_CHECK_SP:
            route << " Stable=Opt scf(maxcycle=" << scf_mc << ",xqc) " << functional_ << "/" << basis_;
            break;

        case CalculationType::HIGH_SP: {
            std::string basis_to_use = select_basis_for_calculation();
            route << " scf(maxcycle=" << scf_mc << ",xqc) " << functional_ << "/" << basis_to_use
                  << " Guess(Read) Geom(AllCheck)";
        }
        break;

        case CalculationType::IRC_FORWARD:
        case CalculationType::IRC_REVERSE: {
            std::string basis_to_use = select_basis_for_calculation();
            std::string direction    = (calc_type_ == CalculationType::IRC_FORWARD) ? "Forward" : "Reverse";
            int         maxpoints    = (irc_maxpoints_ != -1) ? irc_maxpoints_ : 50;
            int         recalc       = (irc_recalc_ != -1) ? irc_recalc_ : 10;
            int         maxcyc       = (irc_maxcycle_ != -1) ? irc_maxcycle_ : 350;
            int         stepsz       = (irc_stepsize_ != -1) ? irc_stepsize_ : 10;
            route << " irc=(" << direction << ",RCFC,MaxPoints=" << maxpoints << ",Recalc=" << recalc
                  << ",MaxCycle=" << maxcyc << ",StepSize=" << stepsz << ",loose,LQA,nogradstop) " << functional_ << "/"
                  << basis_to_use << " Guess(Read) Geom(AllCheck)";
        }
        break;

        case CalculationType::OSS_TS_FREQ:
            // Handled separately in generate_input_content
            break;

        case CalculationType::MODRE_TS_FREQ:
            // Handled separately in generate_input_content
            break;

        case CalculationType::IRC:
            // Handled separately in generate_input_content
            break;
    }

    // Add solvent if specified
    if (!solvent_.empty() && calc_type_ != CalculationType::OSS_TS_FREQ && calc_type_ != CalculationType::MODRE_TS_FREQ)
    {
        route << " scrf=(" << solvent_model_ << ",solvent=" << solvent_ << ")";
    }

    // Add extra keywords
    if (!extra_keywords_.empty())
    {
        route << " " << extra_keywords_;
    }

    return route.str();
}

std::string CreateInput::generate_checkpoint_section(const std::string& isomer_name)
{
    std::ostringstream chk;
    chk << "%chk=" << isomer_name << ".chk";
    return chk.str();
}

std::string CreateInput::generate_title()
{
    switch (calc_type_)
    {
        case CalculationType::SP:
            return "Title: Normal single point calculation";
        case CalculationType::OPT_FREQ:
            return "Title: Geometrical optimization and frequency calculation";
        case CalculationType::TS_FREQ:
            return "Title: transition state search and frequency calculation";
        case CalculationType::OSS_CHECK_SP:
            return "Title: Stable Opt to check openshell singlet";
        case CalculationType::HIGH_SP:
            return "Title: Single point calculation with higher level of theory (larger basis set)";
        case CalculationType::IRC_FORWARD:
            return "Title: IRC forward";
        case CalculationType::IRC_REVERSE:
            return "Title: IRC reverse";
        case CalculationType::OSS_TS_FREQ:
            return "Title: Openshell singlet transition state search and frequency calculation";
        case CalculationType::MODRE_TS_FREQ:
            return "Title: Modredundant transition state search and frequency calculation";
        default:
            return "Title: Gaussian calculation";
    }
}

std::string CreateInput::generate_molecule_section(const std::string& coordinates)
{
    std::ostringstream molecule;
    molecule << charge_ << " " << mult_ << "\n";
    molecule << coordinates;
    return molecule.str();
}

std::string CreateInput::read_xyz_coordinates(const std::string& xyz_file)
{
    std::ifstream file(xyz_file);
    if (!file.is_open())
    {
        return "";
    }

    std::string              line;
    std::vector<std::string> lines;

    // Read all lines
    while (std::getline(file, line))
    {
        lines.push_back(line);
    }

    if (lines.size() < 3)
    {
        return "";
    }

    // Skip first two lines (atom count and comment), join the rest
    std::ostringstream coords;
    for (size_t i = 2; i < lines.size(); ++i)
    {
        coords << lines[i] << "\n";
    }

    return coords.str();
}

bool CreateInput::write_input_file(const std::string& input_path, const std::string& content)
{
    std::ofstream file(input_path);
    if (!file.is_open())
    {
        return false;
    }

    file << content;
    file.close();
    return true;
}

std::vector<std::string> CreateInput::generate_input_filename(const std::string& xyz_file)
{
    std::filesystem::path path(xyz_file);
    std::filesystem::path dir  = path.parent_path();
    std::string           stem = path.stem().string();

    if (calc_type_ == CalculationType::IRC)
    {
        // Combined IRC creates two files: forward and reverse
        std::vector<std::string> files;
        files.push_back((dir / (stem + "F" + extension_)).string());
        files.push_back((dir / (stem + "R" + extension_)).string());
        return files;
    }
    else if (calc_type_ == CalculationType::IRC_FORWARD)
    {
        return {(dir / (stem + "F" + extension_)).string()};
    }
    else if (calc_type_ == CalculationType::IRC_REVERSE)
    {
        return {(dir / (stem + "R" + extension_)).string()};
    }
    else
    {
        return {(dir / (stem + extension_)).string()};
    }
}

void CreateInput::report_progress(size_t current, size_t total)
{
    if (quiet_mode)
        return;

    double percentage = (static_cast<double>(current) / total) * 100.0;
    std::cout << "\rCreating: " << current << "/" << total << " files (" << std::fixed << std::setprecision(0)
              << percentage << "%)" << std::flush;
}

void CreateInput::print_summary(const CreateSummary& summary, const std::string& operation)
{
    if (quiet_mode)
        return;

    std::cout << "\n" << operation << " completed:" << std::endl;
    std::cout << "Files processed: " << summary.processed_files << "/" << summary.total_files << std::endl;
    std::cout << "Files created: " << summary.created_files << std::endl;
    std::cout << "Files skipped: " << summary.skipped_files << std::endl;
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

void CreateInput::log_message(const std::string& message)
{
    if (!quiet_mode)
    {
        std::cout << message << std::endl;
    }
}

void CreateInput::log_error(const std::string& error)
{
    context->error_collector->add_error(error);
}

// Setter methods
void CreateInput::set_calculation_type(CalculationType type)
{
    calc_type_ = type;
}

void CreateInput::set_functional(const std::string& functional)
{
    functional_ = functional;
}

void CreateInput::set_basis(const std::string& basis)
{
    basis_ = basis;
}

void CreateInput::set_large_basis(const std::string& large_basis)
{
    large_basis_ = large_basis;
}

void CreateInput::set_solvent(const std::string& solvent, const std::string& model)
{
    solvent_       = solvent;
    solvent_model_ = model;
}

void CreateInput::set_print_level(const std::string& print_level)
{
    print_level_ = print_level;
}

void CreateInput::set_extra_keywords(const std::string& keywords)
{
    extra_keywords_ = keywords;
}

void CreateInput::set_molecular_specs(int charge, int mult)
{
    charge_ = charge;
    mult_   = mult;
}

void CreateInput::set_tail(const std::string& tail)
{
    tail_ = tail;
}

void CreateInput::set_extension(const std::string& extension)
{
    extension_ = extension;
}

void CreateInput::set_tschk_path(const std::string& path)
{
    tschk_path_ = path;
}

void CreateInput::set_freeze_atoms(int atom1, int atom2)
{
    freeze_atoms_ = {atom1, atom2};
}

void CreateInput::set_scf_maxcycle(int maxcycle)
{
    scf_maxcycle_ = maxcycle;
}

void CreateInput::set_opt_maxcycles(int maxcycles)
{
    opt_maxcycles_ = maxcycles;
}

void CreateInput::set_irc_maxpoints(int maxpoints)
{
    irc_maxpoints_ = maxpoints;
}

void CreateInput::set_irc_recalc(int recalc)
{
    irc_recalc_ = recalc;
}

void CreateInput::set_irc_maxcycle(int maxcycle)
{
    irc_maxcycle_ = maxcycle;
}

void CreateInput::set_irc_stepsize(int stepsize)
{
    irc_stepsize_ = stepsize;
}

std::vector<int> CreateInput::parseFreezeAtomsString(const std::string& freeze_str)
{
    std::vector<int> atoms;
    std::string      cleaned_str = freeze_str;

    // Remove any leading/trailing whitespace
    cleaned_str.erase(cleaned_str.begin(), std::find_if(cleaned_str.begin(), cleaned_str.end(), [](unsigned char ch) {
                          return !std::isspace(ch);
                      }));
    cleaned_str.erase(std::find_if(cleaned_str.rbegin(),
                                   cleaned_str.rend(),
                                   [](unsigned char ch) {
                                       return !std::isspace(ch);
                                   })
                          .base(),
                      cleaned_str.end());

    // Handle comma-separated format: "1,2"
    if (cleaned_str.find(',') != std::string::npos)
    {
        std::stringstream ss(cleaned_str);
        std::string       token;
        while (std::getline(ss, token, ','))
        {
            // Trim whitespace from token
            token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](unsigned char ch) {
                            return !std::isspace(ch);
                        }));
            token.erase(std::find_if(token.rbegin(),
                                     token.rend(),
                                     [](unsigned char ch) {
                                         return !std::isspace(ch);
                                     })
                            .base(),
                        token.end());

            try
            {
                int atom = std::stoi(token);
                atoms.push_back(atom);
            }
            catch (const std::exception&)
            {
                // Skip invalid tokens
            }
        }
    }
    // Handle space-separated format: "1 2"
    else
    {
        std::stringstream ss(cleaned_str);
        int               atom;
        while (ss >> atom)
        {
            atoms.push_back(atom);
        }
    }

    return atoms;
}