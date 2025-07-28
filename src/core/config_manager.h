/**
 * @file config_manager.h
 * @brief Configuration management system for Gaussian Extractor
 * @author Le Nhan Pham
 * @date 2025
 * 
 * This header defines a comprehensive configuration management system that handles
 * loading, parsing, validation, and access to configuration settings from files
 * and command-line overrides. The system supports hierarchical configuration
 * with defaults, user preferences, and runtime overrides.
 * 
 * @section Configuration Sources
 * Configuration values are loaded from multiple sources in priority order:
 * 1. Built-in defaults (lowest priority)
 * 2. System-wide configuration files
 * 3. User-specific configuration files
 * 4. Command-line overrides (highest priority)
 * 
 * @section Configuration Categories
 * Settings are organized into logical categories:
 * - GENERAL: Basic application behavior
 * - EXTRACT: Data extraction parameters
 * - JOB_CHECKER: Job status checking options
 * - PERFORMANCE: Threading and resource limits
 * - OUTPUT: Formatting and display preferences
 * 
 * @section File Format
 * Configuration files use a simple key=value format with:
 * - Comments starting with # character
 * - Section headers for organization
 * - Type-safe value conversion and validation
 * - Support for various data types (string, int, double, bool)
 * 
 * @section Thread Safety
 * The configuration system is designed for single-threaded initialization
 * followed by read-only access from multiple threads. Write operations
 * during runtime are supported but should be used carefully in multi-threaded
 * environments.
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>

/**
 * @defgroup ConfigConstants Configuration File Constants
 * @brief Constants defining configuration file names and search locations
 * @{
 */

/**
 * @brief Primary configuration file name (hidden file convention)
 * 
 * The preferred configuration file name following Unix hidden file conventions.
 * This file is searched for in the current directory and user home directory.
 */
const std::string DEFAULT_CONFIG_FILENAME = ".gaussian_extractor.conf";

/**
 * @brief Alternative configuration file name (visible file)
 * 
 * Alternative configuration file name for systems or users who prefer
 * visible configuration files. Also searched in standard locations.
 */
const std::string ALT_CONFIG_FILENAME = "gaussian_extractor.conf";

/** @} */ // end of ConfigConstants group

/**
 * @enum ConfigCategory
 * @brief Enumeration of configuration setting categories
 * 
 * Configuration settings are organized into logical categories to improve
 * organization, validation, and help system functionality. Each setting
 * belongs to exactly one category, which determines how it's displayed
 * in help output and configuration templates.
 */
enum class ConfigCategory {
    GENERAL,      ///< General application behavior and defaults
    EXTRACT,      ///< Data extraction parameters and thermodynamic settings
    JOB_CHECKER,  ///< Job status checking and file organization options
    PERFORMANCE,  ///< Threading, memory, and performance-related settings
    OUTPUT        ///< Output formatting, display preferences, and reporting
};

/**
 * @struct ConfigValue
 * @brief Structure containing a configuration value with complete metadata
 * 
 * This structure encapsulates a single configuration setting with all associated
 * metadata needed for proper management, validation, and documentation. It tracks
 * both the current value and the original default, along with descriptive
 * information for help systems and configuration file generation.
 * 
 * @section Value Lifecycle
 * 1. Created with default value and metadata during initialization
 * 2. Updated from configuration file during loading (user_set = true)
 * 3. Potentially overridden by command-line arguments
 * 4. Accessed throughout application lifecycle via type-safe getters
 * 
 * @section Metadata Usage
 * - description: Used in help output and configuration file generation
 * - category: Used for organizing settings in help and validation
 * - user_set: Tracks whether user has explicitly set this value
 * - default_value: Preserved for reset operations and help display
 */
struct ConfigValue {
    std::string value;         ///< Current value of the configuration setting
    std::string default_value; ///< Original default value for reset/comparison
    std::string description;   ///< Human-readable description for help systems
    std::string category;      ///< Category name for organization and validation
    bool user_set;            ///< Whether user has explicitly set this value
    
    /**
     * @brief Default constructor creating an empty configuration value
     * 
     * Creates an uninitialized configuration value. Should only be used
     * in contexts where the value will be immediately assigned proper metadata.
     */
    ConfigValue() : user_set(false) {}
    
    /**
     * @brief Constructor with full metadata specification
     * @param def_val Default value for this configuration setting
     * @param desc Human-readable description for help and documentation
     * @param cat Category name for organization (should match ConfigCategory)
     * 
     * Creates a fully initialized configuration value with metadata.
     * The current value is initially set to the default value.
     */
    ConfigValue(const std::string& def_val, const std::string& desc, const std::string& cat)
        : value(def_val), default_value(def_val), description(desc), category(cat), user_set(false) {}
};

/**
 * @class ConfigManager
 * @brief Centralized configuration management system with file loading and validation
 * 
 * The ConfigManager class provides a complete configuration management solution
 * that handles loading, parsing, validation, and access to configuration settings.
 * It supports multiple configuration sources, type-safe value access, and
 * comprehensive error handling and reporting.
 * 
 * @section Design Features
 * - Hierarchical configuration loading (defaults → files → command-line)
 * - Type-safe value conversion with error handling
 * - Comprehensive validation and error reporting
 * - Configuration file generation and templating
 * - Category-based organization for large configuration sets
 * - Thread-safe read operations after initialization
 * 
 * @section Usage Pattern
 * 1. Create ConfigManager instance (typically global)
 * 2. Call load_config() to initialize from files
 * 3. Use type-safe getters (get_string, get_int, etc.) to access values
 * 4. Optionally apply command-line overrides
 * 5. Use throughout application lifetime for configuration access
 * 
 * @section Error Handling
 * Configuration errors are collected rather than thrown as exceptions,
 * allowing the application to continue with default values while reporting
 * issues to the user for correction.
 * 
 * @note The class follows the singleton-like pattern through a global instance
 *       but can be instantiated multiple times if needed for testing
 */
class ConfigManager {
private:
    std::unordered_map<std::string, ConfigValue> config_values; ///< Map of all configuration values with metadata
    std::string config_file_path;     ///< Path to the loaded configuration file
    bool config_loaded;               ///< Whether configuration has been successfully loaded
    std::vector<std::string> load_errors; ///< Collection of errors encountered during loading
    
    /**
     * @defgroup ConfigInternals Internal Configuration Methods
     * @brief Private methods for configuration file processing and validation
     * @{
     */
    
    /**
     * @brief Initialize all configuration values with their defaults
     * 
     * Sets up the complete configuration schema with default values,
     * descriptions, and category assignments. This method defines the
     * complete set of available configuration options.
     */
    void initialize_default_values();
    
    /**
     * @brief Search for and locate configuration file
     * @return true if configuration file found, false otherwise
     * 
     * Searches for configuration files in standard locations:
     * - Current working directory
     * - User home directory
     * - System configuration directories
     */
    bool find_config_file();
    
    /**
     * @brief Parse a single line from configuration file
     * @param line The line content to parse
     * @param line_number Line number for error reporting
     * 
     * Parses individual configuration file lines, handling:
     * - Key=value assignments
     * - Comments and empty lines
     * - Error detection and reporting
     * - Value validation and conversion
     */
    void parse_config_line(const std::string& line, int line_number);
    
    /**
     * @brief Remove leading and trailing whitespace from string
     * @param str String to trim
     * @return Trimmed string with whitespace removed
     */
    std::string trim(const std::string& str);
    
    /**
     * @brief Convert string to lowercase for case-insensitive comparisons
     * @param str String to convert
     * @return Lowercase version of the string
     */
    std::string to_lower(const std::string& str);
    
    /**
     * @brief Validate that a configuration key is recognized
     * @param key Configuration key to validate
     * @return true if key is valid, false otherwise
     */
    bool is_valid_key(const std::string& key);
    
    /**
     * @brief Template method for type-safe value conversion
     * @tparam T Target type for conversion
     * @param str_value String value to convert
     * @param result Reference to store converted value
     * @return true if conversion successful, false otherwise
     * 
     * Provides type-safe conversion from string configuration values
     * to appropriate C++ types with comprehensive error handling.
     */
    template<typename T>
    bool convert_value(const std::string& str_value, T& result);
    
    /** @} */ // end of ConfigInternals group
    
public:
    /**
     * @brief Constructor initializing configuration system
     * 
     * Creates a ConfigManager instance and initializes the default
     * configuration schema. The configuration is not loaded from files
     * until load_config() is explicitly called.
     */
    ConfigManager();
    
    /**
     * @defgroup ConfigLoading Configuration Loading Operations
     * @brief Methods for loading and managing configuration files
     * @{
     */
    
    /**
     * @brief Load configuration from file with error handling
     * @param custom_path Optional custom path to configuration file
     * @return true if loading successful, false if errors occurred
     * 
     * Loads configuration from file, applying values over defaults.
     * If custom_path is provided, only that file is tried. Otherwise,
     * the standard search locations are used.
     * 
     * @note Errors are collected in load_errors vector rather than thrown
     *       as exceptions, allowing operation to continue with defaults
     */
    bool load_config(const std::string& custom_path = "");
    
    /**
     * @brief Create a default configuration file with all options
     * @param path Path where to create the configuration file (empty = auto)
     * @return true if file creation successful, false otherwise
     * 
     * Generates a complete configuration file template with all available
     * options, their default values, and descriptive comments. Useful for
     * initial setup and documentation.
     */
    bool create_default_config_file(const std::string& path = "");
    
    /**
     * @brief Reload configuration from the previously loaded file
     * 
     * Re-reads the configuration file and applies any changes.
     * Useful for runtime configuration updates without restarting
     * the application.
     */
    void reload_config();
    
    /** @} */ // end of ConfigLoading group
    
    /**
     * @defgroup ConfigValueGetters Type-Safe Configuration Value Getters
     * @brief Methods for retrieving configuration values with automatic type conversion
     * @{
     */
    
    /**
     * @brief Get string configuration value
     * @param key Configuration key name
     * @return String value, or default if key not found
     * @throws std::invalid_argument if key doesn't exist and no default
     */
    std::string get_string(const std::string& key);
    
    /**
     * @brief Get integer configuration value
     * @param key Configuration key name
     * @return Integer value, or default if key not found
     * @throws std::invalid_argument if key doesn't exist or conversion fails
     */
    int get_int(const std::string& key);
    
    /**
     * @brief Get double configuration value
     * @param key Configuration key name
     * @return Double value, or default if key not found
     * @throws std::invalid_argument if key doesn't exist or conversion fails
     */
    double get_double(const std::string& key);
    
    /**
     * @brief Get boolean configuration value
     * @param key Configuration key name
     * @return Boolean value, or default if key not found
     * @throws std::invalid_argument if key doesn't exist or conversion fails
     * 
     * @note Accepts various boolean representations: true/false, yes/no, 1/0, on/off
     */
    bool get_bool(const std::string& key);
    
    /**
     * @brief Get unsigned integer configuration value
     * @param key Configuration key name
     * @return Unsigned integer value, or default if key not found
     * @throws std::invalid_argument if key doesn't exist or conversion fails
     */
    unsigned int get_uint(const std::string& key);
    
    /**
     * @brief Get size_t configuration value
     * @param key Configuration key name
     * @return size_t value, or default if key not found
     * @throws std::invalid_argument if key doesn't exist or conversion fails
     */
    size_t get_size_t(const std::string& key);
    
    /** @} */ // end of ConfigValueGetters group
    
    /**
     * @defgroup ConfigFallbackGetters Configuration Getters with Fallback Values
     * @brief Safe value getters that return fallback values instead of throwing exceptions
     * @{
     */
    
    /**
     * @brief Get string value with fallback
     * @param key Configuration key name
     * @param fallback Value to return if key not found or conversion fails
     * @return Configuration value or fallback
     */
    std::string get_string(const std::string& key, const std::string& fallback);
    
    /**
     * @brief Get integer value with fallback
     * @param key Configuration key name
     * @param fallback Value to return if key not found or conversion fails
     * @return Configuration value or fallback
     */
    int get_int(const std::string& key, int fallback);
    
    /**
     * @brief Get double value with fallback
     * @param key Configuration key name
     * @param fallback Value to return if key not found or conversion fails
     * @return Configuration value or fallback
     */
    double get_double(const std::string& key, double fallback);
    
    /**
     * @brief Get boolean value with fallback
     * @param key Configuration key name
     * @param fallback Value to return if key not found or conversion fails
     * @return Configuration value or fallback
     */
    bool get_bool(const std::string& key, bool fallback);
    
    /**
     * @brief Get unsigned integer value with fallback
     * @param key Configuration key name
     * @param fallback Value to return if key not found or conversion fails
     * @return Configuration value or fallback
     */
    unsigned int get_uint(const std::string& key, unsigned int fallback);
    
    /**
     * @brief Get size_t value with fallback
     * @param key Configuration key name
     * @param fallback Value to return if key not found or conversion fails
     * @return Configuration value or fallback
     */
    size_t get_size_t(const std::string& key, size_t fallback);
    
    /** @} */ // end of ConfigFallbackGetters group
    
    /**
     * @defgroup ConfigValueSetters Runtime Configuration Value Setters
     * @brief Methods for updating configuration values during runtime
     * @{
     */
    
    /**
     * @brief Set string configuration value at runtime
     * @param key Configuration key name
     * @param value New string value to set
     * 
     * @note Changes are not persisted to configuration file
     */
    void set_value(const std::string& key, const std::string& value);
    
    /**
     * @brief Set integer configuration value at runtime
     * @param key Configuration key name
     * @param value New integer value to set
     */
    void set_value(const std::string& key, int value);
    
    /**
     * @brief Set double configuration value at runtime
     * @param key Configuration key name
     * @param value New double value to set
     */
    void set_value(const std::string& key, double value);
    
    /**
     * @brief Set boolean configuration value at runtime
     * @param key Configuration key name
     * @param value New boolean value to set
     */
    void set_value(const std::string& key, bool value);
    
    /**
     * @brief Set unsigned integer configuration value at runtime
     * @param key Configuration key name
     * @param value New unsigned integer value to set
     */
    void set_value(const std::string& key, unsigned int value);
    
    /**
     * @brief Set size_t configuration value at runtime
     * @param key Configuration key name
     * @param value New size_t value to set
     */
    void set_value(const std::string& key, size_t value);
    
    /** @} */ // end of ConfigValueSetters group
    
    /**
     * @defgroup ConfigQueries Configuration Query Methods
     * @brief Methods for querying configuration metadata and structure
     * @{
     */
    
    /**
     * @brief Check if a configuration key exists
     * @param key Configuration key name to check
     * @return true if key exists in configuration schema, false otherwise
     */
    bool has_key(const std::string& key);
    
    /**
     * @brief Check if a configuration value was explicitly set by user
     * @param key Configuration key name to check
     * @return true if user has set this value, false if using default
     * 
     * Useful for determining whether to show configuration values in help
     * output or to apply different validation rules for user-set values.
     */
    bool is_user_set(const std::string& key);
    
    /**
     * @brief Get descriptive text for a configuration key
     * @param key Configuration key name
     * @return Description string for help systems and documentation
     */
    std::string get_description(const std::string& key);
    
    /**
     * @brief Get category name for a configuration key
     * @param key Configuration key name
     * @return Category string for organizational purposes
     */
    std::string get_category(const std::string& key);
    
    /**
     * @brief Get all configuration keys belonging to a specific category
     * @param category Category name to filter by
     * @return Vector of key names in the specified category
     * 
     * Useful for generating category-specific help output or validation.
     */
    std::vector<std::string> get_keys_by_category(const std::string& category);
    
    /** @} */ // end of ConfigQueries group
    
    /**
     * @defgroup ConfigStatus Configuration Status and Debugging
     * @brief Methods for checking configuration state and debugging issues
     * @{
     */
    
    /**
     * @brief Check if configuration has been successfully loaded
     * @return true if configuration loaded without critical errors
     */
    bool is_config_loaded() const { return config_loaded; }
    
    /**
     * @brief Get path to the loaded configuration file
     * @return File path string, empty if no file was loaded
     */
    std::string get_config_file_path() const { return config_file_path; }
    
    /**
     * @brief Get list of errors encountered during configuration loading
     * @return Vector of error message strings
     * 
     * Errors are collected rather than thrown to allow operation with
     * default values while informing user of configuration issues.
     */
    std::vector<std::string> get_load_errors() const { return load_errors; }
    
    /**
     * @brief Print summary of current configuration values
     * @param show_descriptions Whether to include descriptions in output
     * 
     * Displays all configuration values organized by category, with
     * optional descriptions for documentation purposes.
     */
    void print_config_summary(bool show_descriptions = false);
    
    /**
     * @brief Print complete configuration file template
     * 
     * Outputs a complete configuration file template with all available
     * options, descriptions, and default values. Useful for creating
     * initial configuration files.
     */
    void print_config_file_template();
    
    /** @} */ // end of ConfigStatus group
    
    /**
     * @defgroup ConfigValidation Configuration Validation Methods
     * @brief Methods for validating configuration values and consistency
     * @{
     */
    
    /**
     * @brief Validate all configuration values for correctness and consistency
     * @return Vector of validation error messages (empty if all valid)
     * 
     * Performs comprehensive validation including:
     * - Numeric range checking
     * - File extension validation  
     * - Inter-parameter consistency checks
     * - Resource limit validation
     */
    std::vector<std::string> validate_config();
    
    /**
     * @brief Validate file extension configuration values
     * @return true if all extension values are valid, false otherwise
     * 
     * Checks that configured file extensions follow proper format
     * and are supported by the application.
     */
    bool validate_file_extensions();
    
    /**
     * @brief Validate numeric configuration values are within acceptable ranges
     * @return true if all numeric values are in valid ranges, false otherwise
     * 
     * Validates that numeric parameters like thread counts, memory limits,
     * and temperatures are within physically meaningful and safe ranges.
     */
    bool validate_numeric_ranges();
    
    /** @} */ // end of ConfigValidation group
    
    /**
     * @defgroup ConfigExtensions File Extension Management
     * @brief Methods for managing supported file extensions
     * @{
     */
    
    /**
     * @brief Get list of supported input file extensions
     * @return Vector of input extensions (e.g., ".log", ".out")
     */
    std::vector<std::string> get_input_extensions();
    
    /**
     * @brief Get list of supported output file extensions
     * @return Vector of output extensions (e.g., ".txt", ".csv")
     */
    std::vector<std::string> get_output_extensions();
    
    /**
     * @brief Check if an extension is valid for input files
     * @param ext File extension to validate (should include leading dot)
     * @return true if extension is supported for input, false otherwise
     */
    bool is_valid_input_extension(const std::string& ext);
    
    /**
     * @brief Check if an extension is valid for output files
     * @param ext File extension to validate (should include leading dot)
     * @return true if extension is supported for output, false otherwise
     */
    bool is_valid_output_extension(const std::string& ext);
    
    /** @} */ // end of ConfigExtensions group
    
    /**
     * @defgroup ConfigConvenience Convenient Getters for Common Values
     * @brief Specialized getters for frequently accessed configuration values
     * @{
     */
    
    /**
     * @brief Get default output file extension
     * @return Default extension for output files
     */
    std::string get_default_output_extension();
    
    /**
     * @brief Get default input file extension
     * @return Default extension to search for in input files
     */
    std::string get_default_input_extension();
    
    /**
     * @brief Get default temperature for thermodynamic calculations
     * @return Default temperature in Kelvin
     */
    double get_default_temperature();
    
    /**
     * @brief Get default concentration for phase corrections
     * @return Default concentration in millimolar (mM)
     */
    double get_default_concentration();
    
    /**
     * @brief Get default pressure for standard state calculations
     * @return Default pressure in atmospheres or pascals
     */
    double get_default_pressure();
    
    /**
     * @brief Get default number of processing threads
     * @return Default thread count (0 = auto-detect)
     */
    unsigned int get_default_threads();
    
    /**
     * @brief Get default output format
     * @return Default format string ("text", "csv", etc.)
     */
    std::string get_default_output_format();
    
    /**
     * @brief Get default maximum file size limit
     * @return Default file size limit in megabytes
     */
    size_t get_default_max_file_size();
    
    /** @} */ // end of ConfigConvenience group
    
    /**
     * @defgroup ConfigOverrides Command Line Override Support
     * @brief Methods for applying command-line configuration overrides
     * @{
     */
    
    /**
     * @brief Apply command-line configuration overrides
     * @param overrides Map of configuration key-value pairs to override
     * 
     * Applies configuration overrides from command-line arguments.
     * These have the highest priority and override both default values
     * and configuration file settings.
     */
    void apply_command_line_overrides(const std::unordered_map<std::string, std::string>& overrides);
    
    /** @} */ // end of ConfigOverrides group
    
    /**
     * @defgroup ConfigUtilities Configuration Utility Methods
     * @brief Utility methods for configuration system support
     * @{
     */
    
    /**
     * @brief Get user's home directory path
     * @return Path to user home directory for configuration file search
     * 
     * Platform-specific implementation for locating user home directory
     * where user-specific configuration files are searched.
     */
    std::string get_user_home_directory();
    
    /**
     * @brief Lock configuration for write operations (future use)
     * 
     * Placeholder for thread safety support in future versions.
     * Currently no-op as configuration is read-only after initialization.
     */
    void lock_config() const {}
    
    /**
     * @brief Unlock configuration after write operations (future use)
     * 
     * Placeholder for thread safety support in future versions.
     * Currently no-op as configuration is read-only after initialization.
     */
    void unlock_config() const {}
    
    /** @} */ // end of ConfigUtilities group
};

/**
 * @brief Global configuration manager instance
 * 
 * Single global instance of ConfigManager used throughout the application.
 * Initialized at program startup and used for all configuration access.
 * 
 * @note Defined in config_manager.cpp
 */
extern ConfigManager g_config_manager;

/**
 * @defgroup ConfigMacros Configuration Access Macros
 * @brief Convenient macros for quick configuration value access
 * 
 * These macros provide shorthand access to common configuration values
 * through the global configuration manager instance. They simplify code
 * and reduce verbosity for frequently accessed configuration values.
 * 
 * @note Use these macros for simple value access. For error handling or
 *       fallback values, use the ConfigManager methods directly.
 * @{
 */

/**
 * @brief Get string configuration value via macro
 * @param key Configuration key name
 */
#define CONFIG_GET_STRING(key) g_config_manager.get_string(key)

/**
 * @brief Get integer configuration value via macro
 * @param key Configuration key name
 */
#define CONFIG_GET_INT(key) g_config_manager.get_int(key)

/**
 * @brief Get double configuration value via macro
 * @param key Configuration key name
 */
#define CONFIG_GET_DOUBLE(key) g_config_manager.get_double(key)

/**
 * @brief Get boolean configuration value via macro
 * @param key Configuration key name
 */
#define CONFIG_GET_BOOL(key) g_config_manager.get_bool(key)

/**
 * @brief Get unsigned integer configuration value via macro
 * @param key Configuration key name
 */
#define CONFIG_GET_UINT(key) g_config_manager.get_uint(key)

/** @} */ // end of ConfigMacros group

/**
 * @namespace ConfigUtils
 * @brief Utility functions supporting the configuration system
 * 
 * The ConfigUtils namespace provides a collection of utility functions
 * that support configuration file processing, validation, and management.
 * These functions are used internally by ConfigManager and can also be
 * used by other parts of the application for configuration-related tasks.
 */
namespace ConfigUtils {
    /**
     * @defgroup ConfigFileUtils File System Utilities
     * @brief Utilities for configuration file system operations
     * @{
     */
    
    /**
     * @brief Check if a file exists and is accessible
     * @param path Path to file to check
     * @return true if file exists, false otherwise
     */
    bool file_exists(const std::string& path);
    
    /**
     * @brief Check if a file is readable
     * @param path Path to file to check
     * @return true if file can be read, false otherwise
     */
    bool is_readable(const std::string& path);
    
    /**
     * @brief Check if a file/directory is writable
     * @param path Path to check for write permissions
     * @return true if writable, false otherwise
     */
    bool is_writable(const std::string& path);
    
    /**
     * @brief Get formatted string of configuration file search paths
     * @return Multi-line string showing where configuration files are searched
     * 
     * Returns human-readable description of configuration file search
     * locations for help and debugging output.
     */
    std::string get_config_search_paths();
    
    /** @} */ // end of ConfigFileUtils group
    
    /**
     * @defgroup ConfigStringUtils String Processing Utilities
     * @brief Utilities for parsing and formatting configuration strings
     * @{
     */
    
    /**
     * @brief Split string into vector using delimiter
     * @param str String to split
     * @param delimiter Character to split on
     * @return Vector of string parts
     */
    std::vector<std::string> split_string(const std::string& str, char delimiter);
    
    /**
     * @brief Join vector of strings with delimiter
     * @param strings Vector of strings to join
     * @param delimiter String to use as separator
     * @return Single joined string
     */
    std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter);
    
    /**
     * @brief Convert string to boolean value
     * @param str String to convert (supports various formats)
     * @return Boolean value
     * 
     * Supports various boolean representations:
     * - true/false, yes/no, on/off (case insensitive)
     * - 1/0 numeric values
     */
    bool string_to_bool(const std::string& str);
    
    /**
     * @brief Convert boolean value to standard string representation
     * @param value Boolean value to convert
     * @return "true" or "false" string
     */
    std::string bool_to_string(bool value);
    
    /** @} */ // end of ConfigStringUtils group
    
    /**
     * @defgroup ConfigValidationUtils Configuration Validation Utilities
     * @brief Utilities for validating configuration parameter values
     * @{
     */
    
    /**
     * @brief Validate temperature value for physical reasonableness
     * @param temp Temperature value in Kelvin
     * @return true if temperature is physically reasonable, false otherwise
     */
    bool is_valid_temperature(double temp);
    
    /**
     * @brief Validate concentration value for chemical reasonableness
     * @param conc Concentration value
     * @return true if concentration is chemically reasonable, false otherwise
     */
    bool is_valid_concentration(double conc);
    
    /**
     * @brief Validate pressure value for physical reasonableness
     * @param pressure Pressure value
     * @return true if pressure is physically reasonable, false otherwise
     */
    bool is_valid_pressure(double pressure);
    
    /**
     * @brief Validate thread count for system appropriateness
     * @param threads Number of threads
     * @return true if thread count is appropriate for system, false otherwise
     */
    bool is_valid_thread_count(unsigned int threads);
    
    /**
     * @brief Validate file size limit for system appropriateness
     * @param size_mb File size limit in megabytes
     * @return true if size limit is reasonable, false otherwise
     */
    bool is_valid_file_size(size_t size_mb);
    
    /**
     * @brief Validate file extension format
     * @param ext File extension to validate
     * @return true if extension format is valid, false otherwise
     */
    bool is_valid_extension(const std::string& ext);
    
    /** @} */ // end of ConfigValidationUtils group
    
    /**
     * @defgroup ConfigMigration Configuration Migration Utilities
     * @brief Utilities for handling configuration file version migration
     * @{
     */
    
    /**
     * @brief Extract configuration file format version
     * @param config_content Content of configuration file
     * @return Version number (0 if no version found)
     * 
     * For future use when configuration file format changes require
     * migration of existing user configuration files.
     */
    int get_config_version(const std::string& config_content);
    
    /**
     * @brief Migrate configuration file if needed
     * @param config_path Path to configuration file
     * @return true if migration successful or not needed, false if failed
     * 
     * For future use when configuration file format changes require
     * automatic migration of existing user configuration files.
     */
    bool migrate_config_if_needed(const std::string& config_path);
    
    /** @} */ // end of ConfigMigration group
}

#endif // CONFIG_MANAGER_H