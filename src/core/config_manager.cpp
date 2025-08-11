#include "config_manager.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <regex>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

// Global configuration instance
ConfigManager g_config_manager;

ConfigManager::ConfigManager() : config_loaded(false) {
    initialize_default_values();
}

void ConfigManager::initialize_default_values() {
    // General settings
    config_values["output_extension"] = ConfigValue(".log", "Default output file extension to process", "general");
    config_values["input_extensions"] = ConfigValue(".com,.gjf,.gau", "Comma-separated list of input file extensions", "general");
    config_values["output_extensions"] = ConfigValue(".log,.out", "Comma-separated list of output file extensions", "general");
    config_values["quiet_mode"] = ConfigValue("false", "Enable quiet mode by default", "general");
    config_values["auto_backup"] = ConfigValue("false", "Automatically backup files before moving", "general");
    
    // Extract command settings
    config_values["default_temperature"] = ConfigValue("298.15", "Default temperature in Kelvin", "extract");
    config_values["default_concentration"] = ConfigValue("1.0", "Default concentration in M", "extract");
    config_values["default_pressure"] = ConfigValue("1.0", "Default pressure in atm", "extract");
    config_values["default_sort_column"] = ConfigValue("2", "Default column to sort by (2-10)", "extract");
    config_values["default_output_format"] = ConfigValue("text", "Default output format (text/csv)", "extract");
    config_values["use_input_temp"] = ConfigValue("false", "Use temperature from input files by default", "extract");
    config_values["phase_correction"] = ConfigValue("true", "Apply phase correction by default", "extract");
    
    // Job checker settings
    config_values["done_directory_suffix"] = ConfigValue("done", "Default suffix for completed jobs directory", "job_checker");
    config_values["error_directory_name"] = ConfigValue("errorJobs", "Default directory name for error jobs", "job_checker");
    config_values["pcm_directory_name"] = ConfigValue("PCMMkU", "Default directory name for PCM failed jobs", "job_checker");
    config_values["show_error_details"] = ConfigValue("false", "Show error details by default", "job_checker");
    config_values["move_related_files"] = ConfigValue("true", "Move related .gau/.chk files with .log files", "job_checker");
    config_values["create_subdirectories"] = ConfigValue("true", "Create subdirectories for job organization", "job_checker");
    
    // Performance settings
    config_values["default_threads"] = ConfigValue("half", "Default thread count (number/half/max)", "performance");
    config_values["max_file_size_mb"] = ConfigValue("100", "Maximum file size to process in MB", "performance");
    config_values["memory_limit_mb"] = ConfigValue("0", "Memory limit in MB (0 = auto)", "performance");
    config_values["cluster_safe_mode"] = ConfigValue("auto", "Cluster safety mode (auto/on/off)", "performance");
    config_values["progress_reporting"] = ConfigValue("true", "Show progress during processing", "performance");
    config_values["file_handle_limit"] = ConfigValue("20", "Maximum concurrent file handles", "performance");
    
    // Output settings
    config_values["results_filename_template"] = ConfigValue("{dirname}.results", "Template for results filename", "output");
    config_values["csv_filename_template"] = ConfigValue("{dirname}.csv", "Template for CSV filename", "output");
    config_values["include_metadata"] = ConfigValue("true", "Include metadata in output files", "output");
    config_values["decimal_precision"] = ConfigValue("6", "Decimal precision for numerical output", "output");
    config_values["scientific_notation"] = ConfigValue("false", "Use scientific notation for small numbers", "output");
    config_values["include_timestamps"] = ConfigValue("true", "Include timestamps in output", "output");
}

bool ConfigManager::load_config(const std::string& custom_path) {
    load_errors.clear();
    config_loaded = false;
    
    // Determine config file path
    if (!custom_path.empty()) {
        config_file_path = custom_path;
    } else if (!find_config_file()) {
        // No config file found, use defaults
        config_loaded = true;
        return true;
    }
    
    // Try to open and parse config file
    std::ifstream config_file(config_file_path);
    if (!config_file.is_open()) {
        load_errors.push_back("Cannot open config file: " + config_file_path);
        return false;
    }
    
    std::string line;
    int line_number = 0;
    
    while (std::getline(config_file, line)) {
        line_number++;
        parse_config_line(line, line_number);
    }
    
    config_file.close();
    config_loaded = true;
    
    // Validate loaded configuration
    auto validation_errors = validate_config();
    if (!validation_errors.empty()) {
        load_errors.insert(load_errors.end(), validation_errors.begin(), validation_errors.end());
    }
    
    return load_errors.empty();
}

bool ConfigManager::find_config_file() {
    std::vector<std::string> search_paths;
    
    // 1. Current directory
    search_paths.push_back("./" + DEFAULT_CONFIG_FILENAME);
    search_paths.push_back("./" + ALT_CONFIG_FILENAME);
    
    // 2. User home directory
    std::string home_dir = get_user_home_directory();
    if (!home_dir.empty()) {
        search_paths.push_back(home_dir + "/" + DEFAULT_CONFIG_FILENAME);
        search_paths.push_back(home_dir + "/" + ALT_CONFIG_FILENAME);
    }
    
    // 3. System config directories (Unix-like systems)
    #ifndef _WIN32
    search_paths.push_back("/etc/gaussian_extractor/" + ALT_CONFIG_FILENAME);
    search_paths.push_back("/usr/local/etc/" + ALT_CONFIG_FILENAME);
    #endif
    
    // Check each path
    for (const auto& path : search_paths) {
        if (ConfigUtils::file_exists(path) && ConfigUtils::is_readable(path)) {
            config_file_path = path;
            return true;
        }
    }
    
    return false;
}

void ConfigManager::parse_config_line(const std::string& line, int line_number) {
    std::string trimmed = trim(line);
    
    // Skip empty lines and comments
    if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
        return;
    }
    
    // Find key-value separator
    size_t eq_pos = trimmed.find('=');
    if (eq_pos == std::string::npos) {
        load_errors.push_back("Line " + std::to_string(line_number) + ": No '=' found in: " + trimmed);
        return;
    }
    
    std::string key = trim(trimmed.substr(0, eq_pos));
    std::string value = trim(trimmed.substr(eq_pos + 1));
    
    // Remove quotes from value if present
    if (value.length() >= 2) {
        if ((value.front() == '"' && value.back() == '"') ||
            (value.front() == '\'' && value.back() == '\'')) {
            value = value.substr(1, value.length() - 2);
        }
    }
    
    // Validate key
    if (!is_valid_key(key)) {
        load_errors.push_back("Line " + std::to_string(line_number) + ": Unknown configuration key: " + key);
        return;
    }
    
    // Set the value
    if (config_values.find(key) != config_values.end()) {
        config_values[key].value = value;
        config_values[key].user_set = true;
    }
}

std::string ConfigManager::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string ConfigManager::to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool ConfigManager::is_valid_key(const std::string& key) {
    return config_values.find(key) != config_values.end();
}

std::string ConfigManager::get_user_home_directory() {
    #ifdef _WIN32
    // Use environment variables for Windows
    const char* home = getenv("USERPROFILE");
    if (home) return std::string(home);
    const char* drive = getenv("HOMEDRIVE");
    const char* path_env = getenv("HOMEPATH");
    if (drive && path_env) {
        return std::string(drive) + std::string(path_env);
    }
    #else
    const char* home = getenv("HOME");
    if (home) return std::string(home);
    
    // Fallback to passwd entry
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return std::string(pw->pw_dir);
    }
    #endif
    
    return "";
}

template<typename T>
bool ConfigManager::convert_value(const std::string& str_value, T& result) {
    std::istringstream iss(str_value);
    return !(iss >> result).fail() && iss.eof();
}

// Specialized template for bool
template<>
bool ConfigManager::convert_value<bool>(const std::string& str_value, bool& result) {
    std::string lower = to_lower(str_value);
    if (lower == "true" || lower == "yes" || lower == "on" || lower == "1") {
        result = true;
        return true;
    } else if (lower == "false" || lower == "no" || lower == "off" || lower == "0") {
        result = false;
        return true;
    }
    return false;
}

// Value getters with type conversion
std::string ConfigManager::get_string(const std::string& key) {
    auto it = config_values.find(key);
    return (it != config_values.end()) ? it->second.value : "";
}

int ConfigManager::get_int(const std::string& key) {
    return get_int(key, 0);
}

double ConfigManager::get_double(const std::string& key) {
    return get_double(key, 0.0);
}

bool ConfigManager::get_bool(const std::string& key) {
    return get_bool(key, false);
}

unsigned int ConfigManager::get_uint(const std::string& key) {
    return get_uint(key, 0);
}

size_t ConfigManager::get_size_t(const std::string& key) {
    return get_size_t(key, 0);
}

// Value getters with fallback
std::string ConfigManager::get_string(const std::string& key, const std::string& fallback) {
    auto it = config_values.find(key);
    if (it == config_values.end()) return fallback;
    return it->second.value.empty() ? fallback : it->second.value;
}

int ConfigManager::get_int(const std::string& key, int fallback) {
    auto it = config_values.find(key);
    if (it == config_values.end()) return fallback;
    
    int result;
    if (convert_value(it->second.value, result)) {
        return result;
    }
    return fallback;
}

double ConfigManager::get_double(const std::string& key, double fallback) {
    auto it = config_values.find(key);
    if (it == config_values.end()) return fallback;
    
    double result;
    if (convert_value(it->second.value, result)) {
        return result;
    }
    return fallback;
}

bool ConfigManager::get_bool(const std::string& key, bool fallback) {
    auto it = config_values.find(key);
    if (it == config_values.end()) return fallback;
    
    bool result;
    if (convert_value(it->second.value, result)) {
        return result;
    }
    return fallback;
}

unsigned int ConfigManager::get_uint(const std::string& key, unsigned int fallback) {
    auto it = config_values.find(key);
    if (it == config_values.end()) return fallback;
    
    unsigned int result;
    if (convert_value(it->second.value, result)) {
        return result;
    }
    return fallback;
}

size_t ConfigManager::get_size_t(const std::string& key, size_t fallback) {
    auto it = config_values.find(key);
    if (it == config_values.end()) return fallback;
    
    size_t result;
    if (convert_value(it->second.value, result)) {
        return result;
    }
    return fallback;
}

// Value setters (runtime only)
void ConfigManager::set_value(const std::string& key, const std::string& value) {
    if (config_values.find(key) != config_values.end()) {
        config_values[key].value = value;
    }
}

void ConfigManager::set_value(const std::string& key, int value) {
    set_value(key, std::to_string(value));
}

void ConfigManager::set_value(const std::string& key, double value) {
    set_value(key, std::to_string(value));
}

void ConfigManager::set_value(const std::string& key, bool value) {
    std::string str_value = value ? "true" : "false";
    if (config_values.find(key) != config_values.end()) {
        config_values[key].value = str_value;
    }
}

void ConfigManager::set_value(const std::string& key, unsigned int value) {
    set_value(key, std::to_string(value));
}

void ConfigManager::set_value(const std::string& key, size_t value) {
    set_value(key, std::to_string(value));
}

// Configuration queries
bool ConfigManager::has_key(const std::string& key) {
    return config_values.find(key) != config_values.end();
}

bool ConfigManager::is_user_set(const std::string& key) {
    auto it = config_values.find(key);
    return (it != config_values.end()) ? it->second.user_set : false;
}

std::string ConfigManager::get_description(const std::string& key) {
    auto it = config_values.find(key);
    return (it != config_values.end()) ? it->second.description : "";
}

std::string ConfigManager::get_category(const std::string& key) {
    auto it = config_values.find(key);
    return (it != config_values.end()) ? it->second.category : "";
}

std::vector<std::string> ConfigManager::get_keys_by_category(const std::string& category) {
    std::vector<std::string> keys;
    for (const auto& pair : config_values) {
        if (pair.second.category == category) {
            keys.push_back(pair.first);
        }
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

void ConfigManager::print_config_summary(bool show_descriptions) {
    std::cout << "\n=== Configuration Summary ===" << std::endl;
    if (!config_file_path.empty()) {
        std::cout << "Config file: " << config_file_path << std::endl;
    } else {
        std::cout << "Config file: Using built-in defaults" << std::endl;
    }
    std::cout << "Loaded: " << (config_loaded ? "Yes" : "No") << std::endl;
    
    if (!load_errors.empty()) {
        std::cout << "\nLoad errors:" << std::endl;
        for (const auto& error : load_errors) {
            std::cout << "  " << error << std::endl;
        }
    }
    
    // Group by category
    std::vector<std::string> categories = {"general", "extract", "job_checker", "performance", "output"};
    
    for (const auto& category : categories) {
        auto keys = get_keys_by_category(category);
        if (keys.empty()) continue;
        
        std::cout << "\n[" << category << "]" << std::endl;
        for (const auto& key : keys) {
            const auto& config_val = config_values[key];
            std::cout << "  " << key << " = " << config_val.value;
            if (config_val.user_set) {
                std::cout << " (user set)";
            } else {
                std::cout << " (default)";
            }
            if (show_descriptions) {
                std::cout << "\n    # " << config_val.description;
            }
            std::cout << std::endl;
        }
    }
    std::cout << "==============================\n" << std::endl;
}

void ConfigManager::print_config_file_template() {
    std::cout << "# Gaussian Extractor Configuration File" << std::endl;
    std::cout << "# Save this as .gaussian_extractor.conf in your home directory or current working directory" << std::endl;
    std::cout << "#" << std::endl;
    std::cout << "# Lines starting with # or ; are comments" << std::endl;
    std::cout << "# Values can be quoted with \" or ' if they contain spaces" << std::endl;
    std::cout << "#" << std::endl;
    
    std::vector<std::string> categories = {"general", "extract", "job_checker", "performance", "output"};
    
    for (const auto& category : categories) {
        auto keys = get_keys_by_category(category);
        if (keys.empty()) continue;
        
        std::cout << "\n# " << std::string(50, '=') << std::endl;
        std::cout << "# " << category << " settings" << std::endl;
        std::cout << "# " << std::string(50, '=') << std::endl;
        
        for (const auto& key : keys) {
            const auto& config_val = config_values[key];
            std::cout << std::endl;
            std::cout << "# " << config_val.description << std::endl;
            std::cout << key << " = " << config_val.default_value << std::endl;
        }
    }
    
    std::cout << "\n# End of configuration file" << std::endl;
}

bool ConfigManager::create_default_config_file(const std::string& path) {
    std::string file_path = path;
    if (file_path.empty()) {
        std::string home = get_user_home_directory();
        if (home.empty()) {
            file_path = "./" + DEFAULT_CONFIG_FILENAME;
        } else {
            file_path = home + "/" + DEFAULT_CONFIG_FILENAME;
        }
    }
    
    std::ofstream config_file(file_path);
    if (!config_file.is_open()) {
        return false;
    }
    
    // Redirect cout to file temporarily
    std::streambuf* cout_buffer = std::cout.rdbuf();
    std::cout.rdbuf(config_file.rdbuf());
    
    print_config_file_template();
    
    // Restore cout
    std::cout.rdbuf(cout_buffer);
    config_file.close();
    
    return true;
}

std::vector<std::string> ConfigManager::validate_config() {
    std::vector<std::string> errors;
    
    // Validate temperature
    double temp = get_double("default_temperature");
    if (!ConfigUtils::is_valid_temperature(temp)) {
        errors.push_back("Invalid temperature: " + std::to_string(temp) + " K");
    }
    
    // Validate concentration
    double conc = get_double("default_concentration");
    if (!ConfigUtils::is_valid_concentration(conc)) {
        errors.push_back("Invalid concentration: " + std::to_string(conc) + " M");
    }
    
    // Validate pressure
    double press = get_double("default_pressure");
    if (!ConfigUtils::is_valid_pressure(press)) {
        errors.push_back("Invalid pressure: " + std::to_string(press) + " atm");
    }
    
    // Validate file size
    size_t file_size = get_size_t("max_file_size_mb");
    if (!ConfigUtils::is_valid_file_size(file_size)) {
        errors.push_back("Invalid max file size: " + std::to_string(file_size) + " MB");
    }
    
    // Validate sort column
    int column = get_int("default_sort_column");
    if (column < 2 || column > 10) {
        errors.push_back("Invalid sort column: " + std::to_string(column) + " (must be 2-10)");
    }
    
    // Validate output format
    std::string format = get_string("default_output_format");
    if (format != "text" && format != "csv") {
        errors.push_back("Invalid output format: " + format + " (must be 'text' or 'csv')");
    }
    
    // Validate file extensions
    if (!validate_file_extensions()) {
        errors.push_back("Invalid file extensions format");
    }
    
    return errors;
}

bool ConfigManager::validate_file_extensions() {
    // Validate input extensions
    std::string input_exts = get_string("input_extensions");
    auto input_list = ConfigUtils::split_string(input_exts, ',');
    for (const auto& ext : input_list) {
        std::string trimmed_ext = trim(ext);
        if (!ConfigUtils::is_valid_extension(trimmed_ext)) {
            return false;
        }
    }
    
    // Validate output extensions
    std::string output_exts = get_string("output_extensions");
    auto output_list = ConfigUtils::split_string(output_exts, ',');
    for (const auto& ext : output_list) {
        std::string trimmed_ext = trim(ext);
        if (!ConfigUtils::is_valid_extension(trimmed_ext)) {
            return false;
        }
    }
    
    return true;
}

std::vector<std::string> ConfigManager::get_input_extensions() {
    std::string exts = get_string("input_extensions");
    auto result = ConfigUtils::split_string(exts, ',');
    for (auto& ext : result) {
        ext = trim(ext);
        if (!ext.empty() && ext[0] != '.') {
            ext = "." + ext;
        }
    }
    return result;
}

std::vector<std::string> ConfigManager::get_output_extensions() {
    std::string exts = get_string("output_extensions");
    auto result = ConfigUtils::split_string(exts, ',');
    for (auto& ext : result) {
        ext = trim(ext);
        if (!ext.empty() && ext[0] != '.') {
            ext = "." + ext;
        }
    }
    return result;
}

bool ConfigManager::is_valid_input_extension(const std::string& ext) {
    auto valid_exts = get_input_extensions();
    return std::find(valid_exts.begin(), valid_exts.end(), ext) != valid_exts.end();
}

bool ConfigManager::is_valid_output_extension(const std::string& ext) {
    auto valid_exts = get_output_extensions();
    return std::find(valid_exts.begin(), valid_exts.end(), ext) != valid_exts.end();
}

// Convenient getters
std::string ConfigManager::get_default_output_extension() {
    return get_string("output_extension");
}

std::string ConfigManager::get_default_input_extension() {
    auto input_exts = get_input_extensions();
    return input_exts.empty() ? ".com" : input_exts[0];
}

double ConfigManager::get_default_temperature() {
    return get_double("default_temperature");
}

double ConfigManager::get_default_concentration() {
    return get_double("default_concentration");
}

double ConfigManager::get_default_pressure() {
    return get_double("default_pressure");
}

unsigned int ConfigManager::get_default_threads() {
    std::string thread_str = get_string("default_threads");
    if (thread_str == "half") {
        return std::max(1u, std::thread::hardware_concurrency() / 2);
    } else if (thread_str == "max") {
        return std::thread::hardware_concurrency();
    } else {
        unsigned int threads = get_uint("default_threads", 0);
        if (threads == 0) {
            return std::max(1u, std::thread::hardware_concurrency() / 2);
        }
        return threads;
    }
}

std::string ConfigManager::get_default_output_format() {
    return get_string("default_output_format");
}

size_t ConfigManager::get_default_max_file_size() {
    return get_size_t("max_file_size_mb");
}

void ConfigManager::apply_command_line_overrides(const std::unordered_map<std::string, std::string>& overrides) {
    for (const auto& override : overrides) {
        if (has_key(override.first)) {
            set_value(override.first, override.second);
        }
    }
}

void ConfigManager::reload_config() {
    initialize_default_values();
    if (!config_file_path.empty()) {
        load_config(config_file_path);
    }
}

// ConfigUtils namespace implementation
namespace ConfigUtils {

bool file_exists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

bool is_readable(const std::string& path) {
    std::ifstream file(path);
    return file.is_open();
}

bool is_writable(const std::string& path) {
    std::ofstream file(path, std::ios::app);
    return file.is_open();
}

std::string get_config_search_paths() {
    std::ostringstream oss;
    oss << "Configuration file search paths:\n";
    oss << "  1. ./" << DEFAULT_CONFIG_FILENAME << "\n";
    oss << "  2. ./" << ALT_CONFIG_FILENAME << "\n";
    
    ConfigManager temp_manager;
    std::string home = temp_manager.get_user_home_directory();
    if (!home.empty()) {
        oss << "  3. " << home << "/" << DEFAULT_CONFIG_FILENAME << "\n";
        oss << "  4. " << home << "/" << ALT_CONFIG_FILENAME << "\n";
    }
    
    #ifndef _WIN32
    oss << "  5. /etc/gaussian_extractor/" << ALT_CONFIG_FILENAME << "\n";
    oss << "  6. /usr/local/etc/" << ALT_CONFIG_FILENAME << "\n";
    #endif
    
    return oss.str();
}

std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::istringstream iss(str);
    std::string token;
    
    while (std::getline(iss, token, delimiter)) {
        result.push_back(token);
    }
    
    return result;
}

std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return "";
    
    std::ostringstream oss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) oss << delimiter;
        oss << strings[i];
    }
    
    return oss.str();
}

bool string_to_bool(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return (lower == "true" || lower == "yes" || lower == "on" || lower == "1");
}

std::string bool_to_string(bool value) {
    return value ? "true" : "false";
}

bool is_valid_temperature(double temp) {
    return temp > 0.0 && temp < 10000.0; // Reasonable range for chemistry
}

bool is_valid_concentration(double conc) {
    return conc > 0.0 && conc <= 1000.0; // Up to 1000 M
}

bool is_valid_pressure(double pressure) {
    return pressure > 0.0 && pressure <= 1000.0; // Up to 1000 atm
}

bool is_valid_thread_count(unsigned int threads) {
    unsigned int max_threads = std::thread::hardware_concurrency();
    if (max_threads == 0) max_threads = 32; // Fallback
    return threads > 0 && threads <= max_threads * 2; // Allow some oversubscription
}

bool is_valid_file_size(size_t size_mb) {
    return size_mb > 0 && size_mb <= 10000; // Up to 10GB
}

bool is_valid_extension(const std::string& ext) {
    if (ext.empty()) return false;
    
    // Must start with dot and contain only alphanumeric characters
    std::regex ext_regex(R"(^\.[a-zA-Z0-9]+$)");
    return std::regex_match(ext, ext_regex);
}

int get_config_version(const std::string& config_content) {
    std::regex version_regex(R"(#\s*version\s*=\s*(\d+))");
    std::smatch match;
    
    if (std::regex_search(config_content, match, version_regex)) {
        return std::stoi(match[1].str());
    }
    
    return 1; // Default version
}

bool migrate_config_if_needed(const std::string& /* config_path */) {
    // Placeholder for future config format migrations
    return true;
}

} // namespace ConfigUtils