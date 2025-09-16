/**
 * @file module_executor.h
 * @brief Command execution functions for Gaussian Extractor
 * @author Le Nhan Pham
 * @date 2025
 *
 * This header defines the module execution functions that implement
 * the actual functionality for each supported command in Gaussian Extractor.
 * These functions are called by the main application after command parsing.
 *
 * @section Module Execution Functions
 * - execute_extract_command: Process Gaussian log files and extract thermodynamic data
 * - execute_check_done_command: Identify and organize completed calculations
 * - execute_check_errors_command: Identify and organize failed calculations
 * - execute_check_pcm_command: Identify PCM convergence failures
 * - execute_check_all_command: Run comprehensive job status checks
 * - execute_high_level_kj_command: Calculate high-level energies with kJ/mol output
 * - execute_high_level_au_command: Calculate high-level energies with atomic unit output
 */

#ifndef MODULE_EXECUTOR_H
#define MODULE_EXECUTOR_H

#include "command_system.h"

/**
 * @defgroup ModuleExecutors Module Execution Functions
 * @brief Functions for executing specific modules with given contexts
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

/**
 * @brief Execute the extract command for thermodynamic data extraction
 * @param context Configured command context with extraction parameters
 * @return Exit code: 0 for success, non-zero for various error conditions
 *
 * Processes Gaussian log files to extract thermodynamic data including:
 * - Electronic energies (SCF, correlation, etc.)
 * - Thermal corrections (enthalpy, Gibbs free energy)
 * - Zero-point energy corrections
 * - Phase corrections for concentration effects
 *
 * Supports multi-threaded processing with resource management and
 * progress reporting. Output can be formatted as text, CSV, or other formats.
 */
int execute_extract_command(const CommandContext& context);

/**
 * @brief Execute the check-done command for completed job organization
 * @param context Configured command context with checking parameters
 * @return Exit code: 0 for success, non-zero for various error conditions
 *
 * Identifies Gaussian calculations that have completed successfully
 * (normal termination) and organizes them into a designated directory.
 * Moves associated files (.log, .chk, .gau) together for easy management.
 */
int execute_check_done_command(const CommandContext& context);

/**
 * @brief Execute the check-errors command for failed job organization
 * @param context Configured command context with checking parameters
 * @return Exit code: 0 for success, non-zero for various error conditions
 *
 * Identifies Gaussian calculations that terminated with errors and
 * organizes them into an error directory. Can optionally display
 * detailed error messages from the log files for debugging.
 */
int execute_check_errors_command(const CommandContext& context);

/**
 * @brief Execute the check-pcm command for PCM convergence failure organization
 * @param context Configured command context with checking parameters
 * @return Exit code: 0 for success, non-zero for various error conditions
 *
 * Specifically identifies calculations that failed due to PCM (Polarizable
 * Continuum Model) convergence issues and organizes them separately.
 * These jobs often need different restart strategies than general errors.
 */
int execute_check_pcm_command(const CommandContext& context);

/**
 * @brief Execute comprehensive checking of all job types
 * @param context Configured command context with checking parameters
 * @return Exit code: 0 for success, non-zero for various error conditions
 *
 * Runs all job checking operations in sequence:
 * - Completed jobs (check-done)
 * - Error jobs (check-errors)
 * - PCM failures (check-pcm)
 *
 * Provides a comprehensive job management workflow in a single command.
 */
int execute_check_all_command(const CommandContext& context);

/**
 * @brief Execute the check-imaginary command for jobs with imaginary frequencies
 * @param context Configured command context with checking parameters
 * @return Exit code: 0 for success, non-zero for various error conditions
 *
 * Identifies Gaussian calculations that have imaginary (negative) vibrational
 * frequencies and organizes them into a designated directory. This is useful
 * for identifying transition states or failed optimizations.
 */
int execute_check_imaginary_command(const CommandContext& context);

/**
 * @brief Execute high-level energy calculation with kJ/mol output
 * @param context Configured command context with calculation parameters
 * @return Exit code: 0 for success, non-zero for various error conditions
 *
 * Combines high-level electronic energies with low-level thermal corrections
 * to calculate accurate thermodynamic properties. Uses a two-level approach:
 * - High-level single-point energies (current directory)
 * - Low-level geometry optimization and frequency data (parent directory)
 *
 * Output focuses on Gibbs free energies in kJ/mol units for easy comparison.
 * Supports temperature and concentration corrections for solution-phase calculations.
 */
int execute_high_level_kj_command(const CommandContext& context);

/**
 * @brief Execute detailed energy component analysis with atomic unit output
 * @param context Configured command context with calculation parameters
 * @return Exit code: 0 for success, non-zero for various error conditions
 *
 * Provides detailed breakdown of energy components in high-level calculations:
 * - Electronic energy components (SCF, correlation, solvation) in atomic units
 * - Thermal correction components (ZPE, enthalpy, entropy) in atomic units
 * - Phase corrections and temperature effects
 * - Component-by-component analysis for troubleshooting
 *
 * Output in atomic units provides maximum precision for detailed analysis.
 * Useful for understanding energy contributions and validating calculations.
 */
int execute_high_level_au_command(const CommandContext& context);


/**
 * @brief Execute coordinate extraction from Gaussian log files
 * @param context Configured command context
 * @return Exit code: 0 for success, non-zero for errors
 *
 * Processes log files to extract final Cartesian coordinates
 * and save them in XYZ format. Supports parallel processing
 * with resource management.
 */
int execute_extract_coords_command(const CommandContext& context);

/**
 * @brief Execute Gaussian input file creation from XYZ files
 * @param context Configured command context
 * @return Exit code: 0 for success, non-zero for errors
 *
 * Creates Gaussian input files (.gau, .com, .in, .gjf) from XYZ coordinate files
 * for various types of calculations including single points, optimizations,
 * transition state searches, and high-level energy calculations.
 */
int execute_create_input_command(const CommandContext& context);

/** @} */  // end of ModuleExecutors group

#endif  // MODULE_EXECUTOR_H