#include "command_system.h"

#include "config_manager.h"
#include "version.h"
#include <iostream>
#include <algorithm>
#include <thread>

#include <string>
#include <cstdlib>

CommandContext CommandParser::parse(int argc, char* argv[]) {
    // Early check for version before any other processing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--version" || arg == "-v") {
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
    if (argc == 1) {
        validate_context(context);
        return context;
    }

    // Scan all arguments to find a command (flexible positioning)
    CommandType found_command = CommandType::EXTRACT;
    int command_index = -1;
    
    for (int i = 1; i < argc; ++i) {
        CommandType potential_command = parse_command(argv[i]);
        if (potential_command != CommandType::EXTRACT || std::string(argv[i]) == "extract") {
            found_command = potential_command;
            command_index = i;
            break;
        }
    }
    
    context.command = found_command;

    // Parse all arguments, skipping the command if found
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        // Skip the command argument itself
        if (i == command_index) {
            continue;
        }

        if (arg == "-h" || arg == "--help") {
            if (context.command == CommandType::EXTRACT) {
                print_help(argv[0]);
            } else {
                print_command_help(context.command, argv[0]);
            }
            std::exit(0);
        }

        else if (arg == "--config-help") {
            print_config_help();
            std::exit(0);
        }
        else if (arg == "--create-config") {
            create_default_config();
            std::exit(0);
        }
        else if (arg == "--show-config") {
            g_config_manager.print_config_summary(true);
            std::exit(0);
        }

        // Parse common options first
        parse_common_options(context, i, argc, argv);

        // Parse command-specific options
        if (context.command == CommandType::EXTRACT || 
            context.command == CommandType::HIGH_LEVEL_KJ || 
            context.command == CommandType::HIGH_LEVEL_AU) {
            parse_extract_options(context, i, argc, argv);
        } else {
            parse_checker_options(context, i, argc, argv);
        }
    }

    validate_context(context);
    return context;
}

CommandType CommandParser::parse_command(const std::string& cmd) {
    if (cmd == "extract") return CommandType::EXTRACT;
    if (cmd == "done") return CommandType::CHECK_DONE;
    if (cmd == "errors") return CommandType::CHECK_ERRORS;
    if (cmd == "pcm") return CommandType::CHECK_PCM;
    if (cmd == "check") return CommandType::CHECK_ALL;
    if (cmd == "high-kj" || cmd == "--high-level-kj") return CommandType::HIGH_LEVEL_KJ;
    if (cmd == "high-au" || cmd == "--high-level-au") return CommandType::HIGH_LEVEL_AU;

    // If it starts with '-', it's probably an option, not a command
    if (!cmd.empty() && cmd.front() == '-') return CommandType::EXTRACT;

    // Default to extract for backward compatibility
    return CommandType::EXTRACT;
}

std::string CommandParser::get_command_name(CommandType command) {
    switch (command) {
        case CommandType::EXTRACT: return std::string("extract");
        case CommandType::CHECK_DONE: return std::string("done");
        case CommandType::CHECK_ERRORS: return std::string("errors");
        case CommandType::CHECK_PCM: return std::string("pcm");
        case CommandType::CHECK_ALL: return std::string("check");
        case CommandType::HIGH_LEVEL_KJ: return std::string("high-kj");
        case CommandType::HIGH_LEVEL_AU: return std::string("high-au");
        default: return std::string("unknown");
    }
}

void CommandParser::parse_common_options(CommandContext& context, int& i, int argc, char* argv[]) {
    std::string arg = argv[i];

    if (arg == "-q" || arg == "--quiet") {
        context.quiet = true;
    }
    else if (arg == "-e" || arg == "--ext") {
        if (++i < argc) {
            std::string ext = argv[i];
            std::string full_ext = (ext[0] == '.') ? ext : ("." + ext);
            if (g_config_manager.is_valid_output_extension(full_ext)) {
                context.extension = full_ext;
            } else {
                add_warning(context, "Error: Extension '" + ext + "' not in configured output extensions. Using default.");
                context.extension = g_config_manager.get_default_output_extension();
            }
        } else {
            add_warning(context, "Error: Extension value required after -e/--ext.");
        }
    }
    else if (arg == "-nt" || arg == "--threads") {
        if (++i < argc) {
            std::string threads_arg = argv[i];
            unsigned int hardware_cores = std::thread::hardware_concurrency();
            if (hardware_cores == 0) hardware_cores = 4;

            if (threads_arg == "max") {
                context.requested_threads = hardware_cores;
            } else if (threads_arg == "half") {
                context.requested_threads = std::max(1u, hardware_cores / 2);
            } else {
                try {
                    unsigned int req_threads = std::stoul(threads_arg);
                    if (req_threads == 0) {
                        add_warning(context, "Error: Thread count must be at least 1. Using configured default.");
                        context.requested_threads = g_config_manager.get_default_threads();
                    } else {
                        context.requested_threads = req_threads;
                    }
                } catch (const std::exception& e) {
                    add_warning(context, "Error: Invalid thread count format. Using configured default.");
                    context.requested_threads = g_config_manager.get_default_threads();
                }
            }
        } else {
            add_warning(context, "Error: Thread count required after -nt/--threads.");
        }
    }
    else if (arg == "--max-file-size") {
        if (++i < argc) {
            try {
                int size = std::stoi(argv[i]);
                if (size <= 0) {
                    add_warning(context, "Error: Max file size must be positive. Using default 100MB.");
                } else {
                    context.max_file_size_mb = static_cast<size_t>(size);
                }
            } catch (const std::exception& e) {
                add_warning(context, "Error: Invalid max file size format. Using default 100MB.");
            }
        } else {
            add_warning(context, "Error: Max file size value required after --max-file-size.");
        }
    }
}

void CommandParser::parse_extract_options(CommandContext& context, int& i, int argc, char* argv[]) {
    std::string arg = argv[i];

    if (arg == "-t" || arg == "--temp") {
        if (++i < argc) {
            try {
                context.temp = std::stod(argv[i]);
                if (context.temp <= 0) {
                    add_warning(context, "Warning: Temperature must be positive. Using default 298.15 K.");
                    context.temp = 298.15;
                } else {
                    context.use_input_temp = true;
                }
            } catch (const std::exception& e) {
                add_warning(context, "Error: Invalid temperature format. Using default 298.15 K.");
                context.temp = 298.15;
            }
        } else {
            add_warning(context, "Error: Temperature value required after -t/--temp.");
        }
    }
    else if (arg == "-c" || arg == "--cm") {
        if (++i < argc) {
            try {
                int conc = std::stoi(argv[i]);
                if (conc <= 0) {
                    add_warning(context, "Error: Concentration must be positive. Using configured default.");
                    context.concentration = static_cast<int>(g_config_manager.get_default_concentration() * 1000);
                } else {
                    context.concentration = conc * 1000;
                }
            } catch (const std::exception& e) {
                add_warning(context, "Error: Invalid concentration format. Using configured default.");
                context.concentration = static_cast<int>(g_config_manager.get_default_concentration() * 1000);
            }
        } else {
            add_warning(context, "Error: Concentration value required after -c/--cm.");
        }
    }
    else if (arg == "-col" || arg == "--column") {
        if (++i < argc) {
            try {
                int col = std::stoi(argv[i]);
                if (col >= 1 && col <= 10) {
                    context.sort_column = col;
                } else {
                    add_warning(context, "Error: Column must be between 1-10. Using default column 2.");
                }
            } catch (const std::exception& e) {
                add_warning(context, "Error: Invalid column format. Using default column 2.");
            }
        } else {
            add_warning(context, "Error: Column value required after -col/--column.");
        }
    }
    else if (arg == "-f" || arg == "--format") {
        if (++i < argc) {
            std::string fmt = argv[i];
            if (fmt == "text" || fmt == "csv") {
                context.output_format = fmt;
            } else {
                add_warning(context, "Error: Format must be 'text' or 'csv'. Using default 'text'.");
                context.output_format = "text";
            }
        } else {
            add_warning(context, "Error: Format value required after -f/--format.");
        }
    }
    else if (arg == "--memory-limit") {
        if (++i < argc) {
            try {
                int size = std::stoi(argv[i]);
                if (size <= 0) {
                    add_warning(context, "Error: Memory limit must be positive. Using auto-calculated limit.");
                } else {
                    context.memory_limit_mb = static_cast<size_t>(size);
                }
            } catch (const std::exception& e) {
                add_warning(context, "Error: Invalid memory limit format. Using auto-calculated limit.");
            }
        } else {
            add_warning(context, "Error: Memory limit value required after --memory-limit.");
        }
    }
    else if (arg == "--resource-info") {
        context.show_resource_info = true;
    }
    else if (!arg.empty() && arg.front() == '-') {
        add_warning(context, "Warning: Unknown argument '" + arg + "' ignored.");
    }
}

void CommandParser::parse_checker_options(CommandContext& context, int& i, int argc, char* argv[]) {
    std::string arg = argv[i];

    if (arg == "--target-dir") {
        if (++i < argc) {
            context.target_dir = argv[i];
        } else {
            add_warning(context, "Error: Target directory name required after --target-dir.");
        }
    }
    else if (arg == "--dir-suffix") {
        if (++i < argc) {
            context.dir_suffix = argv[i];
        } else {
            add_warning(context, "Error: Directory suffix required after --dir-suffix.");
        }
    }
    else if (arg == "--show-details") {
        context.show_error_details = true;
    }
    else if (!arg.empty() && arg.front() == '-') {
        add_warning(context, "Warning: Unknown argument '" + arg + "' ignored.");
    }
}

void CommandParser::add_warning(CommandContext& context, const std::string& warning) {
    context.warnings.push_back(warning);
}

void CommandParser::validate_context(CommandContext& context) {
    // Set default threads if not specified
    if (context.requested_threads == 0) {
        context.requested_threads = g_config_manager.get_default_threads();
    }

    // Validate file size limits
    if (context.max_file_size_mb == 0) {
        context.max_file_size_mb = g_config_manager.get_default_max_file_size();
    }
}

void CommandParser::apply_config_to_context(CommandContext& context) {
    // Apply configuration defaults to context
    if (!g_config_manager.is_config_loaded()) {
        return; // Keep built-in defaults if config not loaded
    }

    // Apply configuration values
    context.quiet = g_config_manager.get_bool("quiet_mode");
    context.requested_threads = g_config_manager.get_default_threads();
    context.max_file_size_mb = g_config_manager.get_default_max_file_size();
    context.extension = g_config_manager.get_default_output_extension();
    context.temp = g_config_manager.get_default_temperature();
    context.concentration = static_cast<int>(g_config_manager.get_default_concentration() * 1000);
    context.sort_column = g_config_manager.get_int("default_sort_column");
    context.output_format = g_config_manager.get_default_output_format();
    context.use_input_temp = g_config_manager.get_bool("use_input_temp");
    context.memory_limit_mb = g_config_manager.get_size_t("memory_limit_mb");
    context.show_error_details = g_config_manager.get_bool("show_error_details");
    context.dir_suffix = g_config_manager.get_string("done_directory_suffix");
}

void CommandParser::load_configuration() {
    // Try to load configuration file
    if (!g_config_manager.is_config_loaded()) {
        g_config_manager.load_config();
    }
}

void CommandParser::print_config_help() {
    std::cout << "Gaussian Extractor Configuration System\n\n";
    std::cout << "Configuration file locations (searched in order):\n";
    std::cout << "Configuration file locations (searched in order):\n";
    std::cout << "  1. ./gaussian_extractor.conf\n";
    std::cout << "  2. ./.gaussian_extractor.conf\n";
    std::cout << "  3. ~/.gaussian_extractor.conf\n";
    std::cout << "  4. ~/gaussian_extractor.conf\n";
    #ifndef _WIN32
    std::cout << "  5. /etc/gaussian_extractor/gaussian_extractor.conf\n";
    std::cout << "  6. /usr/local/etc/gaussian_extractor.conf\n";
    #endif
    std::cout << std::endl;

    std::cout << "Commands:\n";
    std::cout << "  --show-config     Show current configuration\n";
    std::cout << "  --create-config   Create default configuration file\n";
    std::cout << "  --config-help     Show this configuration help\n\n";

    std::cout << "Configuration file format:\n";
    std::cout << "  # Lines starting with # are comments\n";
    std::cout << "  key = value\n";
    std::cout << "  # Values can be quoted: key = \"value with spaces\"\n\n";

    std::cout << "Example configuration entries:\n";
    std::cout << "  default_temperature = 298.15\n";
    std::cout << "  default_concentration = 2.0\n";
    std::cout << "  output_extensions = .log,.out\n";
    std::cout << "  input_extensions = .com,.gjf,.gau\n";
    std::cout << "  default_threads = 4\n\n";
}

void CommandParser::create_default_config() {
    std::cout << "Creating default configuration file...\n";

    if (g_config_manager.create_default_config_file()) {
        std::string home_dir = g_config_manager.get_user_home_directory();
        if (!home_dir.empty()) {
            std::cout << "Configuration file created at: " << home_dir << "/.gaussian_extractor.conf" << std::endl;
        } else {
            std::cout << "Configuration file created at: ./.gaussian_extractor.conf" << std::endl;
        }
        std::cout << "Edit this file to customize your default settings.\n";
    } else {
        std::cout << "Failed to create configuration file.\n";
        std::cout << "You can create it manually using the template below:\n\n";
        g_config_manager.print_config_file_template();
    }
}

std::unordered_map<std::string, std::string> CommandParser::extract_config_overrides(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> overrides;

    for (int i = 1; i < argc - 1; ++i) {
        std::string arg = argv[i];
        if (arg.substr(0, 9) == "--config-") {
            std::string key = arg.substr(9);
            std::string value = argv[i + 1];
            overrides[key] = value;
            ++i; // Skip the value argument
        }
    }

    return overrides;
}

void CommandParser::print_help(const std::string& program_name) {
    std::cout << "Gaussian Extractor - Parallel Energy Extraction and Job Management Tool\n\n";
    std::cout << "Usage: " << program_name << " [COMMAND] [OPTIONS]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  extract           Extract energies from Gaussian log files (default)\n";
    std::cout << "  done              Check and move completed jobs to {dir}-done/\n";
    std::cout << "  errors            Check and move error jobs to errorJobs/\n";
    std::cout << "  pcm               Check and move PCM convergence failures to PCMMkU/\n";
    std::cout << "  check             Run all job checks (done, errors, pcm)\n";
    std::cout << "  high-kj           Calculate high-level energies with output in kJ/mol\n";
    std::cout << "  --high-level-kj   (same as high-kj)\n";
    std::cout << "  high-au           Calculate high-level energies with detailed output in atomic units\n";
    std::cout << "  --high-level-au   (same as high-au)\n\n";

    std::cout << "Common Options:\n";
    auto output_exts = g_config_manager.get_output_extensions();
    std::string ext_list;
    for (size_t i = 0; i < output_exts.size(); ++i) {
        if (i > 0) ext_list += ",";
        ext_list += output_exts[i];
    }
    std::cout << "  -e, --ext <ext>       File extension (configured: " << ext_list << ")\n";
    std::cout << "  -nt, --threads <N>    Thread count: number|max|half (default: " << g_config_manager.get_string("default_threads") << ")\n";
    std::cout << "  -q, --quiet           Quiet mode (default: " << (g_config_manager.get_bool("quiet_mode") ? "on" : "off") << ")\n";
    std::cout << "  --max-file-size <MB>  Maximum file size in MB (default: " << g_config_manager.get_default_max_file_size() << ")\n";
    std::cout << "  -h, --help            Show this help message\n";
    std::cout << "  -v, --version         Show version information\n\n";

    std::cout << "Extract Command Options:\n";
    std::cout << "  -t, --temp <K>        Temperature in Kelvin (default: " << g_config_manager.get_default_temperature() << ")\n";
    std::cout << "  -c, --cm <M>          Concentration in M (default: " << g_config_manager.get_default_concentration() << ")\n";
    std::cout << "  -col, --column <N>    Sort column 1-10 (default: " << g_config_manager.get_int("default_sort_column") << ")\n";
    std::cout << "  -f, --format <fmt>    Output format: text|csv (default: " << g_config_manager.get_default_output_format() << ")\n";
    std::cout << "  --memory-limit <MB>   Memory limit in MB (default: auto-calculated)\n";
    std::cout << "  --resource-info       Show resource information\n\n";

    std::cout << "Job Checker Options:\n";
    std::cout << "  --target-dir <name>   Custom target directory name\n";
    std::cout << "  --dir-suffix <suffix> Custom suffix for done directory (default: " << g_config_manager.get_string("done_directory_suffix") << ")\n";
    std::cout << "  --show-details        Show error details (default: " << (g_config_manager.get_bool("show_error_details") ? "on" : "off") << ")\n\n";

    std::cout << "Configuration Options:\n";
    std::cout << "  --show-config         Show current configuration settings\n";
    std::cout << "  --create-config       Create default configuration file\n";
    std::cout << "  --config-help         Show configuration system help\n\n";

    std::cout << "Examples:\n";
    std::cout << "  " << program_name << "                    # Extract energies (default)\n";
    std::cout << "  " << program_name << " extract -t 300    # Extract with 300K temperature\n";
    std::cout << "  " << program_name << " done              # Move completed jobs\n";
    std::cout << "  " << program_name << " errors -q         # Check errors quietly\n";
    std::cout << "  " << program_name << " pcm -nt 4         # Check PCM failures with 4 threads\n";
    std::cout << "  " << program_name << " check             # Run all checks\n\n";
}

void CommandParser::print_command_help(CommandType command, const std::string& program_name) {
    std::string cmd_name = get_command_name(command);

    std::cout << "Gaussian Extractor - " << cmd_name << " command\n\n";
    std::cout << "Usage: " << program_name << " " << cmd_name << " [OPTIONS]\n\n";

    switch (command) {
        case CommandType::CHECK_DONE:
            std::cout << "Description: Check for completed Gaussian jobs (normal termination)\n";
            std::cout << "             and move them to {current_directory}-done/\n\n";
            std::cout << "This command looks for 'Normal termination of Gaussian' in the last 10 lines\n";
            std::cout << "of each log file and moves completed jobs along with their .gau and .chk files.\n\n";
            break;

        case CommandType::CHECK_ERRORS:
            std::cout << "Description: Check for Gaussian jobs that terminated with errors\n";
            std::cout << "             and move them to errorJobs/\n\n";
            std::cout << "This command looks for error messages in log files (excluding 'Error on' messages)\n";
            std::cout << "and moves error jobs along with their .gau and .chk files.\n\n";
            break;

        case CommandType::CHECK_PCM:
            std::cout << "Description: Check for jobs that failed due to PCM convergence issues\n";
            std::cout << "             and move them to PCMMkU/\n\n";
            std::cout << "This command looks for 'failed in PCMMkU' messages in log files\n";
            std::cout << "and moves failed jobs along with their .gau and .chk files.\n\n";
            break;

        case CommandType::CHECK_ALL:
            std::cout << "Description: Run all job checks (done, errors, pcm) in sequence\n\n";
            break;

        case CommandType::HIGH_LEVEL_KJ:
            std::cout << "Description: Calculate high-level energies with output in kJ/mol units\n";
            std::cout << "             Uses high-level electronic energy combined with low-level thermal corrections\n\n";
            std::cout << "This command reads high-level electronic energies from current directory\n";
            std::cout << "and thermal corrections from parent directory (../) to compute final\n";
            std::cout << "thermodynamic quantities. Output format focuses on final Gibbs energies.\n\n";
            std::cout << "Additional Options:\n";
            std::cout << "  -t, --temp <K>        Temperature in Kelvin (default: from input or 298.15)\n";
            std::cout << "  -c, --concentration <M> Concentration in M for phase correction (default: 1.0)\n";
            std::cout << "  -f, --format <fmt>    Output format: text|csv (default: text)\n";
            std::cout << "  -col, --column <N>    Sort column 1-7 (default: 2)\n";
            std::cout << "                        1=Name, 2=G kJ/mol, 3=G a.u, 4=G eV, 5=LowFQ, 6=Status, 7=PhCorr\n\n";
            break;

        case CommandType::HIGH_LEVEL_AU:
            std::cout << "Description: Calculate detailed energy components in atomic units\n";
            std::cout << "             Uses high-level electronic energy combined with low-level thermal corrections\n\n";
            std::cout << "This command reads high-level electronic energies from current directory\n";
            std::cout << "and thermal corrections from parent directory (../) to compute detailed\n";
            std::cout << "energy component breakdown including ZPE, TC, TS, H, and G values.\n\n";
            std::cout << "Additional Options:\n";
            std::cout << "  -t, --temp <K>        Temperature in Kelvin (default: from input or 298.15)\n";
            std::cout << "  -c, --concentration <M> Concentration in M for phase correction (default: 1.0)\n";
            std::cout << "  -f, --format <fmt>    Output format: text|csv (default: text)\n";
            std::cout << "  -col, --column <N>    Sort column 1-10 (default: 2)\n";
            std::cout << "                        1=Name, 2=E high, 3=E low, 4=ZPE, 5=TC, 6=TS, 7=H, 8=G, 9=LowFQ, 10=PhCorr\n\n";
            break;

        default:
            break;
    }

    std::cout << "Options:\n";
    std::cout << "  -e, --ext <ext>       File extension: log|out (default: log)\n";
    std::cout << "  -nt, --threads <N>    Thread count: number|max|half (default: half)\n";
    std::cout << "  -q, --quiet           Quiet mode (minimal output)\n";
    std::cout << "  --max-file-size <MB>  Maximum file size in MB (default: 100)\n";

    if (command == CommandType::CHECK_DONE) {
        std::cout << "  --dir-suffix <suffix> Directory suffix (default: done)\n";
        std::cout << "                        Creates {current_dir}-{suffix}/\n";
    }

    if (command == CommandType::CHECK_ERRORS || command == CommandType::CHECK_PCM) {
        std::cout << "  --target-dir <name>   Custom target directory name\n";
    }

    if (command == CommandType::CHECK_ERRORS) {
        std::cout << "  --show-details        Show actual error messages found\n";
    }

    std::cout << "  -h, --help            Show this help message\n";
    std::cout << "  -v, --version         Show version information\n\n";

    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " " << cmd_name << "              # Basic usage\n";
    std::cout << "  " << program_name << " " << cmd_name << " -q           # Quiet mode\n";
    if (command == CommandType::HIGH_LEVEL_KJ) {
        std::cout << "  " << program_name << " " << cmd_name << " -col 5       # Sort by frequency\n";
        std::cout << "  " << program_name << " " << cmd_name << " -t 300 -col 2 -f csv  # Custom temp, sort by G kJ/mol, CSV\n";
    } else if (command == CommandType::HIGH_LEVEL_AU) {
        std::cout << "  " << program_name << " " << cmd_name << " -col 8       # Sort by Gibbs energy\n";
        std::cout << "  " << program_name << " " << cmd_name << " -t 273 -col 4 -f csv  # Custom temp, sort by ZPE, CSV\n";
    } else {

    }
    std::cout << "  " << program_name << " " << cmd_name << " -nt 4        # Use 4 threads\n";

    if (command == CommandType::CHECK_DONE) {
        std::cout << "  " << program_name << " " << cmd_name << " --dir-suffix completed  # Use 'completed' suffix\n";
    }

    std::cout << "\n";
}
