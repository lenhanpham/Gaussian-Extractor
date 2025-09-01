#ifndef UTILS_H
#define UTILS_H

#include <string>

/**
 * @enum FileReadMode
 * @brief Specifies the mode for reading file content
 */
enum class FileReadMode {
    FULL,  ///< Read entire file
    TAIL,  ///< Read last N lines of file
    SMART  ///< Read last N lines, fall back to full file if pattern not found
};

/**
 * @namespace FileUtils
 * @brief Utility functions for file operations
 */
namespace Utils {

/**
 * @brief Read file content with specified mode
 * @param file_path Path to the file
 * @param mode Reading mode (FULL, TAIL, or SMART)
 * @param tail_lines Number of lines to read from end (if TAIL or SMART mode)
 * @param pattern Optional pattern to search for in SMART mode (if empty, no fallback)
 * @return File content as string
 * @throws std::runtime_error if file cannot be read
 *
 * Efficiently reads file content:
 * - FULL: Reads the entire file.
 * - TAIL: Reads the last specified number of lines.
 * - SMART: Reads the last specified number of lines; if pattern is not found, reads the entire file.
 */
std::string read_file_unified(const std::string& file_path, FileReadMode mode, size_t tail_lines = 0, const std::string& pattern = "");

}  // namespace FileUtils

#endif // UTILS_H