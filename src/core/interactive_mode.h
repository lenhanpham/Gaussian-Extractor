/**
 * @file interactive_mode.h
 * @brief Header file for interactive mode functionality
 * @author Le Nhan Pham
 * @date 2025
 *
 * This header declares functions for the interactive command-line interface
 * of the Gaussian Extractor application.
 */

#ifndef INTERACTIVE_MODE_H
#define INTERACTIVE_MODE_H

#include <string>

// Forward declarations for command execution functions
struct CommandContext;
int execute_extract_command(const CommandContext& context);
int execute_check_done_command(const CommandContext& context);
int execute_check_errors_command(const CommandContext& context);
int execute_check_pcm_command(const CommandContext& context);
int execute_check_imaginary_command(const CommandContext& context);
int execute_check_all_command(const CommandContext& context);
int execute_high_level_kj_command(const CommandContext& context);
int execute_high_level_au_command(const CommandContext& context);
int execute_extract_coords_command(const CommandContext& context);

/**
 * @brief Interactive command loop for Windows double-click usage
 *
 * This function provides an interactive shell where users can enter commands
 * after the initial EXTRACT operation. It reads commands from stdin and
 * processes them using the existing command parsing system.
 *
 * @return Exit code when the interactive loop terminates
 */
int run_interactive_loop();

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

#endif  // INTERACTIVE_MODE_H