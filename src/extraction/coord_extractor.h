#ifndef COORD_EXTRACTOR_H
#define COORD_EXTRACTOR_H

#include "gaussian_extractor.h"
#include "job_management/job_checker.h"
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>


/**
 * @file coord_extractor.h
 * @brief Header file for coordinate extraction command xyz to extract coordinates from Gaussian outputs
 */
/**
 * @struct ExtractSummary
 * @brief Statistical summary of coordinate extraction operations
 *
 * Collects statistics about extraction operations including file counts,
 * success rates, errors, and performance metrics.
 */
struct ExtractSummary
{
    size_t                   total_files;       ///< Total number of files considered
    size_t                   processed_files;   ///< Number of files successfully processed
    size_t                   extracted_files;   ///< Number of files with successful extraction
    size_t                   failed_files;      ///< Number of files where extraction failed
    size_t                   moved_to_final;    ///< Number of XYZ files moved to final_coord dir
    size_t                   moved_to_running;  ///< Number of XYZ files moved to running_coord dir
    std::vector<std::string> errors;            ///< Collection of error messages encountered
    double                   execution_time;    ///< Total execution time in seconds

    ExtractSummary()
        : total_files(0), processed_files(0), extracted_files(0), failed_files(0), moved_to_final(0),
          moved_to_running(0), execution_time(0.0)
    {}
};

/**
 * @class CoordExtractor
 * @brief System for extracting final coordinates from Gaussian log files
 *
 * Provides functionality to process Gaussian log files, extract the last set
 * of Cartesian coordinates, and save them in XYZ format. Supports parallel
 * processing with resource management integration.
 *
 * @section Key Features
 * - Efficient backward reading for large log files
 * - Automatic fallback to full read if needed
 * - Multi-threaded processing
 * - Resource-aware operation
 * - Detailed reporting and error handling
 * - Moves XYZ files to status-based directories
 */
class CoordExtractor
{
private:
    std::shared_ptr<ProcessingContext> context;     ///< Shared processing context
    bool                               quiet_mode;  ///< Suppress non-essential output

    /**
     * @brief Extract coordinates from a single log file
     * @param log_file Path to the log file
     * @param error_msg Reference to store any error message
     * @return Pair of success flag and job status
     *
     * Processes a single file: reads content efficiently, parses the last
     * orientation section, converts to XYZ format, writes output, and determines
     * job status for directory assignment.
     */
    std::pair<bool, JobStatus> extract_from_file(const std::string&                     log_file,
                                                 const std::unordered_set<std::string>& conflicting_base_names,
                                                 std::string&                           error_msg);

    /**
     * @brief Get element symbol from atomic number
     * @param atomic_num Atomic number (1-118)
     * @return Element symbol string
     *
     * Returns the standard two-letter (or one-letter) element symbol.
     * Uses modern names where applicable.
     */
    std::string get_atomic_symbol(int atomic_num);

    /**
     * @brief Generate output XYZ filename from log file
     * @param log_file Input log file path
     * @return Generated .xyz filename
     */
    std::string generate_xyz_filename(const std::string&                     log_file,
                                      const std::unordered_set<std::string>& conflicting_base_names);

    /**
     * @brief Move XYZ file to appropriate directory based on job status
     * @param xyz_file Path to XYZ file
     * @param status Job status determining target directory
     * @param error_msg Reference to store any error
     * @return true if move successful
     */
    bool move_xyz_file(const std::string& xyz_file, JobStatus status, std::string& error_msg);

    /**
     * @brief Create target directory if it doesn't exist
     * @param target_dir Directory path
     * @return true if directory exists or was created
     */
    bool create_target_directory(const std::string& target_dir);

    /**
     * @brief Get current directory name
     * @return Current directory name
     */
    std::string get_current_directory_name();

    /**
     * @brief Report progress of extraction
     * @param current Processed files count
     * @param total Total files
     */
    void report_progress(size_t current, size_t total);

    /**
     * @brief Log informational message
     * @param message Message to log
     */
    void log_message(const std::string& message);

    /**
     * @brief Log error message
     * @param error Error to log
     */
    void log_error(const std::string& error);

public:
    /**
     * @brief Constructor with processing context
     * @param ctx Shared processing context
     * @param quiet Quiet mode flag
     */
    explicit CoordExtractor(std::shared_ptr<ProcessingContext> ctx, bool quiet = false);

    /**
     * @brief Extract coordinates from multiple log files
     * @param log_files Vector of log file paths
     * @return ExtractSummary with results
     *
     * Processes files in parallel, extracts coordinates, writes XYZ files,
     * and moves them to appropriate directories based on job status.
     */
    ExtractSummary extract_coordinates(const std::vector<std::string>& log_files);

    /**
     * @brief Print extraction summary
     * @param summary Extraction summary
     * @param operation Operation description
     */
    void print_summary(const ExtractSummary& summary, const std::string& operation);
};

#endif  // COORD_EXTRACTOR_H