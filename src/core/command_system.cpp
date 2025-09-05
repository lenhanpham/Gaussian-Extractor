#include "command_system.h"
#include "config_manager.h"
#include "help_utils.h"
#include "parameter_parser.h"
#include "utils.h"
#include "version.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

CommandContext CommandParser::parse(int argc, char* argv[])
{
    // Early check for version before any other processing
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--version" || arg == "-v")
        {
            std::cout << GaussianExtractor::get_version_info() << std::endl;
            std::exit(0);
        }
    }

    // Load configuration first
    load_configuration();

    CommandContext context;
    // Apply configuration defaults after config is loaded
    apply_config_to_context(context);

    // Detect job scheduler resources early
    context.job_resources = JobSchedulerDetector::detect_job_resources();

    // If no arguments, default to EXTRACT
    if (argc == 1)
    {
        validate_context(context);
        return context;
    }

    // Scan all arguments to find a command (flexible positioning)
    CommandType found_command = CommandType::EXTRACT;
    int         command_index = -1;

    for (int i = 1; i < argc; ++i)
    {
        CommandType potential_command = parse_command(argv[i]);
        if (potential_command != CommandType::EXTRACT || std::string(argv[i]) == "extract")
        {
            found_command = potential_command;
            command_index = i;
            break;
        }
    }

    context.command = found_command;

    // Parse all arguments, skipping the command if found
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        // Skip the command argument itself
        if (i == command_index)
        {
            continue;
        }

        if (arg == "-h" || arg == "--help")
        {
            if (context.command == CommandType::EXTRACT)
            {
                HelpUtils::print_help();  // Use default parameter
            }
            else
            {
                HelpUtils::print_command_help(context.command);  // Use default parameter
            }
            std::exit(0);
        }

        else if (arg == "--config-help")
        {
            HelpUtils::print_config_help();
            std::exit(0);
        }
        else if (arg == "--create-config")
        {
            HelpUtils::create_default_config();
            std::exit(0);
        }
        else if (arg == "--show-config")
        {
            g_config_manager.print_config_summary(true);
            std::exit(0);
        }
        else if (arg == "--genci-params")
        {
            std::string template_type = "";   // Empty means general template
            std::string directory     = ".";  // Default to current directory
            std::string filename;
            bool        is_general_template = true;

            if (++i < argc)
            {
                std::string first_arg = argv[i];

                if (first_arg[0] == '-')
                {
                    // Next argument is another option, use defaults (general template)
                    --i;
                }
                else if (first_arg.find('/') != std::string::npos || first_arg.find('\\') != std::string::npos ||
                         first_arg.find('.') == 0 || std::filesystem::exists(first_arg))
                {
                    // Argument looks like a directory path
                    directory = first_arg;

                    // Check if there's a calculation type argument after the directory
                    if (i + 1 < argc && argv[i + 1][0] != '-')
                    {
                        template_type       = argv[++i];
                        is_general_template = false;
                    }
                    // If no calc_type after directory, generate general template in that directory
                }
                else
                {
                    // Assume it's a calculation type
                    template_type       = first_arg;
                    is_general_template = false;

                    // Check if there's an optional directory argument
                    if (i + 1 < argc && argv[i + 1][0] != '-')
                    {
                        directory = argv[++i];
                    }
                }
            }
            else
            {
                // No arguments provided, use defaults (general template)
                --i;
            }

            // Ensure directory exists or create it
            std::filesystem::path dir_path(directory);
            if (!std::filesystem::exists(dir_path))
            {
                try
                {
                    std::filesystem::create_directories(dir_path);
                }
                catch (const std::filesystem::filesystem_error& e)
                {
                    std::cerr << "Error: Cannot create directory " << directory << ": " << e.what() << std::endl;
                    exit(1);
                }
            }

            ParameterParser parser;
            bool            success;

            if (is_general_template)
            {
                std::filesystem::path base_path  = std::filesystem::path(directory) / "ci_parameters.params";
                std::filesystem::path final_path = Utils::generate_unique_filename(base_path);
                filename                         = final_path.string();
                success                          = parser.generateGeneralTemplate(filename);
            }
            else
            {
                std::filesystem::path base_path  = std::filesystem::path(directory) / (template_type + ".params");
                std::filesystem::path final_path = Utils::generate_unique_filename(base_path);
                filename                         = final_path.string();
                success                          = parser.generateTemplate(template_type, filename);
            }

            if (success)
            {
                std::cout << "Template generated successfully: " << filename << std::endl;
                if (is_general_template)
                {
                    std::cout << "This is a general parameter file containing all possible parameters." << std::endl;
                    std::cout << "Edit the calc_type and uncomment relevant parameters as needed." << std::endl;
                }
                std::cout << "Use with: gaussian_extractor ci --param-file " << filename << std::endl;
                exit(0);  // Exit after generating template
            }
            else
            {
                if (is_general_template)
                {
                    std::cerr << "Failed to generate general template" << std::endl;
                }
                else
                {
                    std::cerr << "Failed to generate template for: " << template_type << std::endl;
                }
                exit(1);
            }
        }
        else if (arg == "--genci-all-params")
        {
            std::string directory = ".";  // Default to current directory

            // Check if there's an optional directory argument
            if (++i < argc && argv[i][0] != '-')
            {
                directory = argv[i];
            }
            else
            {
                // No directory provided, use current directory
                --i;  // Reset index since we didn't consume an argument
            }

            // Ensure directory path is absolute for clear feedback
            std::filesystem::path dir_path(directory);
            std::filesystem::path abs_dir_path = std::filesystem::absolute(dir_path);

            ParameterParser parser;
            if (parser.generateAllTemplates(directory))
            {
                exit(0);  // Exit after generating templates
            }
            else
            {
                std::cerr << "Failed to generate templates in: " << abs_dir_path.string() << std::endl;
                exit(1);
            }
        }

        // Parse common options first
        parse_common_options(context, i, argc, argv);

        // Parse command-specific options
        if (context.command == CommandType::EXTRACT || context.command == CommandType::HIGH_LEVEL_KJ ||
            context.command == CommandType::HIGH_LEVEL_AU)
        {
            parse_extract_options(context, i, argc, argv);
        }
        else if (context.command == CommandType::EXTRACT_COORDS)
        {
            parse_xyz_options(context, i, argc, argv);
        }
        else if (context.command == CommandType::CREATE_INPUT)
        {
            parse_create_input_options(context, i, argc, argv);
        }
        else
        {
            parse_checker_options(context, i, argc, argv);
        }
    }

    validate_context(context);
    return context;
}

CommandType CommandParser::parse_command(const std::string& cmd)
{
    if (cmd == "extract")
        return CommandType::EXTRACT;
    if (cmd == "done")
        return CommandType::CHECK_DONE;
    if (cmd == "errors")
        return CommandType::CHECK_ERRORS;
    if (cmd == "pcm")
        return CommandType::CHECK_PCM;
    if (cmd == "imode" || cmd == "--imaginary")
        return CommandType::CHECK_IMAGINARY;
    if (cmd == "check")
        return CommandType::CHECK_ALL;
    if (cmd == "high-kj" || cmd == "--high-level-kj")
        return CommandType::HIGH_LEVEL_KJ;
    if (cmd == "high-au" || cmd == "--high-level-au")
        return CommandType::HIGH_LEVEL_AU;
    if (cmd == "xyz" || cmd == "--extract-coord")
        return CommandType::EXTRACT_COORDS;
    if (cmd == "ci" || cmd == "--create-input")
        return CommandType::CREATE_INPUT;

    // If it starts with '-', it's probably an option, not a command
    if (!cmd.empty() && cmd.front() == '-')
        return CommandType::EXTRACT;

    // Default to extract for backward compatibility
    return CommandType::EXTRACT;
}

std::string CommandParser::get_command_name(CommandType command)
{
    switch (command)
    {
        case CommandType::EXTRACT:
            return std::string("extract");
        case CommandType::CHECK_DONE:
            return std::string("done");
        case CommandType::CHECK_ERRORS:
            return std::string("errors");
        case CommandType::CHECK_PCM:
            return std::string("pcm");
        case CommandType::CHECK_IMAGINARY:
            return std::string("imode");
        case CommandType::CHECK_ALL:
            return std::string("check");
        case CommandType::HIGH_LEVEL_KJ:
            return std::string("high-kj");
        case CommandType::HIGH_LEVEL_AU:
            return std::string("high-au");
        case CommandType::EXTRACT_COORDS:
            return std::string("xyz");
        case CommandType::CREATE_INPUT:
            return std::string("ci");
        default:
            return std::string("unknown");
    }
}

void CommandParser::parse_common_options(CommandContext& context, int& i, int argc, char* argv[])
{
    std::string arg = argv[i];

    if (arg == "-q" || arg == "--quiet")
    {
        context.quiet = true;
    }
    else if (arg == "-e" || arg == "--ext")
    {
        if (++i < argc)
        {
            std::string ext      = argv[i];
            std::string full_ext = (ext[0] == '.') ? ext : ("." + ext);
            if (full_ext == ".log" || full_ext == ".out")
            {
                context.extension = full_ext;
            }
            else
            {
                add_warning(context,
                            "Error: Extension '" + ext + "' not in configured output extensions. Using default.");
                context.extension = g_config_manager.get_default_output_extension();
            }
        }
        else
        {
            add_warning(context, "Error: Extension value required after -e/--ext.");
        }
    }
    else if (arg == "-nt" || arg == "--threads")
    {
        if (++i < argc)
        {
            std::string  threads_arg    = argv[i];
            unsigned int hardware_cores = std::thread::hardware_concurrency();
            if (hardware_cores == 0)
                hardware_cores = 4;

            if (threads_arg == "max")
            {
                context.requested_threads = hardware_cores;
            }
            else if (threads_arg == "half")
            {
                context.requested_threads = std::max(1u, hardware_cores / 2);
            }
            else
            {
                try
                {
                    unsigned int req_threads = std::stoul(threads_arg);
                    if (req_threads == 0)
                    {
                        add_warning(context, "Error: Thread count must be at least 1. Using configured default.");
                        context.requested_threads = g_config_manager.get_default_threads();
                    }
                    else
                    {
                        context.requested_threads = req_threads;
                    }
                }


                catch (const std::exception& e)
                {
                    add_warning(context, "Error: Invalid thread count format. Using configured default.");
                    context.requested_threads = g_config_manager.get_default_threads();
                }
            }
        }
        else
        {
            add_warning(context, "Error: Thread count required after -nt/--threads.");
        }
    }
    else if (arg == "--max-file-size")
    {
        if (++i < argc)
        {
            try
            {
                int size = std::stoi(argv[i]);
                if (size <= 0)
                {
                    add_warning(context, "Error: Max file size must be positive. Using default 100MB.");
                }
                else
                {
                    context.max_file_size_mb = static_cast<size_t>(size);
                }
            }
            catch (const std::exception& e)
            {
                add_warning(context, "Error: Invalid max file size format. Using default 100MB.");
            }
        }
        else
        {
            add_warning(context, "Error: Max file size value required after --max-file-size.");
        }
    }
    else if (arg == "--batch-size")
    {
        if (++i < argc)
        {
            try
            {
                int size = std::stoi(argv[i]);
                if (size <= 0)
                {
                    add_warning(context, "Error: Batch size must be positive. Using default (auto-detect).");
                }
                else
                {
                    context.batch_size = static_cast<size_t>(size);
                }
            }
            catch (const std::exception& e)
            {
                add_warning(context, "Error: Invalid batch size format. Using default (auto-detect).");
            }
        }
        else
        {
            add_warning(context, "Error: Batch size value required after --batch-size.");
        }
    }
}

void CommandParser::parse_extract_options(CommandContext& context, int& i, int argc, char* argv[])
{
    std::string arg = argv[i];

    if (arg == "-t" || arg == "--temp")
    {
        if (++i < argc)
        {
            try
            {
                context.temp = std::stod(argv[i]);
                if (context.temp <= 0)
                {
                    add_warning(context, "Warning: Temperature must be positive. Using default 298.15 K.");
                    context.temp = 298.15;
                }
                else
                {
                    context.use_input_temp = true;
                }
            }
            catch (const std::exception& e)
            {
                add_warning(context, "Error: Invalid temperature format. Using default 298.15 K.");
                context.temp = 298.15;
            }
        }
        else
        {
            add_warning(context, "Error: Temperature value required after -t/--temp.");
        }
    }
    else if (arg == "-c" || arg == "--cm")
    {
        if (++i < argc)
        {
            try
            {
                int conc = std::stoi(argv[i]);
                if (conc <= 0)
                {
                    add_warning(context, "Error: Concentration must be positive. Using configured default.");
                    context.concentration = static_cast<int>(g_config_manager.get_default_concentration() * 1000);
                }
                else
                {
                    context.concentration = conc * 1000;
                }
            }
            catch (const std::exception& e)
            {
                add_warning(context, "Error: Invalid concentration format. Using configured default.");
                context.concentration = static_cast<int>(g_config_manager.get_default_concentration() * 1000);
            }
        }
        else
        {
            add_warning(context, "Error: Concentration value required after -c/--cm.");
        }
    }
    else if (arg == "-col" || arg == "--column")
    {
        if (++i < argc)
        {
            try
            {
                int col = std::stoi(argv[i]);
                if (col >= 1 && col <= 10)
                {
                    context.sort_column = col;
                }
                else
                {
                    add_warning(context, "Error: Column must be between 1-10. Using default column 2.");
                }
            }
            catch (const std::exception& e)
            {
                add_warning(context, "Error: Invalid column format. Using default column 2.");
            }
        }
        else
        {
            add_warning(context, "Error: Column value required after -col/--column.");
        }
    }
    else if (arg == "-f" || arg == "--format")
    {
        if (++i < argc)
        {
            std::string fmt = argv[i];
            if (fmt == "text" || fmt == "csv")
            {
                context.output_format = fmt;
            }
            else
            {
                add_warning(context, "Error: Format must be 'text' or 'csv'. Using default 'text'.");
                context.output_format = "text";
            }
        }
        else
        {
            add_warning(context, "Error: Format value required after -f/--format.");
        }
    }
    else if (arg == "--memory-limit")
    {
        if (++i < argc)
        {
            try
            {
                int size = std::stoi(argv[i]);
                if (size <= 0)
                {
                    add_warning(context, "Error: Memory limit must be positive. Using auto-calculated limit.");
                }
                else
                {
                    context.memory_limit_mb = static_cast<size_t>(size);
                }
            }
            catch (const std::exception& e)
            {
                add_warning(context, "Error: Invalid memory limit format. Using auto-calculated limit.");
            }
        }
        else
        {
            add_warning(context, "Error: Memory limit value required after --memory-limit.");
        }
    }
    else if (arg == "--resource-info")
    {
        context.show_resource_info = true;
    }
    else if (!arg.empty() && arg.front() == '-')
    {
        add_warning(context, "Warning: Unknown argument '" + arg + "' ignored.");
    }
}

void CommandParser::parse_checker_options(CommandContext& context, int& i, int argc, char* argv[])
{
    std::string arg = argv[i];

    if (arg == "--target-dir")
    {
        if (++i < argc)
        {
            context.target_dir = argv[i];
        }
        else
        {
            add_warning(context, "Error: Target directory name required after --target-dir.");
        }
    }
    else if (arg == "--dir-suffix")
    {
        if (++i < argc)
        {
            context.dir_suffix = argv[i];
        }
        else
        {
            add_warning(context, "Error: Directory suffix required after --dir-suffix.");
        }
    }
    else if (arg == "--show-details")
    {
        context.show_error_details = true;
    }
    else if (!arg.empty() && arg.front() == '-')
    {
        add_warning(context, "Warning: Unknown argument '" + arg + "' ignored.");
    }
}

// In command_system.cpp, modify parse_xyz_options:
void CommandParser::parse_xyz_options(CommandContext& context, int& i, int argc, char* argv[])
{
    std::string arg = argv[i];

    if (arg == "-f" || arg == "--files")
    {
        bool files_found = false;
        // Keep consuming arguments until we hit another option or the end
        while (++i < argc)
        {
            std::string file_arg = argv[i];

            // Check if the argument is another option
            if (file_arg.length() > 1 && file_arg[0] == '-')
            {
                // It's another option, so we're done with files.
                // Decrement i so the main loop can process this new option.
                i--;
                break;
            }

            files_found = true;

            // Process the argument, which may contain multiple filenames separated by commas
            std::replace(file_arg.begin(), file_arg.end(), ',', ' ');
            std::istringstream iss(file_arg);
            std::string        file;
            while (iss >> file)
            {
                if (!file.empty())
                {
                    // Trim whitespace (should be handled by stringstream, but good practice)
                    file.erase(0, file.find_first_not_of(" "));
                    file.erase(file.find_last_not_of(" ") + 1);

                    if (file.empty())
                        continue;

                    bool has_valid_extension = false;
                    for (const auto& ext : context.valid_extensions)
                    {
                        if (file.size() >= ext.size() && file.compare(file.size() - ext.size(), ext.size(), ext) == 0)
                        {
                            has_valid_extension = true;
                            break;
                        }
                    }

                    if (!has_valid_extension)
                    {
                        file += context.extension;
                    }

                    if (!std::filesystem::exists(file))
                    {
                        add_warning(context, "Specified file does not exist: " + file);
                    }

                    context.specific_files.push_back(file);
                }
            }
        }

        if (!files_found)
        {
            add_warning(context, "--files requires a filename or list of filenames");
        }
    }
}

void CommandParser::add_warning(CommandContext& context, const std::string& warning)
{
    context.warnings.push_back(warning);
}


void CommandParser::validate_context(CommandContext& context)
{
    // Set default threads if not specified
    if (context.requested_threads == 0)
    {
        context.requested_threads = g_config_manager.get_default_threads();
    }

    // Validate file size limits
    if (context.max_file_size_mb == 0)
    {
        context.max_file_size_mb = g_config_manager.get_default_max_file_size();
    }
}

void CommandParser::apply_config_to_context(CommandContext& context)
{
    // Apply configuration defaults to context
    if (!g_config_manager.is_config_loaded())
    {
        return;  // Keep built-in defaults if config not loaded
    }

    // Apply configuration values
    context.quiet              = g_config_manager.get_bool("quiet_mode");
    context.requested_threads  = g_config_manager.get_default_threads();
    context.max_file_size_mb   = g_config_manager.get_default_max_file_size();
    context.extension          = g_config_manager.get_default_output_extension();
    context.valid_extensions   = ConfigUtils::split_string(g_config_manager.get_string("output_extensions"), ',');
    context.temp               = g_config_manager.get_default_temperature();
    context.concentration      = static_cast<int>(g_config_manager.get_default_concentration() * 1000);
    context.sort_column        = g_config_manager.get_int("default_sort_column");
    context.output_format      = g_config_manager.get_default_output_format();
    context.use_input_temp     = g_config_manager.get_bool("use_input_temp");
    context.memory_limit_mb    = g_config_manager.get_size_t("memory_limit_mb");
    context.show_error_details = g_config_manager.get_bool("show_error_details");
    context.dir_suffix         = g_config_manager.get_string("done_directory_suffix");
}

void CommandParser::load_configuration()
{
    // Try to load configuration file
    if (!g_config_manager.is_config_loaded())
    {
        g_config_manager.load_config();
    }
}


void CommandParser::create_default_config()
{
    std::cout << "Creating default configuration file...\n";

    if (g_config_manager.create_default_config_file())
    {
        std::string home_dir = g_config_manager.get_user_home_directory();
        if (!home_dir.empty())
        {
            std::cout << "Configuration file created at: " << home_dir << "/.gaussian_extractor.conf" << std::endl;
        }
        else
        {
            std::cout << "Configuration file created at: ./.gaussian_extractor.conf" << std::endl;
        }
        std::cout << "Edit this file to customize your default settings.\n";
    }
    else
    {
        std::cout << "Failed to create configuration file.\n";
        std::cout << "You can create it manually using the template below:\n\n";
        g_config_manager.print_config_file_template();
    }
}

std::unordered_map<std::string, std::string> CommandParser::extract_config_overrides(int argc, char* argv[])
{
    std::unordered_map<std::string, std::string> overrides;

    for (int i = 1; i < argc - 1; ++i)
    {
        std::string arg = argv[i];
        if (arg.substr(0, 9) == "--config-")
        {
            std::string key   = arg.substr(9);
            std::string value = argv[i + 1];
            overrides[key]    = value;
            ++i;  // Skip the value argument
        }
    }

    return overrides;
}


void CommandParser::parse_create_input_options(CommandContext& context, int& i, int argc, char* argv[])
{
    std::string arg = argv[i];

    if (arg == "--calc-type")
    {
        if (++i < argc)
        {
            context.ci_calc_type = argv[i];
        }
        else
        {
            add_warning(context, "Error: calc-type requires a value");
        }
    }
    else if (arg == "--functional")
    {
        if (++i < argc)
        {
            context.ci_functional = argv[i];
        }
        else
        {
            add_warning(context, "Error: functional requires a value");
        }
    }
    else if (arg == "--basis")
    {
        if (++i < argc)
        {
            context.ci_basis = argv[i];
        }
        else
        {
            add_warning(context, "Error: basis requires a value");
        }
    }
    else if (arg == "--large-basis")
    {
        if (++i < argc)
        {
            context.ci_large_basis = argv[i];
        }
        else
        {
            add_warning(context, "Error: large-basis requires a value");
        }
    }
    else if (arg == "--solvent")
    {
        if (++i < argc)
        {
            context.ci_solvent = argv[i];
        }
        else
        {
            add_warning(context, "Error: solvent requires a value");
        }
    }
    else if (arg == "--solvent-model")
    {
        if (++i < argc)
        {
            context.ci_solvent_model = argv[i];
        }
        else
        {
            add_warning(context, "Error: solvent-model requires a value");
        }
    }
    else if (arg == "--charge")
    {
        if (++i < argc)
        {
            try
            {
                context.ci_charge = std::stoi(argv[i]);
            }
            catch (const std::exception& e)
            {
                add_warning(context, "Error: invalid charge value");
            }
        }
        else
        {
            add_warning(context, "Error: charge requires a value");
        }
    }
    else if (arg == "--mult")
    {
        if (++i < argc)
        {
            try
            {
                context.ci_mult = std::stoi(argv[i]);
            }
            catch (const std::exception& e)
            {
                add_warning(context, "Error: invalid multiplicity value");
            }
        }
        else
        {
            add_warning(context, "Error: mult requires a value");
        }
    }
    else if (arg == "--print-level")
    {
        if (++i < argc)
        {
            context.ci_print_level = argv[i];
        }
        else
        {
            add_warning(context, "Error: print-level requires a value");
        }
    }
    else if (arg == "--extra-keywords")
    {
        if (++i < argc)
        {
            context.ci_extra_keywords = argv[i];
        }
        else
        {
            add_warning(context, "Error: extra-keywords requires a value");
        }
    }
    else if (arg == "--tail")
    {
        if (++i < argc)
        {
            context.ci_tail = argv[i];
        }
        else
        {
            add_warning(context, "Error: tail requires a value");
        }
    }
    else if (arg == "--extension")
    {
        if (++i < argc)
        {
            context.ci_extension = argv[i];
        }
        else
        {
            add_warning(context, "Error: extension requires a value");
        }
    }
    else if (arg == "--tschk-path")
    {
        if (++i < argc)
        {
            context.ci_tschk_path = argv[i];
        }
        else
        {
            add_warning(context, "Error: tschk-path requires a value");
        }
    }
    else if (arg == "--freeze-atoms")
    {
        if (++i < argc)
        {
            try
            {
                context.ci_freeze_atom1 = std::stoi(argv[i]);
                if (++i < argc)
                {
                    context.ci_freeze_atom2 = std::stoi(argv[i]);
                }
                else
                {
                    add_warning(context, "Error: freeze-atoms requires two values");
                }
            }
            catch (const std::exception& e)
            {
                add_warning(context, "Error: freeze-atoms requires integer values");
            }
        }
        else
        {
            add_warning(context, "Error: freeze-atoms requires values");
        }
    }
    else if (arg == "--genci-params")
    {
        std::string template_type = "";   // Empty means general template
        std::string directory     = ".";  // Default to current directory
        std::string filename;
        bool        is_general_template = true;

        if (++i < argc)
        {
            std::string first_arg = argv[i];

            if (first_arg[0] == '-')
            {
                // Next argument is another option, use defaults (general template)
                --i;
            }
            else if (first_arg.find('/') != std::string::npos || first_arg.find('\\') != std::string::npos ||
                     first_arg.find('.') == 0 || std::filesystem::exists(first_arg))
            {
                // Argument looks like a directory path
                directory = first_arg;

                // Check if there's a calculation type argument after the directory
                if (i + 1 < argc && argv[i + 1][0] != '-')
                {
                    template_type       = argv[++i];
                    is_general_template = false;
                }
                // If no calc_type after directory, generate general template in that directory
            }
            else
            {
                // Assume it's a calculation type
                template_type       = first_arg;
                is_general_template = false;

                // Check if there's an optional directory argument
                if (i + 1 < argc && argv[i + 1][0] != '-')
                {
                    directory = argv[++i];
                }
            }
        }
        else
        {
            // No arguments provided, use defaults (general template)
            --i;
        }

        // Ensure directory exists or create it
        std::filesystem::path dir_path(directory);
        if (!std::filesystem::exists(dir_path))
        {
            try
            {
                std::filesystem::create_directories(dir_path);
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                std::cerr << "Error: Cannot create directory " << directory << ": " << e.what() << std::endl;
                exit(1);
            }
        }

        ParameterParser parser;
        bool            success;

        if (is_general_template)
        {
            std::filesystem::path base_path  = std::filesystem::path(directory) / "ci_parameters.params";
            std::filesystem::path final_path = Utils::generate_unique_filename(base_path);
            filename                         = final_path.string();
            success                          = parser.generateGeneralTemplate(filename);
        }
        else
        {
            std::filesystem::path base_path  = std::filesystem::path(directory) / (template_type + ".params");
            std::filesystem::path final_path = Utils::generate_unique_filename(base_path);
            filename                         = final_path.string();
            success                          = parser.generateTemplate(template_type, filename);
        }

        if (success)
        {
            std::cout << "Template generated successfully: " << filename << std::endl;
            if (is_general_template)
            {
                std::cout << "This is a general parameter file containing all possible parameters." << std::endl;
                std::cout << "Edit the calc_type and uncomment relevant parameters as needed." << std::endl;
            }
            std::cout << "Use with: gaussian_extractor ci --param-file " << filename << std::endl;
            exit(0);  // Exit after generating template
        }
        else
        {
            if (is_general_template)
            {
                std::cerr << "Failed to generate general template" << std::endl;
            }
            else
            {
                std::cerr << "Failed to generate template for: " << template_type << std::endl;
            }
            exit(1);
        }
    }
    else if (arg == "--genci-all-params")
    {
        std::string directory = ".";  // Default to current directory

        // Check if there's an optional directory argument
        if (++i < argc && argv[i][0] != '-')
        {
            directory = argv[i];
        }
        else
        {
            // No directory provided, use current directory
            --i;  // Reset index since we didn't consume an argument
        }

        // Ensure directory path is absolute for clear feedback
        std::filesystem::path dir_path(directory);
        std::filesystem::path abs_dir_path = std::filesystem::absolute(dir_path);

        ParameterParser parser;
        if (parser.generateAllTemplates(directory))
        {
            exit(0);  // Exit after generating templates
        }
        else
        {
            std::cerr << "Failed to generate templates in: " << abs_dir_path.string() << std::endl;
            exit(1);
        }
    }
    else if (arg == "--param-file")
    {
        std::string param_file;

        if (++i < argc && argv[i][0] != '-')
        {
            // User specified a file path
            param_file = argv[i];
        }
        else
        {
            // No file specified, look for default parameter file
            param_file = find_or_create_default_param_file();
            if (param_file.empty())
            {
                add_warning(context, "Error: Could not find or create default parameter file");
                return;
            }
            // Decrement i since we didn't consume an argument
            --i;
        }

        ParameterParser parser;
        if (parser.loadFromFile(param_file))
        {
            // Apply loaded parameters to context
            context.ci_calc_type     = parser.getString("calc_type", context.ci_calc_type);
            context.ci_functional    = parser.getString("functional", context.ci_functional);
            context.ci_basis         = parser.getString("basis", context.ci_basis);
            context.ci_large_basis   = parser.getString("large_basis", context.ci_large_basis);
            context.ci_solvent       = parser.getString("solvent", context.ci_solvent);
            context.ci_solvent_model = parser.getString("solvent_model", context.ci_solvent_model);

            // Convert functional and basis to uppercase for better readability
            std::transform(
                context.ci_functional.begin(), context.ci_functional.end(), context.ci_functional.begin(), ::toupper);
            std::transform(context.ci_basis.begin(), context.ci_basis.end(), context.ci_basis.begin(), ::toupper);
            std::transform(context.ci_large_basis.begin(),
                           context.ci_large_basis.end(),
                           context.ci_large_basis.begin(),
                           ::toupper);
            context.ci_print_level    = parser.getString("print_level", context.ci_print_level);
            context.ci_extra_keywords = parser.getString("extra_keywords", context.ci_extra_keywords);
            context.ci_charge         = parser.getInt("charge", context.ci_charge);
            context.ci_mult           = parser.getInt("mult", context.ci_mult);
            context.ci_tail           = parser.getString("tail", context.ci_tail);
            context.ci_extension      = parser.getString("extension", context.ci_extension);
            context.ci_tschk_path     = parser.getString("tschk_path", context.ci_tschk_path);
            // Handle freeze atoms - try freeze_atoms first, then fall back to separate parameters
            std::string freeze_atoms_str = parser.getString("freeze_atoms", "");
            if (!freeze_atoms_str.empty())
            {
                // Parse freeze_atoms string (comma or space separated)
                std::vector<int> atoms;
                std::string      cleaned_str = freeze_atoms_str;
                // Remove leading/trailing whitespace
                cleaned_str.erase(cleaned_str.begin(),
                                  std::find_if(cleaned_str.begin(), cleaned_str.end(), [](unsigned char ch) {
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

                if (atoms.size() >= 2)
                {
                    context.ci_freeze_atom1 = atoms[0];
                    context.ci_freeze_atom2 = atoms[1];
                }
            }
            else
            {
                // Fall back to separate freeze_atom1 and freeze_atom2 parameters
                context.ci_freeze_atom1 = parser.getInt("freeze_atom1", context.ci_freeze_atom1);
                context.ci_freeze_atom2 = parser.getInt("freeze_atom2", context.ci_freeze_atom2);
            }

            // Load custom cycle and optimization parameters (always loaded)
            context.ci_scf_maxcycle  = parser.getInt("scf_maxcycle", context.ci_scf_maxcycle);
            context.ci_opt_maxcycles = parser.getInt("opt_maxcycles", context.ci_opt_maxcycles);
            context.ci_irc_maxpoints = parser.getInt("irc_maxpoints", context.ci_irc_maxpoints);
            context.ci_irc_recalc    = parser.getInt("irc_recalc", context.ci_irc_recalc);
            context.ci_irc_maxcycle  = parser.getInt("irc_maxcycle", context.ci_irc_maxcycle);
            context.ci_irc_stepsize  = parser.getInt("irc_stepsize", context.ci_irc_stepsize);

            std::cout << "Parameters loaded from: " << param_file << std::endl;
        }
        else
        {
            add_warning(context, "Error: Failed to load parameter file: " + param_file);
        }
    }
    else if (!arg.empty() && arg.front() == '-')
    {
        add_warning(context, "Warning: Unknown argument '" + arg + "' ignored.");
    }
    else
    {
        // Treat as positional argument (file or comma-separated files)
        // Parse comma-separated filenames and handle whitespace
        std::string file_arg = arg;
        std::replace(file_arg.begin(), file_arg.end(), ',', ' ');

        std::istringstream iss(file_arg);
        std::string        filename;
        while (iss >> filename)
        {
            if (!filename.empty())
            {
                // Trim whitespace from both ends
                filename.erase(0, filename.find_first_not_of(" \t"));
                filename.erase(filename.find_last_not_of(" \t") + 1);

                if (!filename.empty())
                {
                    context.specific_files.push_back(filename);
                }
            }
        }
    }
}

/**
 * @brief Find or create a default parameter file for create_input
 * @return Path to the parameter file, or empty string if failed
 */
std::string CommandParser::find_or_create_default_param_file()
{
    const std::string DEFAULT_PARAM_FILENAME = ".ci_parameters.params";
    const std::string ALT_PARAM_FILENAME     = "ci_parameters.params";

    std::vector<std::string> search_paths;

    // 1. Current directory
    search_paths.push_back("./" + DEFAULT_PARAM_FILENAME);
    search_paths.push_back("./" + ALT_PARAM_FILENAME);

    // 2. Executable directory
    std::string exe_dir = ConfigUtils::get_executable_directory();
    if (!exe_dir.empty())
    {
        search_paths.push_back(exe_dir + "/" + DEFAULT_PARAM_FILENAME);
        search_paths.push_back(exe_dir + "/" + ALT_PARAM_FILENAME);
    }

    // 3. User home directory
    std::string home_dir = g_config_manager.get_user_home_directory();
    if (!home_dir.empty())
    {
        search_paths.push_back(home_dir + "/" + DEFAULT_PARAM_FILENAME);
        search_paths.push_back(home_dir + "/" + ALT_PARAM_FILENAME);
    }

    // 4. System config directories (Unix-like systems)
#ifndef _WIN32
    search_paths.push_back("/etc/gaussian_extractor/" + ALT_PARAM_FILENAME);
    search_paths.push_back("/usr/local/etc/" + ALT_PARAM_FILENAME);
#endif

    // Search for existing parameter file
    for (const auto& path : search_paths)
    {
        if (std::filesystem::exists(path))
        {
            std::cout << "Found default parameter file: " << path << std::endl;
            return path;
        }
    }

    // No existing file found, create default in current directory
    std::string     default_path = "./" + DEFAULT_PARAM_FILENAME;
    ParameterParser parser;

    if (parser.generateTemplate("sp", default_path))
    {
        std::cout << "Created default parameter file: " << default_path << std::endl;
        std::cout << "Using default parameters from newly created file." << std::endl;
        return default_path;
    }
    else
    {
        std::cerr << "Error: Failed to create default parameter file: " << default_path << std::endl;
        return "";
    }
}
