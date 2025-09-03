/**
 * @file command_system.h
 * @brief Command parsing and execution system for Gaussian Extractor
 * @author Le Nhan Pham
 * @date 2025
 *
 * This header defines the command system architecture for the Gaussian Extractor
 * application. It provides a unified interface for parsing command-line arguments,
 * managing command contexts, and dispatching to appropriate command handlers.
 *
 * @section Architecture
 * The command system consists of three main components:
 * - CommandType enumeration defining available commands
 * - CommandContext structure holding parsed parameters and state
 * - CommandParser class for argument parsing and validation
 *
 * @section Supported Commands
 * - extract: Process Gaussian log files and extract thermodynamic data
 * - check-done: Identify and organize completed calculations
 * - check-errors: Identify and organize failed calculations
 * - check-pcm: Identify PCM convergence failures
 * - check-all: Run comprehensive job status checks
 * - high-kj: Calculate high-level energies with output in kJ/mol units
 * - high-au: Calculate high-level energies with detailed output in atomic units
 *
 * @section Integration
 * The command system integrates with:
 * - Configuration management for default values
 * - Job scheduler detection for resource allocation
 * - Multi-threading and resource management systems
 */

#ifndef COMMAND_SYSTEM_H
#define COMMAND_SYSTEM_H

#include "config_manager.h"
#include "job_scheduler.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @enum CommandType
 * @brief Enumeration of available command types in Gaussian Extractor
 *
 * This enumeration defines all supported commands that can be executed by the
 * Gaussian Extractor application. Each command type corresponds to a specific
 * functionality and has its own parameter requirements and execution flow.
 */
enum class CommandType
{
    EXTRACT,          ///< Default command - extract thermodynamic data from log files
    CHECK_DONE,       ///< Check and organize completed job calculations
    CHECK_ERRORS,     ///< Check and organize jobs that terminated with errors
    CHECK_PCM,        ///< Check and organize jobs with PCM convergence failures
    CHECK_IMAGINARY,  ///< Check and organize jobs with imaginary frequencies
    CHECK_ALL,        ///< Run comprehensive checks for all job types
    HIGH_LEVEL_KJ,    ///< Calculate high-level energies with output in kJ/mol units
    HIGH_LEVEL_AU,    ///< Calculate high-level energies with detailed output in atomic units
    EXTRACT_COORDS    ///< Extract coordinates from log files and organize XYZ files
};

/**
 * @struct CommandContext
 * @brief Complete context and parameters for command execution
 *
 * This structure contains all parameters, options, and state information
 * needed to execute any command in the Gaussian Extractor. It serves as
 * the primary communication mechanism between command parsing and execution.
 *
 * The structure is organized into logical groups:
 * - Core command identification
 * - Common parameters shared across commands
 * - Command-specific parameters
 * - Runtime state and configuration
 */
struct CommandContext
{
    CommandType command;  ///< The command to execute

    // Common parameters for all commands
    bool                     quiet;              ///< Suppress non-essential output
    unsigned int             requested_threads;  ///< Number of threads requested by user
    size_t                   max_file_size_mb;   ///< Maximum individual file size in MB
    size_t                   batch_size;         ///< Batch size for processing large directories (0 = auto)
    std::string              extension;          ///< File extension to process (default: ".log")
    std::vector<std::string> valid_extensions;   ///< List of valid file extensions (e.g {".log", ".out"})
    std::vector<std::string> warnings;           ///< Collected warnings from parsing
    JobResources             job_resources;      ///< Job scheduler resource information

    // Extract-specific parameters
    double      temp;                ///< Temperature for calculations (K)
    int         concentration;       ///< Concentration for phase corrections (mM)
    int         sort_column;         ///< Column number for result sorting
    std::string output_format;       ///< Output format ("text", "csv", etc.)
    bool        use_input_temp;      ///< Use temperature from input files
    size_t      memory_limit_mb;     ///< Memory usage limit in MB
    bool        show_resource_info;  ///< Display resource usage information

    // Job checker-specific parameters
    std::string target_dir;          ///< Custom directory name for organizing files
    bool        show_error_details;  ///< Display detailed error messages from log files
    std::string dir_suffix;          ///< Custom suffix for completed job directory

    // Coordinate extraction-specific parameters
    std::vector<std::string> specific_files;  ///< List of specific files to process (empty for all files)

    /**
     * @brief Default constructor with built-in fallback values
     *
     * Initializes all parameters with sensible defaults. These defaults are
     * later overridden by configuration file values and command-line options.
     * The initialization order is: defaults → config file → command line.
     *
     * @note Configuration defaults are applied via apply_config_defaults()
     *       after the configuration system is initialized
     */
    CommandContext()
        : command(CommandType::EXTRACT),                                       // Default to extraction command
          quiet(false),                                                        // Show normal output by default
          requested_threads(0),                                                // Auto-detect thread count
          max_file_size_mb(100),                                               // 100MB max file size
          batch_size(0),                                                       // Auto-detect batch size (0 = disabled)
          extension(".log"),                                                   // Process .log files
          valid_extensions({".log", ".out", ".LOG", ".OUT", ".Log", ".Out"}),  // Valid output extensions
          temp(298.15),                                                        // Room temperature (25°C)
          concentration(1000),                                                 // 1M concentration (1000 mM)
          sort_column(2),             // Sort by second column (typically energy)
          output_format("text"),      // Plain text output
          use_input_temp(false),      // Use fixed temperature
          memory_limit_mb(0),         // No memory limit (auto-detect)
          show_resource_info(false),  // Don't show resource info by default
          target_dir(""),             // Use default directory names
          show_error_details(false),  // Show minimal error info
          dir_suffix("done")
    {}  // Default suffix for completed jobs

    /**
     * @brief Apply configuration file defaults to context parameters
     *
     * Updates context parameters with values from the loaded configuration file.
     * This method is called after configuration loading to apply user preferences
     * while preserving any command-line overrides that were already applied.
     *
     * @note This method should be called after configuration loading but before
     *       final parameter validation
     */
    void apply_config_defaults();
};

/**
 * @class CommandParser
 * @brief Static class for parsing command-line arguments and creating command contexts
 *
 * The CommandParser class provides a complete command-line argument parsing system
 * for the Gaussian Extractor application. It handles:
 * - Command identification and validation
 * - Option parsing with type conversion and validation
 * - Configuration file integration
 * - Help system and documentation generation
 * - Error handling and user feedback
 *
 * @section Design Pattern
 * This class follows the static utility class pattern - all methods are static
 * and no instances are created. This simplifies usage and ensures consistent
 * parsing behavior throughout the application.
 *
 * @section Parsing Flow
 * 1. Command identification (extract, check-done, etc.)
 * 2. Common option parsing (threads, quiet, etc.)
 * 3. Command-specific option parsing
 * 4. Configuration integration and validation
 * 5. Context finalization and return
 */
class CommandParser
{
public:
    /**
     * @brief Parse command-line arguments and create execution context
     * @param argc Number of command-line arguments
     * @param argv Array of command-line argument strings
     * @return Fully populated CommandContext ready for execution
     *
     * This is the main entry point for command-line parsing. It processes all
     * arguments, applies configuration defaults, validates parameters, and
     * returns a complete CommandContext ready for command execution.
     *
     * @throws std::invalid_argument for invalid command syntax
     * @throws std::out_of_range for parameter values outside valid ranges
     *
     * @note Warnings are collected in the context rather than thrown as exceptions
     */
    static CommandContext parse(int argc, char* argv[]);

    /**
     * @brief Print general help information
     * @param program_name Name of the program executable (for display)
     *
     * Displays comprehensive help including:
     * - Application overview and description
     * - Available commands and brief descriptions
     * - Common options and their usage
     * - Configuration file information
     * - Usage examples
     */
    static void print_help(const std::string& program_name = "gaussian_extractor.x");

    /**
     * @brief Print help for a specific command
     * @param command The command type to show help for
     * @param program_name Name of the program executable (for display)
     *
     * Displays detailed help for a specific command including:
     * - Command description and purpose
     * - Command-specific options and parameters
     * - Usage examples and common scenarios
     * - Related commands and workflows
     */
    static void print_command_help(CommandType command, const std::string& program_name = "gaussian_extractor.x");

    /**
     * @brief Print configuration system help
     *
     * Displays information about:
     * - Configuration file locations
     * - Available configuration options
     * - Configuration file format
     * - How to create default configuration
     */
    static void print_config_help();

    /**
     * @brief Create a default configuration file
     *
     * Generates a default configuration file with all available options
     * and their descriptions for user customization.
     */
    static void create_default_config();

private:
    /**
     * @brief Parse command string to CommandType enum
     * @param cmd Command string from command line
     * @return Corresponding CommandType enumeration value
     * @throws std::invalid_argument if command is not recognized
     */
    static CommandType parse_command(const std::string& cmd);

    /**
     * @brief Convert CommandType enum to string representation
     * @param command CommandType to convert
     * @return String name of the command
     */
    static std::string get_command_name(CommandType command);

    /**
     * @brief Parse options common to all commands
     * @param context CommandContext to populate
     * @param i Current argument index (modified by reference)
     * @param argc Total number of arguments
     * @param argv Argument array
     *
     * Handles options like --quiet, --threads, --max-size that are
     * available for all commands.
     */
    static void parse_common_options(CommandContext& context, int& i, int argc, char* argv[]);

    /**
     * @brief Parse options specific to extract command
     * @param context CommandContext to populate
     * @param i Current argument index (modified by reference)
     * @param argc Total number of arguments
     * @param argv Argument array
     *
     * Handles extract-specific options like --temp, --concentration, --format.
     */
    static void parse_extract_options(CommandContext& context, int& i, int argc, char* argv[]);

    /**
     * @brief Parse options specific to job checker commands
     * @param context CommandContext to populate
     * @param i Current argument index (modified by reference)
     * @param argc Total number of arguments
     * @param argv Argument array
     *
     * Handles checker-specific options like --target-dir, --show-errors.
     */
    static void parse_checker_options(CommandContext& context, int& i, int argc, char* argv[]);

    /**
     * @brief Parse options specific to coordinate extraction command
     * @param context CommandContext to populate
     * @param i Current argument index (modified by reference)
     * @param argc Total number of arguments
     * @param argv Argument array
     *
     * Handles xyz-specific options like --files.
     */
    static void parse_xyz_options(CommandContext& context, int& i, int argc, char* argv[]);

    /**
     * @brief Add a warning message to the command context
     * @param context CommandContext to add warning to
     * @param warning Warning message to add
     *
     * Warnings are collected during parsing and displayed to the user
     * before command execution begins.
     */
    static void add_warning(CommandContext& context, const std::string& warning);

    /**
     * @brief Validate parsed context for consistency and correctness
     * @param context CommandContext to validate
     * @throws std::invalid_argument for invalid parameter combinations
     *
     * Performs final validation of the parsed context including:
     * - Parameter range checking
     * - Option compatibility validation
     * - Resource availability verification
     */
    static void validate_context(CommandContext& context);

    /**
     * @brief Load and initialize configuration system
     * @throws std::runtime_error if configuration loading fails critically
     *
     * Initializes the global configuration manager and loads settings
     * from configuration files. Non-critical errors are collected as warnings.
     */
    static void load_configuration();

    /**
     * @brief Apply configuration values to command context
     * @param context CommandContext to update with configuration values
     *
     * Updates context parameters with values from the loaded configuration,
     * respecting any command-line overrides that were already applied.
     */
    static void apply_config_to_context(CommandContext& context);

    /**
     * @brief Extract configuration overrides from command-line arguments
     * @param argc Number of command-line arguments
     * @param argv Array of command-line argument strings
     * @return Map of configuration key-value pairs to override
     *
     * Identifies and extracts --config-option=value style arguments
     * that should override configuration file settings.
     */
    static std::unordered_map<std::string, std::string> extract_config_overrides(int argc, char* argv[]);
};

/**
 * @defgroup CommandExecutors Command Execution Functions
 * @brief Functions for executing specific commands with given contexts
 *
 * These functions implement the actual command logic for each supported
 * command type. They receive a fully configured CommandContext and
 * perform the requested operation, returning appropriate exit codes.
 *
 * @section Return Codes
 * All execution functions follow standard exit code conventions:
 * - 0: Successful execution
 * - 1: General error or failure
 * - 2: Invalid arguments or configuration
 * - 3: Resource unavailable (memory, files, etc.)
 * - 4: Operation interrupted by user or system
 *
 * @section Error Handling
 * Execution functions handle errors gracefully and provide meaningful
 * error messages to users. They also ensure proper resource cleanup
 * even in failure scenarios.
 *
 * @{
 */

#endif  // COMMAND_SYSTEM_H
