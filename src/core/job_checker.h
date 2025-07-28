/**
 * @file job_checker.h
 * @brief Job status checking and file organization system for Gaussian calculations
 * @author Le Nhan Pham
 * @date 2025
 * 
 * This header defines a comprehensive job management system for organizing
 * Gaussian computational chemistry calculations based on their completion status.
 * The system can identify completed jobs, failed jobs, and specific failure types
 * like PCM convergence issues, then organize them into appropriate directories.
 * 
 * @section Job Status Categories
 * The system recognizes several job completion states:
 * - COMPLETED: Normal termination with successful results
 * - ERROR: General error termination requiring investigation
 * - PCM_FAILED: Specific PCM (Polarizable Continuum Model) convergence failure
 * - RUNNING: Job still in progress (not yet terminated)
 * - UNKNOWN: Cannot determine status from available information
 * 
 * @section File Organization
 * The job checker can automatically organize files into directories based on status:
 * - Completed jobs → "done" directory (configurable suffix)
 * - Error jobs → "errorJobs" directory  
 * - PCM failures → "PCMMkU" directory
 * - Related files (.gau, .chk) are moved together with .log files
 * 
 * @section Integration
 * The job checker integrates with the main processing system to provide:
 * - Resource-aware processing with memory and thread management
 * - Progress reporting for large job sets
 * - Detailed error reporting and statistics
 * - Thread-safe operation for concurrent job checking
 */

#ifndef JOB_CHECKER_H
#define JOB_CHECKER_H

#include "gaussian_extractor.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

/**
 * @enum JobStatus
 * @brief Enumeration of possible Gaussian job completion states
 * 
 * This enumeration defines all possible states that a Gaussian calculation
 * job can be in, based on analysis of the log file content. The status
 * determination is based on specific patterns found in Gaussian output.
 */
enum class JobStatus {
    COMPLETED,    ///< Job completed successfully with "Normal termination"
    ERROR,        ///< Job terminated with error (various error patterns)
    PCM_FAILED,   ///< Specific failure due to PCM convergence issues
    RUNNING,      ///< Job is still in progress (no termination found)
    UNKNOWN       ///< Status cannot be determined from available information
};

/**
 * @struct JobCheckResult
 * @brief Complete result information for a single job status check
 * 
 * This structure contains all information gathered during job status checking
 * for a single Gaussian log file. It includes the determined status, any error
 * messages found, and lists of related files that should be moved together.
 * 
 * @section Related Files
 * Gaussian calculations typically generate multiple files:
 * - .log: Main output file with results and status
 * - .gau: Input file used for the calculation
 * - .chk: Checkpoint file with intermediate data
 * - .fchk: Formatted checkpoint file (sometimes)
 * 
 * All related files are moved together to maintain calculation integrity.
 */
struct JobCheckResult {
    std::string filename;                    ///< Original log file name
    JobStatus status;                        ///< Determined job status
    std::string error_message;               ///< Error message if job failed
    std::vector<std::string> related_files;  ///< List of related files (.gau, .chk, etc.)
    
    /**
     * @brief Default constructor creating unknown status result
     */
    JobCheckResult() : status(JobStatus::UNKNOWN) {}
    
    /**
     * @brief Constructor with filename and status
     * @param file Log file name
     * @param stat Determined job status
     */
    JobCheckResult(const std::string& file, JobStatus stat) 
        : filename(file), status(stat) {}
};

/**
 * @struct CheckSummary
 * @brief Statistical summary of job checking operations
 * 
 * This structure collects comprehensive statistics about job checking operations
 * including file counts, success rates, errors encountered, and performance metrics.
 * It provides detailed reporting for users to understand the scope and success
 * of job organization operations.
 * 
 * @section Metrics Collected
 * - File processing statistics (total, processed, matched)
 * - File movement statistics (successful moves, failures)
 * - Error collection for troubleshooting
 * - Timing information for performance analysis
 */
struct CheckSummary {
    size_t total_files;                    ///< Total number of files considered
    size_t processed_files;                ///< Number of files successfully processed
    size_t matched_files;                  ///< Number of files matching criteria
    size_t moved_files;                    ///< Number of files successfully moved
    size_t failed_moves;                   ///< Number of file move operations that failed
    std::vector<std::string> errors;       ///< Collection of error messages encountered
    double execution_time;                 ///< Total execution time in seconds
    
    /**
     * @brief Default constructor initializing all counters to zero
     */
    CheckSummary() : total_files(0), processed_files(0), matched_files(0), 
                    moved_files(0), failed_moves(0), execution_time(0.0) {}
};

/**
 * @class JobChecker
 * @brief Comprehensive job status checking and file organization system
 * 
 * The JobChecker class provides a complete solution for analyzing Gaussian
 * calculation job status and organizing files based on their completion state.
 * It supports multiple checking modes and provides detailed reporting and
 * statistics for job management workflows.
 * 
 * @section Key Features
 * - Multi-threaded job status analysis with resource management
 * - Automatic file organization based on job status
 * - Comprehensive error detection and reporting
 * - Integration with job scheduler resource limits
 * - Progress reporting for large job sets
 * - Thread-safe operation for concurrent processing
 * 
 * @section Checking Modes
 * - check_completed_jobs(): Organize successfully completed calculations
 * - check_error_jobs(): Organize jobs that terminated with errors
 * - check_pcm_failures(): Specifically handle PCM convergence failures
 * - check_all_job_types(): Comprehensive checking of all job types
 * 
 * @section Resource Management
 * The JobChecker uses the ProcessingContext to coordinate resource usage:
 * - Memory usage monitoring and limiting
 * - File handle management to prevent exhaustion
 * - Thread-safe error collection and reporting
 * - Integration with system and job scheduler limits
 * 
 * @note All checking operations are designed to be safe and reversible,
 *       with comprehensive logging and error reporting
 */
class JobChecker {
private:
    std::shared_ptr<ProcessingContext> context; ///< Shared processing context for resource management
    bool quiet_mode;                           ///< Suppress non-essential output messages
    bool show_error_details;                   ///< Display detailed error messages from log files
    
public:
    /**
     * @brief Constructor with processing context and display options
     * @param ctx Shared processing context for resource coordination
     * @param quiet Suppress non-essential output (default: false)
     * @param show_details Display detailed error messages (default: false)
     * 
     * Creates a JobChecker instance with the specified processing context
     * and display preferences. The processing context provides access to
     * shared resource managers and coordination with other system components.
     */
    explicit JobChecker(std::shared_ptr<ProcessingContext> ctx, bool quiet = false, bool show_details = false);
    
    /**
     * @defgroup MainChecking Main Job Checking Functions
     * @brief Primary functions that replicate bash script functionality
     * @{
     */
    
    /**
     * @brief Check and organize completed job calculations
     * @param log_files Vector of log file paths to check
     * @param target_dir_suffix Suffix for target directory name (default: "done")
     * @return CheckSummary with statistics and results
     * 
     * Identifies Gaussian calculations that completed successfully (normal termination)
     * and moves them along with related files to a target directory. The target
     * directory name is formed by appending the suffix to the current directory name.
     * 
     * @section Detection Criteria
     * - Looks for "Normal termination" in log file content
     * - Verifies job completed without errors
     * - Checks for proper calculation convergence
     * 
     * @note This function replicates the behavior of the original bash script
     *       for organizing completed calculations
     */
    CheckSummary check_completed_jobs(const std::vector<std::string>& log_files, 
                                     const std::string& target_dir_suffix = "done");
    
    /**
     * @brief Check and organize jobs that terminated with errors
     * @param log_files Vector of log file paths to check
     * @param target_dir Target directory name (default: "errorJobs")
     * @return CheckSummary with statistics and results
     * 
     * Identifies Gaussian calculations that terminated with various types of errors
     * and organizes them into an error directory for investigation and potential
     * restart with modified parameters.
     * 
     * @section Error Detection
     * - Searches for error termination patterns
     * - Excludes PCM-specific failures (handled separately)
     * - Captures error messages for detailed reporting
     * - Identifies convergence failures and resource issues
     */
    CheckSummary check_error_jobs(const std::vector<std::string>& log_files, 
                                 const std::string& target_dir = "errorJobs");
    
    /**
     * @brief Check and organize jobs with PCM convergence failures
     * @param log_files Vector of log file paths to check
     * @param target_dir Target directory name (default: "PCMMkU")
     * @return CheckSummary with statistics and results
     * 
     * Specifically identifies calculations that failed due to PCM (Polarizable
     * Continuum Model) convergence issues. These failures often require specific
     * restart strategies different from general errors.
     * 
     * @section PCM Detection
     * - Looks for PCM-specific error patterns
     * - Identifies convergence failures in solvent calculations
     * - Detects self-consistent reaction field issues
     * - Captures PCM-related warnings and errors
     */
    CheckSummary check_pcm_failures(const std::vector<std::string>& log_files, 
                                   const std::string& target_dir = "PCMMkU");
    
    /**
     * @brief Run comprehensive checking of all job types
     * @param log_files Vector of log file paths to check
     * @return CheckSummary with combined statistics from all checks
     * 
     * Performs complete job status analysis by running all checking functions
     * in sequence. Provides comprehensive organization of all job types in
     * a single operation.
     * 
     * @section Check Sequence
     * 1. Check for completed jobs first (highest priority)
     * 2. Check for PCM failures (specific error type)
     * 3. Check for general errors (catch remaining failures)
     * 4. Report comprehensive statistics
     */
    CheckSummary check_all_job_types(const std::vector<std::string>& log_files);
    
    /** @} */ // end of MainChecking group
    
    /**
     * @defgroup IndividualChecking Individual File Checking Functions
     * @brief Functions for analyzing single job files
     * @{
     */
    
    /**
     * @brief Determine the status of a single Gaussian job
     * @param log_file Path to the log file to analyze
     * @return JobCheckResult with status and error information
     * 
     * Analyzes a single Gaussian log file to determine its completion status.
     * This is the core analysis function used by all higher-level checking
     * operations.
     * 
     * @section Analysis Process
     * 1. Read and parse log file content
     * 2. Search for termination patterns
     * 3. Identify specific error types if present
     * 4. Extract error messages for reporting
     * 5. Find related files for potential organization
     */
    JobCheckResult check_job_status(const std::string& log_file);
    
    /** @} */ // end of IndividualChecking group
    
    /**
     * @defgroup FileOperations File and Directory Operations
     * @brief Functions for organizing and moving job files
     * @{
     */
    
    /**
     * @brief Move job files to target directory with related files
     * @param result JobCheckResult containing file information
     * @param target_dir Target directory for file organization
     * @return true if all files moved successfully, false otherwise
     * 
     * Moves the main log file and all related files (.gau, .chk, etc.) to
     * the specified target directory. Ensures all files for a calculation
     * are kept together to maintain data integrity.
     * 
     * @section Move Strategy
     * - Creates target directory if it doesn't exist
     * - Moves files atomically when possible
     * - Handles filename conflicts gracefully
     * - Provides detailed error reporting for failures
     */
    bool move_job_files(const JobCheckResult& result, const std::string& target_dir);
    
    /**
     * @brief Find all files related to a Gaussian calculation
     * @param log_file Path to the main log file
     * @return Vector of related file paths
     * 
     * Identifies all files associated with a Gaussian calculation based on
     * the base filename. Typically includes input files (.gau), checkpoint
     * files (.chk), and other generated files.
     * 
     * @section Related File Types
     * - Input files: .gau, .com, .gjf
     * - Checkpoint files: .chk, .fchk
     * - Output files: .log, .out
     * - Other: .rwf, .int, .d2e, etc.
     */
    std::vector<std::string> find_related_files(const std::string& log_file);
    
    /**
     * @brief Create target directory for file organization
     * @param target_dir Path to directory to create
     * @return true if directory created or already exists, false otherwise
     * 
     * Creates the specified directory and any necessary parent directories.
     * Handles permissions and provides appropriate error reporting.
     */
    bool create_target_directory(const std::string& target_dir);
    
    /**
     * @brief Get the name of the current working directory
     * @return Current directory name (not full path)
     * 
     * Extracts just the directory name from the full path for use in
     * generating target directory names with suffixes.
     */
    std::string get_current_directory_name();
    
    /** @} */ // end of FileOperations group
    
    /**
     * @defgroup ProgressReporting Progress and Statistics Reporting
     * @brief Functions for user feedback and operation reporting
     * @{
     */
    
    /**
     * @brief Report progress of ongoing operation
     * @param current Number of items processed so far
     * @param total Total number of items to process
     * @param operation Description of current operation
     * 
     * Provides real-time progress feedback for long-running operations.
     * Respects quiet mode setting and formats output appropriately.
     */
    void report_progress(size_t current, size_t total, const std::string& operation);
    
    /**
     * @brief Print comprehensive summary of checking operation
     * @param summary CheckSummary structure with statistics
     * @param operation Description of completed operation
     * 
     * Displays detailed statistics about the completed checking operation
     * including file counts, success rates, errors, and performance metrics.
     */
    void print_summary(const CheckSummary& summary, const std::string& operation);
    
    /** @} */ // end of ProgressReporting group
    
private:
    /**
     * @defgroup ContentAnalysis Content Analysis Functions
     * @brief Private functions for analyzing log file content and patterns
     * @{
     */
    
    /**
     * @brief Check if log file shows normal termination
     * @param content Complete content of the log file
     * @return true if normal termination detected, false otherwise
     * 
     * Searches for "Normal termination" patterns that indicate successful
     * completion of a Gaussian calculation. This is the primary indicator
     * of a successfully completed job.
     */
    bool check_normal_termination(const std::string& content);
    
    /**
     * @brief Check if log file shows error termination
     * @param content Complete content of the log file
     * @param error_msg Reference to store extracted error message
     * @return true if error termination detected, false otherwise
     * 
     * Searches for various error termination patterns in Gaussian output.
     * Extracts specific error messages for detailed reporting and debugging.
     * Excludes PCM-specific errors which are handled separately.
     */
    bool check_error_termination(const std::string& content, std::string& error_msg);
    
    /**
     * @brief Check if log file shows PCM convergence failure
     * @param content Complete content of the log file
     * @return true if PCM failure detected, false otherwise
     * 
     * Specifically searches for PCM (Polarizable Continuum Model) convergence
     * failures and related solvent calculation issues. These require different
     * handling strategies than general errors.
     */
    bool check_pcm_failure(const std::string& content);
    
    /** @} */ // end of ContentAnalysis group
    
    /**
     * @defgroup IndependentChecking Independent Checking Methods
     * @brief Direct checking methods for command-specific logic
     * @{
     */
    
    /**
     * @brief Direct error checking without other status considerations
     * @param log_file Path to log file to check
     * @return JobCheckResult with error-specific analysis
     * 
     * Performs error checking independent of completion status.
     * Used by error-specific commands that focus only on error detection
     * regardless of whether the job might also show completion.
     */
    JobCheckResult check_error_directly(const std::string& log_file);
    
    /**
     * @brief Direct PCM failure checking without other status considerations
     * @param log_file Path to log file to check
     * @return JobCheckResult with PCM-specific analysis
     * 
     * Performs PCM failure checking independent of other status indicators.
     * Used by PCM-specific commands that focus only on PCM issues.
     */
    JobCheckResult check_pcm_directly(const std::string& log_file);
    
    /** @} */ // end of IndependentChecking group
    
    /**
     * @defgroup FileReading File Reading with Resource Management
     * @brief Private functions for safe file reading with resource limits
     * @{
     */
    
    /**
     * @brief Read last N lines from a file efficiently
     * @param filename Path to file to read
     * @param lines Number of lines to read from end (default: 10)
     * @return String containing the last N lines
     * 
     * Efficiently reads the tail of large log files without loading the entire
     * file into memory. Uses resource management to prevent memory exhaustion.
     * Useful for checking termination status which appears at file end.
     */
    std::string read_file_tail(const std::string& filename, size_t lines = 10);
    
    /**
     * @brief Read complete file content with resource management
     * @param filename Path to file to read
     * @return String containing complete file content
     * 
     * Reads entire file content while respecting memory limits and file handle
     * constraints. Uses the ProcessingContext resource managers for coordination.
     * 
     * @note Should only be used for files that have been validated for size
     *       to prevent memory exhaustion
     */
    std::string read_file_content(const std::string& filename);
    
    /** @} */ // end of FileReading group
    
    /**
     * @defgroup UtilityFunctions Utility Functions
     * @brief Private utility functions for string processing and logging
     * @{
     */
    
    /**
     * @brief Extract base filename without extension from log file path
     * @param log_file Full path to log file
     * @return Base filename for finding related files
     * 
     * Extracts the base filename (without directory path or extension)
     * for use in finding related files (.gau, .chk, etc.) that share
     * the same base name.
     */
    std::string extract_base_name(const std::string& log_file);
    
    /**
     * @brief Log informational message with appropriate formatting
     * @param message Message to log
     * 
     * Outputs informational messages with appropriate formatting and
     * timestamps. Respects quiet mode setting to suppress non-essential output.
     */
    void log_message(const std::string& message);
    
    /**
     * @brief Log error message with appropriate formatting and collection
     * @param error Error message to log
     * 
     * Outputs error messages and adds them to the error collection in the
     * ProcessingContext for later summary reporting. Always displayed
     * regardless of quiet mode.
     */
    void log_error(const std::string& error);
    
    /** @} */ // end of UtilityFunctions group
};

/**
 * @namespace JobCheckerUtils
 * @brief Utility functions supporting job checking operations
 * 
 * The JobCheckerUtils namespace provides a collection of utility functions
 * that support job checking operations including file system operations,
 * string processing for log file parsing, and directory name generation.
 */
namespace JobCheckerUtils {
    /**
     * @defgroup JobFileUtils File System Utilities
     * @brief Utilities for file system operations in job checking
     * @{
     */
    
    /**
     * @brief Check if a file exists and is accessible
     * @param path Path to file to check
     * @return true if file exists and is accessible, false otherwise
     */
    bool file_exists(const std::string& path);
    
    /**
     * @brief Validate that a log file is suitable for processing
     * @param path Path to log file to validate
     * @param max_size_mb Maximum allowed file size in megabytes
     * @return true if file is valid for processing, false otherwise
     * 
     * Checks that the file exists, is readable, has appropriate extension,
     * and is within size limits for safe processing.
     */
    bool is_valid_log_file(const std::string& path, size_t max_size_mb);
    
    /**
     * @brief Extract file extension from path
     * @param path File path to analyze
     * @return File extension including the leading dot (e.g., ".log")
     */
    std::string get_file_extension(const std::string& path);
    
    /** @} */ // end of JobFileUtils group
    
    /**
     * @defgroup JobStringUtils String Processing Utilities
     * @brief Utilities for parsing and processing log file content
     * @{
     */
    
    /**
     * @brief Check if content contains a specific pattern
     * @param content Text content to search
     * @param pattern Pattern to search for
     * @return true if pattern found, false otherwise
     * 
     * Performs case-sensitive pattern matching for finding specific
     * text patterns in log file content.
     */
    bool contains_pattern(const std::string& content, const std::string& pattern);
    
    /**
     * @brief Extract error message from log file content
     * @param content Complete log file content
     * @return Extracted error message string
     * 
     * Searches for and extracts error messages from Gaussian log files.
     * Handles various error message formats and provides clean,
     * readable error descriptions.
     */
    std::string extract_error_message(const std::string& content);
    
    /**
     * @brief Get the last N lines from text content
     * @param content Text content to process
     * @param num_lines Number of lines to extract from end
     * @return Vector of strings containing the last N lines
     * 
     * Efficiently extracts the last N lines from text content.
     * Useful for checking job termination status which typically
     * appears at the end of log files.
     */
    std::vector<std::string> get_last_lines(const std::string& content, size_t num_lines);
    
    /** @} */ // end of JobStringUtils group
    
    /**
     * @defgroup JobDirUtils Directory Name Utilities
     * @brief Utilities for generating and managing directory names
     * @{
     */
    
    /**
     * @brief Generate directory name for completed jobs
     * @param suffix Suffix to append to current directory name (default: "done")
     * @return Generated directory name for organizing completed jobs
     * 
     * Creates appropriate directory names for organizing completed jobs
     * based on current directory name and specified suffix.
     */
    std::string generate_done_directory_name(const std::string& suffix = "done");
    
    /**
     * @brief Sanitize directory name for file system compatibility
     * @param name Directory name to sanitize
     * @return Sanitized directory name safe for file system use
     * 
     * Removes or replaces characters that may cause issues in directory
     * names on various file systems. Ensures cross-platform compatibility.
     */
    std::string sanitize_directory_name(const std::string& name);
    
    /** @} */ // end of JobDirUtils group
}

#endif // JOB_CHECKER_H