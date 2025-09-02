/**
 * @file main.cpp
 * @brief Main entry point for the Gaussian Extractor application
 * @author Le Nhan Pham
 * @date 2025
 *
 * This file contains the main function and signal handling for the Gaussian Extractor,
 * a high-performance tool for processing Gaussian computational chemistry log files.
 * The application supports various commands including extraction, job checking, and
 * high-level energy calculations.
 *
 * @section Features
 * - Multi-threaded file processing with resource management
 * - Job scheduler integration (SLURM, PBS, SGE, LSF)
 * - Comprehensive error detection and job status checking
 * - High-level energy calculations with thermal corrections
 * - Configurable through configuration files and command-line options
 * - Graceful shutdown handling for long-running operations
 */

#include "core/command_system.h"
#include "core/config_manager.h"
#include "core/gaussian_extractor.h"
#include "core/job_scheduler.h"
#include "core/module_executor.h"
#include "core/version.h"
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief Global flag to indicate when a shutdown has been requested
 *
 * This atomic boolean is used to coordinate graceful shutdown across all threads
 * when a termination signal (SIGINT, SIGTERM) is received. All long-running
 * operations should periodically check this flag and terminate cleanly.
 */
std::atomic<bool> g_shutdown_requested{false};

/**
 * @brief Signal handler for graceful shutdown
 *
 * This function is called when the application receives termination signals
 * (SIGINT from Ctrl+C, SIGTERM from system shutdown, etc.). It sets the global
 * shutdown flag to coordinate clean termination of all threads and operations.
 *
 * @param signal The signal number that was received
 *
 * @note This function is signal-safe and only performs async-signal-safe operations
 * @see g_shutdown_requested
 */
void signalHandler(int signal)
{
    std::cerr << "\nReceived signal " << signal << ". Initiating graceful shutdown..." << std::endl;
    g_shutdown_requested.store(true);
}

/**
 * @brief Check if a command is a shell command that should be executed directly
 * @param cmd Command string to check
 * @return true if the command is a shell command, false otherwise
 */
bool is_shell_command(const std::string& cmd);

/**
 * @brief Check if a command is a directory-related command
 * @param cmd Command string to check
 * @return true if the command is a directory command, false otherwise
 */
bool is_directory_command(const std::string& cmd);

/**
 * @brief Check if a command is a utility command that needs special handling
 * @param cmd Command string to check
 * @return true if the command is a utility command, false otherwise
 */
bool is_utility_command(const std::string& cmd);

/**
 * @brief Execute a utility command
 * @param cmd The utility command to execute
 * @return Exit code (0 for success, non-zero for failure)
 */
int execute_utility_command(const std::string& cmd);

/**
 * @brief Execute a directory-related command natively
 * @param cmd Command string to execute
 * @return exit code (0 for success, non-zero for failure)
 */
int execute_directory_command(const std::string& cmd);

/**
 * @brief Interactive command loop for Windows double-click usage
 *
 * This function provides an interactive shell where users can enter commands
 * after the initial EXTRACT operation. It reads commands from stdin and
 * processes them using the existing command parsing system.
 *
 * @return Exit code when the interactive loop terminates
 */
int run_interactive_loop()
{
    std::cout << "\nGaussian Extractor Interactive Mode" << std::endl;
    std::cout << "Developed by Le Nhan Pham" << std::endl;
    std::cout << "https://github.com/lenhanpham/gaussian-extractor" << std::endl;
    std::cout << "\nType 'help' for available commands, 'exit' or 'quit' to exit." << std::endl;
    std::cout << "You can also use shell commands like 'cd', 'dir', etc." << std::endl;
    std::cout << ">> ";

    std::string line;
    while (std::getline(std::cin, line))
    {
        // Trim whitespace and carriage return
        size_t start = line.find_first_not_of(" \t\r");
        if (start != std::string::npos)
        {
            line.erase(0, start);
        }
        else
        {
            line.clear();
        }
        size_t end = line.find_last_not_of(" \t\r");
        if (end != std::string::npos)
        {
            line.erase(end + 1);
        }

        if (line.empty())
        {
            std::cout << ">> ";
            continue;
        }

        if (line == "exit" || line == "quit")
        {
            std::cout << "Exiting Gaussian Extractor. Goodbye!" << std::endl;
#ifdef _WIN32
            std::cout << std::endl << "Press Enter to exit..." << std::endl;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
#endif
            return 0;
        }

        // Handle special commands that would normally exit
        if (line == "help" || line == "--help" || line == "-h")
        {
            std::cout << "\nAvailable commands:" << std::endl;
            std::cout << "  extract           Extract thermodynamic data from log files" << std::endl;
            std::cout << "  done              Check and organize completed jobs" << std::endl;
            std::cout << "  errors            Check and organize failed jobs" << std::endl;
            std::cout << "  pcm               Check PCM convergence failures" << std::endl;
            std::cout << "  imode             Check jobs with imaginary frequencies" << std::endl;
            std::cout << "  check             Run all job checks" << std::endl;
            std::cout << "  high-kj           Calculate high-level energies in kJ/mol" << std::endl;
            std::cout << "  high-au           Calculate high-level energies in atomic units" << std::endl;
            std::cout << "  xyz               Extract coordinates to XYZ format" << std::endl;
            std::cout << "  help              Show this help message" << std::endl;
            std::cout << "  --version         Show version information" << std::endl;
            std::cout << "  --config-help     Show configuration help" << std::endl;
            std::cout << "  --create-config   Create default configuration file" << std::endl;
            std::cout << "  --show-config     Show current configuration" << std::endl;
            std::cout << "\nYou can use command-line options with any command." << std::endl;
            std::cout << "Example: extract -q -nt 4" << std::endl;
            std::cout << "\nYou can also use shell commands like 'cd', 'ls', 'pwd', etc." << std::endl;
        }
        else if (line == "--version" || line == "-v")
        {
            std::cout << GaussianExtractor::get_version_info() << std::endl;
        }
        else if (line == "--config-help")
        {
            CommandParser::print_config_help();
        }
        else if (line == "--create-config")
        {
            CommandParser::create_default_config();
        }
        else if (line == "--show-config")
        {
            g_config_manager.print_config_summary(true);
        }
        // Handle help commands with arguments
        else if (line.find("help ") == 0 || line.find("--help ") == 0)
        {
            std::string help_arg;
            if (line.find("help ") == 0)
            {
                help_arg = line.substr(5);  // Remove "help "
            }
            else if (line.find("--help ") == 0)
            {
                help_arg = line.substr(7);  // Remove "--help "
            }

            // Trim whitespace
            size_t start = help_arg.find_first_not_of(" \t\r");
            if (start != std::string::npos)
            {
                help_arg = help_arg.substr(start);
            }
            size_t end = help_arg.find_last_not_of(" \t\r");
            if (end != std::string::npos)
            {
                help_arg = help_arg.substr(0, end + 1);
            }

            // Map command names to CommandType
            static const std::map<std::string, CommandType> command_map = {{"extract", CommandType::EXTRACT},
                                                                           {"done", CommandType::CHECK_DONE},
                                                                           {"errors", CommandType::CHECK_ERRORS},
                                                                           {"pcm", CommandType::CHECK_PCM},
                                                                           {"imode", CommandType::CHECK_IMAGINARY},
                                                                           {"check", CommandType::CHECK_ALL},
                                                                           {"high-kj", CommandType::HIGH_LEVEL_KJ},
                                                                           {"high-au", CommandType::HIGH_LEVEL_AU},
                                                                           {"xyz", CommandType::EXTRACT_COORDS}};

            auto it = command_map.find(help_arg);
            if (it != command_map.end())
            {
                CommandParser::print_command_help(it->second);
            }
            else if (!help_arg.empty())
            {
                std::cout << "Unknown command: " << help_arg << std::endl;
                std::cout << "Type 'help' for a list of available commands." << std::endl;
            }
            else
            {
                // Just "help" or "--help" without arguments
                CommandParser::print_help();
            }
        }
        else if (is_shell_command(line))
        {
            // Handle directory commands natively
            if (is_directory_command(line))
            {
                // std::cout << "DEBUG: Executing directory command: " << line << std::endl;
                int result = execute_directory_command(line);
                if (result != 0)
                {
                    std::cout << "Directory command failed with code: " << result << std::endl;
                }
            }
            // Handle utility commands specially
            else if (is_utility_command(line))
            {
                // std::cout << "DEBUG: Executing utility command: " << line << std::endl;
                int result = execute_utility_command(line);
                if (result != 0)
                {
                    std::cout << "Utility command failed with code: " << result << std::endl;
                }
            }
            else
            {
                // Execute as regular shell command
                // std::cout << "DEBUG: Executing shell command: " << line << std::endl;
                int result = std::system(line.c_str());
                if (result != 0)
                {
                    std::cout << "Command exited with code: " << result << std::endl;
                }
            }
        }
        else
        {
            // Check if this could be a valid Gaussian command
            std::istringstream iss(line);
            std::string        first_arg;
            iss >> first_arg;

            // List of valid Gaussian commands
            static const std::vector<std::string> valid_commands = {"extract",
                                                                    "done",
                                                                    "errors",
                                                                    "pcm",
                                                                    "imode",
                                                                    "--imaginary",
                                                                    "check",
                                                                    "high-kj",
                                                                    "--high-level-kj",
                                                                    "high-au",
                                                                    "--high-level-au",
                                                                    "xyz",
                                                                    "--extract-coord"};

            bool is_valid_gaussian_command = false;
            for (const auto& cmd : valid_commands)
            {
                if (first_arg == cmd)
                {
                    is_valid_gaussian_command = true;
                    break;
                }
            }

            if (is_valid_gaussian_command)
            {
                // Parse and execute as Gaussian command
                std::vector<char*> args;
                try
                {
                    // Split the line into arguments
                    std::string        arg;
                    std::istringstream iss_full(line);
                    args.push_back(const_cast<char*>("gaussian_extractor"));  // Program name

                    while (iss_full >> arg)
                    {
                        char* cstr = new char[arg.size() + 1];
                        strcpy(cstr, arg.c_str());
                        args.push_back(cstr);
                    }

                    if (args.size() > 1)
                    {
                        // Parse the command
                        CommandContext context = CommandParser::parse(args.size(), args.data());

                        // Show warnings if any
                        if (!context.warnings.empty() && !context.quiet)
                        {
                            for (const auto& warning : context.warnings)
                            {
                                std::cerr << warning << std::endl;
                            }
                            std::cerr << std::endl;
                        }

                        // Execute based on command type
                        int result = 0;
                        switch (context.command)
                        {
                            case CommandType::EXTRACT:
                                result = execute_extract_command(context);
                                break;
                            case CommandType::CHECK_DONE:
                                result = execute_check_done_command(context);
                                break;
                            case CommandType::CHECK_ERRORS:
                                result = execute_check_errors_command(context);
                                break;
                            case CommandType::CHECK_PCM:
                                result = execute_check_pcm_command(context);
                                break;
                            case CommandType::CHECK_IMAGINARY:
                                result = execute_check_imaginary_command(context);
                                break;
                            case CommandType::CHECK_ALL:
                                result = execute_check_all_command(context);
                                break;
                            case CommandType::HIGH_LEVEL_KJ:
                                result = execute_high_level_kj_command(context);
                                break;
                            case CommandType::HIGH_LEVEL_AU:
                                result = execute_high_level_au_command(context);
                                break;
                            case CommandType::EXTRACT_COORDS:
                                result = execute_extract_coords_command(context);
                                break;
                            default:
                                std::cerr << "Unknown command" << std::endl;
                                result = 1;
                        }

                        if (result != 0)
                        {
                            std::cerr << "Command failed with exit code: " << result << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "Please enter a command. Type 'help' for available commands." << std::endl;
                    }

                    // Clean up allocated strings
                    for (size_t i = 1; i < args.size(); ++i)
                    {
                        delete[] args[i];
                    }
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error parsing command: " << e.what() << std::endl;
                }
                catch (...)
                {
                    std::cerr << "Unknown error occurred while parsing command" << std::endl;
                }
            }
            else
            {
                // Unknown command
                std::cout << "Unknown command: " << first_arg << std::endl;
                std::cout << "Type 'help' for a list of available commands." << std::endl;
            }
        }

        std::cout << ">> ";
    }

    // Loop exited (EOF or error)
    std::cout << "Exiting Gaussian Extractor. Goodbye!" << std::endl;
#ifdef _WIN32
    std::cout << std::endl << "Press Enter to exit..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
#endif

    return 0;
}

bool is_shell_command(const std::string& cmd)
{
    // Common shell commands that should be executed directly
    static const std::vector<std::string> shell_commands = {
        "cd",    "ls",     "dir",  "pwd",   "mkdir", "rmdir", "cp",     "mv",      "rm",      "cat",   "more",   "less",
        "head",  "tail",   "grep", "find",  "which", "echo",  "date",   "time",    "history", "alias", "export", "set",
        "unset", "source", "bash", "sh",    "zsh",   "fish",  "python", "python3", "pip",     "pip3",  "git",    "make",
        "cmake", "gcc",    "g++",  "clang", "vim",   "nano",  "emacs",  "code",    "subl",    "atom"};

    // Check if command starts with any shell command
    for (const auto& shell_cmd : shell_commands)
    {
        if (cmd.find(shell_cmd) == 0)
        {
            // Make sure it's a complete word (followed by space or end)
            size_t cmd_len = shell_cmd.length();
            if (cmd.length() == cmd_len || cmd[cmd_len] == ' ')
            {
                return true;
            }
        }
    }

    return false;
}

bool is_directory_command(const std::string& cmd)
{
    // Commands that change the current working directory or show it
    static const std::vector<std::string> dir_commands = {"cd", "pushd", "popd", "pwd", "ls", "dir"};

    // Check if command starts with any directory command
    for (const auto& dir_cmd : dir_commands)
    {
        if (cmd.find(dir_cmd) == 0)
        {
            // Make sure it's a complete word (followed by space or end)
            size_t cmd_len = dir_cmd.length();
            if (cmd.length() == cmd_len || cmd[cmd_len] == ' ')
            {
                return true;
            }
        }
    }

    return false;
}

bool is_utility_command(const std::string& cmd)
{
    // Commands that provide utility functions and should be handled specially
    static const std::vector<std::string> utility_commands = {"which"};

    // Check if command starts with any utility command
    for (const auto& util_cmd : utility_commands)
    {
        if (cmd.find(util_cmd) == 0)
        {
            // Make sure it's a complete word (followed by space or end)
            size_t cmd_len = util_cmd.length();
            if (cmd.length() == cmd_len || cmd[cmd_len] == ' ')
            {
                return true;
            }
        }
    }

    return false;
}

int execute_utility_command(const std::string& cmd)
{
    try
    {
        if (cmd == "which")
        {
            std::cout << "which: missing argument" << std::endl;
            std::cout << "Usage: which <command>" << std::endl;
            return 1;
        }
        else if (cmd.find("which ") == 0)
        {
            std::string program = cmd.substr(6);
            // Trim whitespace
            size_t start = program.find_first_not_of(" \t\r");
            if (start != std::string::npos)
            {
                program = program.substr(start);
            }
            size_t end = program.find_last_not_of(" \t\r");
            if (end != std::string::npos)
            {
                program = program.substr(0, end + 1);
            }

            if (program.empty())
            {
                std::cout << "which: missing argument" << std::endl;
                return 1;
            }

            // Get PATH environment variable
            const char* path_env = std::getenv("PATH");
            if (!path_env)
            {
                std::cout << "which: PATH environment variable not found" << std::endl;
                return 1;
            }

            std::string              path_str = path_env;
            std::vector<std::string> paths;

            // Split PATH by delimiter
#ifdef _WIN32
            const char delimiter = ';';
#else
            const char delimiter = ':';
#endif

            size_t      pos = 0;
            std::string token;
            while ((pos = path_str.find(delimiter)) != std::string::npos)
            {
                token = path_str.substr(0, pos);
                if (!token.empty())
                {
                    paths.push_back(token);
                }
                path_str.erase(0, pos + 1);
            }
            if (!path_str.empty())
            {
                paths.push_back(path_str);
            }

            // Search for the program in each path
            for (const auto& path : paths)
            {
                std::filesystem::path full_path = std::filesystem::path(path) / program;

                // Try with .exe extension on Windows
#ifdef _WIN32
                std::filesystem::path exe_path = full_path;
                exe_path += ".exe";
                if (std::filesystem::exists(exe_path) && std::filesystem::is_regular_file(exe_path))
                {
                    std::cout << exe_path.string() << std::endl;
                    return 0;
                }
#endif

                // Try without extension
                if (std::filesystem::exists(full_path) && std::filesystem::is_regular_file(full_path))
                {
                    std::cout << full_path.string() << std::endl;
                    return 0;
                }
            }

            std::cout << program << " not found in PATH" << std::endl;
            return 1;
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "which: " << e.what() << std::endl;
        return 1;
    }

    return 1;  // Unknown utility command
}

int execute_directory_command(const std::string& cmd)
{
    try
    {
        if (cmd == "pwd")
        {
            std::cout << std::filesystem::current_path() << std::endl;
            return 0;
        }
        else if (cmd == "ls" || cmd == "dir")
        {
            // List directory contents
            for (const auto& entry : std::filesystem::directory_iterator("."))
            {
                std::cout << entry.path().filename().string();
                if (std::filesystem::is_directory(entry.path()))
                {
                    std::cout << "/";
                }
                std::cout << std::endl;
            }
            return 0;
        }
        else if (cmd.find("cd ") == 0 || cmd == "cd")
        {
            std::string path = ".";
            if (cmd.length() > 3)
            {
                path = cmd.substr(3);
                // Trim leading/trailing whitespace
                size_t start = path.find_first_not_of(" \t");
                if (start != std::string::npos)
                {
                    path = path.substr(start);
                }
                size_t end = path.find_last_not_of(" \t");
                if (end != std::string::npos)
                {
                    path = path.substr(0, end + 1);
                }
            }

            std::filesystem::current_path(path);
            std::cout << std::filesystem::current_path() << std::endl;
            return 0;
        }
        else if (cmd.find("pushd ") == 0)
        {
            // For simplicity, treat pushd like cd (full implementation would need a directory stack)
            std::string path = cmd.substr(6);
            // Trim whitespace
            size_t start = path.find_first_not_of(" \t");
            if (start != std::string::npos)
            {
                path = path.substr(start);
            }
            size_t end = path.find_last_not_of(" \t");
            if (end != std::string::npos)
            {
                path = path.substr(0, end + 1);
            }

            std::filesystem::current_path(path);
            std::cout << std::filesystem::current_path() << std::endl;
            return 0;
        }
        else if (cmd == "popd")
        {
            // For simplicity, just go to parent directory
            std::filesystem::current_path("..");
            std::cout << std::filesystem::current_path() << std::endl;
            return 0;
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "Directory command failed: " << e.what() << std::endl;
        return 1;
    }

    return 1;  // Unknown directory command
}

/**
 * @brief Main entry point for the Gaussian Extractor application
 *
 * The main function handles the complete application lifecycle:
 * 1. Sets up signal handlers for graceful shutdown
 * 2. Initializes the configuration system
 * 3. Parses command-line arguments and options
 * 4. Dispatches to the appropriate command handler
 * 5. Handles exceptions and provides appropriate exit codes
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 *
 * @return Exit code: 0 for success, non-zero for various error conditions
 *
 * @section Exit Codes
 * - 0: Successful execution
 * - 1: General error (exceptions, unknown commands, etc.)
 * - Command-specific exit codes may also be returned
 *
 * @section Signal Handling
 * The application installs handlers for SIGINT and SIGTERM to enable graceful
 * shutdown of long-running operations. When these signals are received, the
 * global shutdown flag is set and all threads are notified to terminate cleanly.
 *
 * @section Configuration
 * The configuration system is initialized first, loading settings from:
 * - Default configuration file (.gaussian_extractor.conf)
 * - User-specified configuration file
 * - Command-line overrides
 *
 * Configuration errors are reported as warnings but don't prevent execution.
 *
 * @section Error Handling
 * All exceptions are caught at the top level to ensure clean termination:
 * - std::exception derived exceptions show the error message
 * - Unknown exceptions show a generic error message
 * - All exceptions result in exit code 1
 *
 * @note This function coordinates the entire application flow and ensures
 *       proper resource cleanup even in error conditions
 */
int main(int argc, char* argv[])
{
    // Install signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try
    {
        // Initialize configuration system - load from file and apply defaults
        if (!g_config_manager.load_config())
        {
            // Configuration loaded with warnings/errors - continue with defaults
            auto errors = g_config_manager.get_load_errors();
            if (!errors.empty())
            {
                std::cerr << "Configuration warnings:" << std::endl;
                for (const auto& error : errors)
                {
                    std::cerr << "  " << error << std::endl;
                }
                std::cerr << std::endl;
            }
        }

        // Check if running without arguments
        bool no_arguments = (argc == 1);


        if (no_arguments)
        {
#ifdef _WIN32
            // On Windows, no arguments means double-clicked - run extract + enter interactive mode
            std::cout << "Running default EXTRACT command..." << std::endl;
            CommandContext extract_context = CommandParser::parse(1, argv);

            // Show warnings if any
            if (!extract_context.warnings.empty() && !extract_context.quiet)
            {
                for (const auto& warning : extract_context.warnings)
                {
                    std::cerr << warning << std::endl;
                }
                std::cerr << std::endl;
            }

            // Execute EXTRACT
            int extract_result = execute_extract_command(extract_context);
            if (extract_result != 0)
            {
                std::cerr << "EXTRACT command failed with exit code: " << extract_result << std::endl;
                std::cerr << "Continuing to interactive mode..." << std::endl;
            }

            // Enter interactive mode for Windows users
            return run_interactive_loop();
#else
            // On Linux/macOS, no arguments means run default extract command and exit
            std::cout << "Running default EXTRACT command..." << std::endl;
            CommandContext extract_context = CommandParser::parse(1, argv);

            // Show warnings if any
            if (!extract_context.warnings.empty() && !extract_context.quiet)
            {
                for (const auto& warning : extract_context.warnings)
                {
                    std::cerr << warning << std::endl;
                }
                std::cerr << std::endl;
            }

            // Execute EXTRACT and exit
            int extract_result = execute_extract_command(extract_context);

            // On Linux, exit immediately after command execution
            return extract_result;
#endif
        }
        else
        {
            // Parse command and context (will use configuration defaults)
            CommandContext context = CommandParser::parse(argc, argv);

            // Show warnings if any and not in quiet mode
            if (!context.warnings.empty() && !context.quiet)
            {
                for (const auto& warning : context.warnings)
                {
                    std::cerr << warning << std::endl;
                }
                std::cerr << std::endl;
            }

            // Execute based on command type - dispatch to appropriate handler
            int command_result;
            switch (context.command)
            {
                case CommandType::EXTRACT:
                    command_result = execute_extract_command(context);
                    break;

                case CommandType::CHECK_DONE:
                    command_result = execute_check_done_command(context);
                    break;

                case CommandType::CHECK_ERRORS:
                    command_result = execute_check_errors_command(context);
                    break;

                case CommandType::CHECK_PCM:
                    command_result = execute_check_pcm_command(context);
                    break;

                case CommandType::CHECK_IMAGINARY:
                    command_result = execute_check_imaginary_command(context);
                    break;

                case CommandType::CHECK_ALL:
                    command_result = execute_check_all_command(context);
                    break;

                case CommandType::HIGH_LEVEL_KJ:
                    command_result = execute_high_level_kj_command(context);
                    break;

                case CommandType::HIGH_LEVEL_AU:
                    command_result = execute_high_level_au_command(context);
                    break;

                case CommandType::EXTRACT_COORDS:
                    command_result = execute_extract_coords_command(context);
                    break;

                default:
                    std::cerr << "Error: Unknown command type" << std::endl;
                    command_result = 1;
                    break;
            }

                // On Windows, keep the console window open
#ifdef _WIN32
            std::cout << std::endl << "Press Enter to exit..." << std::endl;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
#endif

            return command_result;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
#ifdef _WIN32
        std::cout << std::endl << "Press Enter to exit..." << std::endl;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
#endif
        return 1;
    }

#ifdef _WIN32
    // On Windows, keep the console window open after successful execution
    std::cout << std::endl << "Press Enter to exit..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
#endif

    return 0;
}
