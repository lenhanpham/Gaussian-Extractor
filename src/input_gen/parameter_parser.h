/**
 * @file parameter_parser.h
 * @brief Header file for parameter file parsing functionality
 * @author Le Nhan Pham
 * @date 2025
 *
 * This header defines the ParameterParser class for reading and writing
 * parameter files used by the create_input module.
 */

#ifndef PARAMETER_PARSER_H
#define PARAMETER_PARSER_H

#include <string>
#include <unordered_map>
#include <vector>

/**
 * @class ParameterParser
 * @brief Parser for parameter files used in Gaussian input creation
 *
 * Provides functionality to read parameter files with key-value pairs,
 * handle multi-line values, and generate template parameter files.
 * Supports comments and flexible formatting.
 */
class ParameterParser
{
public:
    /**
     * @brief Constructor
     */
    ParameterParser();

    /**
     * @brief Load parameters from a file
     * @param filename Path to the parameter file
     * @return true if successful, false otherwise
     */
    bool loadFromFile(const std::string& filename);

    /**
     * @brief Save parameters to a file
     * @param filename Path to save the parameter file
     * @return true if successful, false otherwise
     */
    bool saveToFile(const std::string& filename) const;

    /**
     * @brief Get a parameter value as string
     * @param key Parameter name
     * @param default_value Default value if parameter not found
     * @return Parameter value or default
     */
    std::string getString(const std::string& key, const std::string& default_value = "") const;

    /**
     * @brief Get a parameter value as integer
     * @param key Parameter name
     * @param default_value Default value if parameter not found
     * @return Parameter value or default
     */
    int getInt(const std::string& key, int default_value = 0) const;

    /**
     * @brief Get a parameter value as boolean
     * @param key Parameter name
     * @param default_value Default value if parameter not found
     * @return Parameter value or default
     */
    bool getBool(const std::string& key, bool default_value = false) const;

    /**
     * @brief Set a parameter value
     * @param key Parameter name
     * @param value Parameter value
     */
    void setString(const std::string& key, const std::string& value);

    /**
     * @brief Set a parameter value from integer
     * @param key Parameter name
     * @param value Parameter value
     */
    void setInt(const std::string& key, int value);

    /**
     * @brief Set a parameter value from boolean
     * @param key Parameter name
     * @param value Parameter value
     */
    void setBool(const std::string& key, bool value);

    /**
     * @brief Check if a parameter exists
     * @param key Parameter name
     * @return true if parameter exists, false otherwise
     */
    bool hasParameter(const std::string& key) const;

    /**
     * @brief Generate a template parameter file for a specific calculation type
     * @param calc_type Calculation type (sp, opt_freq, etc.)
     * @param filename Output filename
     * @return true if successful, false otherwise
     */
    bool generateTemplate(const std::string& calc_type, const std::string& filename) const;

    /**
     * @brief Generate template files for all supported calculation types
     * @param directory Directory to save template files
     * @return true if successful, false otherwise
     */
    bool generateAllTemplates(const std::string& directory) const;

    /**
     * @brief Generate a general parameter template with all possible parameters
     * @param filename Output filename
     * @return true if successful, false otherwise
     */
    bool generateGeneralTemplate(const std::string& filename) const;

    /**
     * @brief Get list of supported calculation types
     * @return Vector of calculation type names
     */
    std::vector<std::string> getSupportedCalcTypes() const;

    /**
     * @brief Clear all parameters
     */
    void clear();

private:
    std::unordered_map<std::string, std::string> parameters_;  ///< Parameter storage

    /**
     * @brief Trim whitespace from string
     * @param str String to trim
     * @return Trimmed string
     */
    std::string trim(const std::string& str) const;

    /**
     * @brief Parse a line from the parameter file
     * @param line Line to parse
     * @param[out] key Parsed key
     * @param[out] value Parsed value
     * @return true if parsing successful, false otherwise
     */
    bool parseLine(const std::string& line, std::string& key, std::string& value);

    /**
     * @brief Handle multi-line parameter values
     * @param lines Vector of file lines
     * @param start_index Starting index for multi-line parsing
     * @return Parsed multi-line value
     */
    std::string parseMultiLineValue(const std::vector<std::string>& lines, size_t& start_index);

    /**
     * @brief Parse multi-line parameter content with support for punctuation marks
     * @param lines Vector of file lines
     * @param start_index Starting index (will be updated to skip processed lines)
     * @return Parsed multi-line content
     */
    std::string parseMultiLineParameter(const std::vector<std::string>& lines, size_t& start_index);

    /**
     * @brief Create template content for a specific calculation type
     * @param calc_type Calculation type
     * @return Template content as string
     */
    std::string createTemplateContent(const std::string& calc_type) const;

    /**
     * @brief Create general template content with all possible parameters
     * @return General template content as string
     */
    std::string createGeneralTemplateContent() const;
};

#endif  // PARAMETER_PARSER_H