/**
 * @file help_utils.h
 * @brief Header file for help utility functions
 * @author Le Nhan Pham
 * @date 2025
 *
 * This file contains declarations for help-related utility functions
 * used throughout the Gaussian Extractor application.
 */

#ifndef HELP_UTILS_H
#define HELP_UTILS_H

#include "command_system.h"  // For CommandType
#include <string>

namespace HelpUtils
{

    /**
     * @brief Print general help information
     * @param program_name Name of the program (default: "gaussian_extractor")
     */
    void print_help(const std::string& program_name = "gaussian_extractor");

    /**
     * @brief Print command-specific help information
     * @param command The command type to show help for
     * @param program_name Name of the program (default: "gaussian_extractor")
     */
    void print_command_help(CommandType command, const std::string& program_name = "gaussian_extractor");

    /**
     * @brief Print configuration help information
     */
    void print_config_help();

    /**
     * @brief Create default configuration file
     */
    void create_default_config();

}  // namespace HelpUtils

#endif  // HELP_UTILS_H