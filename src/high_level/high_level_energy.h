/**
 * @file high_level_energy.h
 * @brief High-level energy calculations with thermal corrections for Gaussian calculations
 * @author Le Nhan Pham
 * @date 2025
 *
 * This header provides a complete system for combining high-level electronic energies
 * with low-level thermal corrections to calculate accurate thermodynamic properties.
 * The system implements a two-level approach where high-level single-point energies
 * are combined with thermal corrections from lower-level optimizations and frequency
 * calculations.
 *
 * @section Methodology
 * The high-level energy approach follows this computational strategy:
 * 1. High-level directory: Single-point energy calculations at higher theory level
 * 2. Parent directory: Geometry optimization and frequency calculations at lower level
 * 3. Combination: High-level electronic energy + low-level thermal corrections
 * 4. Corrections: Phase corrections for concentration and temperature effects
 *
 * @section Energy Components
 * - Electronic energies: SCF, CIS, PCM, CLR corrections
 * - Thermal corrections: Zero-point energy, enthalpy, Gibbs free energy
 * - Phase corrections: Concentration and temperature-dependent corrections
 * - Frequency analysis: Vibrational frequencies for thermal properties
 *
 * @section Output Formats
 * - Gibbs format: Focus on final Gibbs free energies with key corrections
 * - Components format: Detailed breakdown of all energy contributions
 * - Both formats support temperature and concentration variations
 */

#ifndef HIGH_LEVEL_ENERGY_H
#define HIGH_LEVEL_ENERGY_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
// Conditional include for execution policies
#ifdef __has_include
    #if __has_include(<execution>)
        #define HAS_EXECUTION_POLICIES 1
    #else
        #define HAS_EXECUTION_POLICIES 0
    #endif
#else
    // Fallback for older compilers that don't support __has_include
    #if defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 1900
        #include <execution>
        #define HAS_EXECUTION_POLICIES 1
    #elif defined(__GNUC__) && __GNUC__ >= 9
        #include <execution>
        #define HAS_EXECUTION_POLICIES 1
    #elif defined(_MSC_VER) && _MSC_VER >= 1914
        #include <execution>
        #define HAS_EXECUTION_POLICIES 1
    #else
        #define HAS_EXECUTION_POLICIES 0
    #endif
#endif
#include "extraction/gaussian_extractor.h"
#include <chrono>
#include <unordered_map>

/**
 * @defgroup HighLevelConstants Physical Constants for High-Level Calculations
 * @brief Fundamental constants used in high-level energy calculations and conversions
 * @{
 */

/**
 * @brief Universal gas constant in J/(K·mol)
 *
 * Used for phase corrections and thermodynamic calculations.
 * Value: 8.314462618 J/(K·mol) (2018 CODATA value)
 */
const double R_CONSTANT = 8.314462618;

/**
 * @brief Standard atmospheric pressure in N/m²
 *
 * Reference pressure for standard state thermodynamic calculations.
 * Value: 101325.0 N/m² (exactly 1 atm)
 */
const double PO_CONSTANT = 101325.0;

/**
 * @brief Boltzmann constant in Hartree/K
 *
 * Used for temperature-dependent energy calculations in atomic units.
 * Value: 3.166811563×10⁻⁶ Hartree/K
 */
const double KB_CONSTANT = 0.000003166811563;

/**
 * @brief Conversion factor from Hartree to kJ/mol
 *
 * Used to convert atomic unit energies to chemical energy units.
 * Value: 2625.5002 kJ/mol per Hartree
 */
const double HARTREE_TO_KJ_MOL = 2625.5002;

/**
 * @brief Conversion factor from Hartree to eV
 *
 * Used to convert atomic unit energies to electron volts.
 * Value: 27.211396641308 eV per Hartree
 */
const double HARTREE_TO_EV = 27.211396641308;

/**
 * @brief Phase correction factor for concentration effects
 *
 * Pre-calculated factor used in phase correction calculations.
 * Value: 3.808798033989866×10⁻⁴
 */
const double PHASE_CORR_FACTOR = 0.0003808798033989866;

/** @} */  // end of HighLevelConstants group

/**
 * @struct HighLevelEnergyData
 * @brief Complete energy data container for high-level calculations
 *
 * This structure holds all energy components and calculated thermodynamic
 * properties for a single high-level energy calculation. It combines
 * high-level electronic energies with low-level thermal corrections
 * and provides comprehensive thermodynamic analysis.
 *
 * @section Data Organization
 * The structure is organized into logical groups:
 * - High-level electronic energies (from current directory)
 * - Low-level thermal data (from parent directory)
 * - Calculated intermediate values
 * - Final thermodynamic quantities
 * - Additional metadata and status information
 *
 * @section Energy Hierarchy
 * - High-level: Accurate electronic energies (DFT, MP2, CCSD(T), etc.)
 * - Low-level: Geometry optimization and frequency calculations (B3LYP, etc.)
 * - Combined: Best of both levels for accurate thermodynamics
 */
struct HighLevelEnergyData
{
    std::string filename;  ///< Original log file name

    /**
     * @defgroup HighLevelEnergies High-Level Electronic Energies
     * @brief Electronic energies from high-level single-point calculations
     * @{
     */
    double scf_high      = 0.0;  ///< SCF Done energy from high-level calculation
    double scf_td_high   = 0.0;  ///< CIS/TD-DFT energy from high-level calculation
    double scf_equi_high = 0.0;  ///< PCM equilibrium corrected energy
    double scf_clr_high  = 0.0;  ///< CLR (Conductor-like screening) corrected energy
    /** @} */

    /**
     * @defgroup LowLevelThermal Low-Level Thermal Data
     * @brief Thermal corrections from low-level optimization and frequency calculations
     * @{
     */
    double scf_low       = 0.0;  ///< Low-level SCF energy for reference
    double scf_td_low    = 0.0;  ///< Low-level CIS energy for reference
    double zpe           = 0.0;  ///< Zero-point energy correction
    double tc_enthalpy   = 0.0;  ///< Thermal correction to enthalpy
    double tc_gibbs      = 0.0;  ///< Thermal correction to Gibbs free energy
    double tc_energy     = 0.0;  ///< Thermal correction to internal energy
    double entropy_total = 0.0;  ///< Total entropy (translational + rotational + vibrational)
    /** @} */

    /**
     * @defgroup CalculatedValues Calculated Intermediate Values
     * @brief Intermediate values calculated during processing
     * @{
     */
    double tc_only        = 0.0;  ///< Thermal correction excluding zero-point energy
    double ts_value       = 0.0;  ///< Temperature × entropy term
    double final_scf_high = 0.0;  ///< Final high-level electronic energy (best available)
    double final_scf_low  = 0.0;  ///< Final low-level electronic energy (for comparison)
    /** @} */

    /**
     * @defgroup ThermodynamicQuantities Final Thermodynamic Quantities
     * @brief Final calculated thermodynamic properties
     * @{
     */
    double enthalpy_hartree        = 0.0;  ///< Enthalpy: H = E_high + TC_H (Hartree)
    double gibbs_hartree           = 0.0;  ///< Gibbs free energy: G = E_high + TC_G (Hartree)
    double gibbs_hartree_corrected = 0.0;  ///< Gibbs free energy with phase correction (Hartree)
    double gibbs_kj_mol            = 0.0;  ///< Gibbs free energy in kJ/mol
    double gibbs_ev                = 0.0;  ///< Gibbs free energy in eV
    /** @} */

    /**
     * @defgroup AdditionalData Additional Metadata and Status
     * @brief Additional calculation data and status information
     * @{
     */
    double      lowest_frequency   = 0.0;        ///< Lowest vibrational frequency (cm⁻¹)
    double      temperature        = 298.15;     ///< Temperature for calculation (K)
    double      phase_correction   = 0.0;        ///< Applied phase correction term
    bool        has_scrf           = false;      ///< Whether calculation includes SCRF (solvent)
    bool        phase_corr_applied = false;      ///< Whether phase correction was applied
    std::string status             = "UNKNOWN";  ///< Job completion status (DONE, ERROR, UNDONE)
    /** @} */

    /**
     * @brief Default constructor for container operations
     */
    HighLevelEnergyData() = default;

    /**
     * @brief Constructor with filename initialization
     * @param fname Name of the log file for this calculation
     */
    HighLevelEnergyData(const std::string& fname) : filename(fname) {}
};

/**
 * @class HighLevelEnergyCalculator
 * @brief Complete high-level energy calculation and analysis system
 *
 * The HighLevelEnergyCalculator class provides a comprehensive solution for
 * combining high-level electronic energies with low-level thermal corrections
 * to calculate accurate thermodynamic properties. It implements the two-level
 * computational approach commonly used in computational chemistry.
 *
 * @section Calculation Strategy
 * The calculator follows this methodology:
 * 1. Extract high-level electronic energies from current directory
 * 2. Extract thermal corrections from parent directory calculations
 * 3. Combine energies using appropriate theoretical framework
 * 4. Apply temperature and concentration corrections
 * 5. Output results in user-specified formats
 *
 * @section Directory Structure
 * Expected directory organization:
 * ```
 * parent_directory/          # Low-level calculations (opt+freq)
 * ├── compound1.log         # Optimization and frequency calculation
 * ├── compound2.log
 * └── high_level_dir/       # High-level calculations (single points)
 *     ├── compound1.log     # High-level single-point energy
 *     └── compound2.log
 * ```
 *
 * @section Features
 * - Automatic detection of calculation types and methods
 * - Support for various high-level methods (DFT, MP2, CCSD(T))
 * - Temperature and concentration-dependent corrections
 * - Multiple output formats for different analysis needs
 * - Comprehensive error handling and validation
 * - Integration with job scheduler resource management
 */

/**
 * @class ThreadPool
 * @brief High-performance thread pool for parallel file processing
 *
 * A thread-safe, efficient thread pool implementation designed specifically for
 * high-throughput file processing tasks. Features dynamic work distribution,
 * resource-aware task scheduling, and cluster-safe operation.
 *
 * @section Features
 * - Fixed-size thread pool to avoid creation/destruction overhead
 * - Task queue with condition variable synchronization
 * - Exception-safe task execution with error propagation
 * - Graceful shutdown with proper thread joining
 * - Memory-efficient task storage using std::function
 *
 * @section Performance Characteristics
 * - Zero allocation after initialization for task submission
 * - Lock-free task completion detection using atomics
 * - Optimized for I/O-bound tasks (file reading/parsing)
 * - Scales efficiently up to hardware thread limits
 */
class ThreadPool
{
private:
    std::vector<std::thread>          workers_;           ///< Worker thread pool
    std::queue<std::function<void()>> tasks_;             ///< Task queue for pending work
    std::mutex                        queue_mutex_;       ///< Mutex protecting task queue
    std::condition_variable           condition_;         ///< Condition variable for task availability
    std::atomic<bool>                 stop_flag_{false};  ///< Atomic flag for graceful shutdown
    std::atomic<size_t>               active_tasks_{0};   ///< Count of currently executing tasks

public:
    /**
     * @brief Constructor with thread count specification
     * @param num_threads Number of worker threads to create
     *
     * Creates a fixed-size thread pool with the specified number of workers.
     * Threads are created immediately and remain active until destruction.
     *
     * @note Recommended thread count is typically between hardware_concurrency/2
     *       and hardware_concurrency for I/O-bound workloads on clusters
     */
    explicit ThreadPool(size_t num_threads);

    /**
     * @brief Destructor with graceful shutdown
     *
     * Stops accepting new tasks, completes all pending tasks, and joins all
     * worker threads. Ensures all resources are properly cleaned up.
     */
    ~ThreadPool();

    /**
     * @brief Submit a task for asynchronous execution
     * @tparam F Function type (automatically deduced)
     * @tparam Args Argument types (automatically deduced)
     * @param f Function or callable to execute
     * @param args Arguments to pass to the function
     * @return std::future<return_type> Future for task result
     *
     * Submits a task to the thread pool for asynchronous execution. The task
     * will be executed by the next available worker thread. Returns a future
     * that can be used to retrieve the result or wait for completion.
     *
     * @note This method is thread-safe and can be called concurrently
     * @throws std::runtime_error if pool is shutting down
     */
    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            if (stop_flag_.load())
            {
                throw std::runtime_error("ThreadPool is shutting down, cannot enqueue new tasks");
            }

            tasks_.emplace([task]() {
                (*task)();
            });
        }
        condition_.notify_one();

        return result;
    }

    /**
     * @brief Get number of active (executing) tasks
     * @return Current number of tasks being executed
     *
     * Returns the number of tasks currently being executed by worker threads.
     * Useful for monitoring load and determining when processing is complete.
     */
    size_t active_count() const
    {
        return active_tasks_.load();
    }

    /**
     * @brief Wait for all tasks to complete
     *
     * Blocks until all submitted tasks have completed execution. Does not
     * prevent new tasks from being submitted during the wait.
     */
    void wait_for_completion();

    /**
     * @brief Check if thread pool is shutting down
     * @return true if shutdown has been initiated
     */
    bool is_shutting_down() const
    {
        return stop_flag_.load();
    }
};

class HighLevelEnergyCalculator
{
public:
    /**
     * @brief Constructor with temperature and concentration specification
     * @param temp Temperature for thermodynamic calculations (K, default: 298.15)
     * @param concentration_m Concentration in mol/L for phase corrections (default: 1.0)
     *
     * Initializes the calculator with specified thermodynamic conditions.
     * Temperature affects thermal corrections and phase corrections, while
     * concentration affects phase corrections for solution-phase calculations.
     */
    HighLevelEnergyCalculator(double temp            = 298.15,
                              double concentration_m = 1.0,
                              int    sort_column     = 2,
                              bool   is_au_format    = false);

    /**
     * @brief Constructor with processing context for advanced resource management
     * @param context Shared processing context with resource managers
     * @param temp Temperature for thermodynamic calculations (K, default: 298.15)
     * @param concentration_m Concentration in mol/L for phase corrections (default: 1.0)
     *
     * Initializes the calculator with a complete processing context that includes:
     * - Memory monitoring for large-scale processing
     * - File handle management for efficient I/O
     * - Thread-safe error collection
     * - Job scheduler integration
     */
    HighLevelEnergyCalculator(std::shared_ptr<ProcessingContext> context,
                              double                             temp            = 298.15,
                              double                             concentration_m = 1.0,
                              int                                sort_column     = 2,
                              bool                               is_au_format    = false);

    /**
     * @defgroup MainCalculation Main Calculation Functions
     * @brief Primary functions for high-level energy calculations
     * @{
     */

    /**
     * @brief Calculate high-level energy for a single file
     * @param high_level_file Path to high-level calculation log file
     * @return HighLevelEnergyData with complete energy analysis
     *
     * Processes a single high-level calculation file and combines it with
     * corresponding low-level thermal data to produce complete thermodynamic
     * properties. This is the core calculation function.
     *
     * @section Process
     * 1. Extract high-level electronic energies from specified file
     * 2. Locate and extract thermal data from corresponding parent file
     * 3. Combine energies using appropriate theoretical framework
     * 4. Apply temperature and concentration corrections
     * 5. Calculate all thermodynamic quantities
     */
    HighLevelEnergyData calculate_high_level_energy(const std::string& high_level_file);

    /**
     * @brief Process entire directory of high-level calculations
     * @param extension File extension to process (default: ".log")
     * @return Vector of HighLevelEnergyData for all processed files
     *
     * Processes all files in the current directory with the specified extension,
     * calculating high-level energies for each. Provides batch processing
     * capability for large sets of calculations.
     */
    std::vector<HighLevelEnergyData> process_directory(const std::string& extension = ".log");

    /**
     * @brief Process entire directory with parallel processing
     * @param extension File extension to process (default: ".log")
     * @param thread_count Number of threads to use (0 = auto-detect, default: 0)
     * @return Vector of HighLevelEnergyData for all processed files
     *
     * Parallel version of process_directory that uses multiple threads for
     * improved performance on large datasets. Includes:
     * - Thread-safe memory management
     * - File handle resource management
     * - Error collection across threads
     * - Progress tracking and reporting
     */
    std::vector<HighLevelEnergyData> process_directory_parallel(const std::string& extension    = ".log",
                                                                unsigned int       thread_count = 0,
                                                                bool               quiet        = false);

    /**
     * @brief Process files with custom resource limits
     * @param files Vector of specific files to process
     * @param thread_count Number of threads to use
     * @param memory_limit_mb Memory limit in MB (0 = auto-calculate)
     * @return Vector of HighLevelEnergyData for processed files
     *
     * Advanced processing function that allows fine-grained control over
     * resource usage. Useful for large-scale computational workflows.
     */
    std::vector<HighLevelEnergyData> process_files_with_limits(const std::vector<std::string>& files,
                                                               unsigned int                    thread_count,
                                                               size_t                          memory_limit_mb = 0,
                                                               bool                            quiet           = false);

    /** @} */  // end of MainCalculation group

    /**
     * @defgroup OutputFunctions Output Formatting Functions
     * @brief Functions for formatting and displaying calculation results
     * @{
     */

    /**
     * @brief Print results in Gibbs free energy focused format
     * @param results Vector of calculation results to display
     * @param quiet Suppress header and summary information (default: false)
     * @param output_file Optional file stream for output (default: nullptr for console)
     *
     * Outputs results with focus on final Gibbs free energies and key
     * corrections. This format emphasizes the most important thermodynamic
     * quantity for most chemical applications.
     */
    void print_gibbs_format(const std::vector<HighLevelEnergyData>& results,
                            bool                                    quiet       = false,
                            std::ostream*                           output_file = nullptr);

    /**
     * @brief Print results with detailed energy component breakdown
     * @param results Vector of calculation results to display
     * @param quiet Suppress header and summary information (default: false)
     * @param output_file Optional file stream for output (default: nullptr for console)
     *
     * Outputs comprehensive breakdown of all energy components including
     * high-level and low-level contributions, thermal corrections, and
     * intermediate calculated values. Useful for detailed analysis and debugging.
     */
    void print_components_format(const std::vector<HighLevelEnergyData>& results,
                                 bool                                    quiet       = false,
                                 std::ostream*                           output_file = nullptr);

    /**
     * @brief Print results in CSV format optimized for Gibbs energies
     * @param results Vector of calculation results to display
     * @param quiet Suppress header and summary information (default: false)
     * @param output_file Optional file stream for output (default: nullptr for console)
     *
     * Outputs results in comma-separated values format suitable for spreadsheet
     * import and data analysis. Focuses on Gibbs free energies in multiple units.
     */
    void print_gibbs_csv_format(const std::vector<HighLevelEnergyData>& results,
                                bool                                    quiet       = false,
                                std::ostream*                           output_file = nullptr);

    /**
     * @brief Print results in CSV format with detailed energy components
     * @param results Vector of calculation results to display
     * @param quiet Suppress header and summary information (default: false)
     * @param output_file Optional file stream for output (default: nullptr for console)
     *
     * Outputs comprehensive energy breakdown in CSV format including all
     * high-level and low-level contributions, thermal corrections, etc.
     */
    void print_components_csv_format(const std::vector<HighLevelEnergyData>& results,
                                     bool                                    quiet       = false,
                                     std::ostream*                           output_file = nullptr);

    /** @} */  // end of OutputFunctions group

    /**
     * @defgroup Configuration Configuration Management
     * @brief Functions for managing calculator parameters
     * @{
     */

    /**
     * @brief Set temperature for thermodynamic calculations
     * @param temp Temperature in Kelvin
     *
     * Updates the temperature used for thermal corrections and phase
     * corrections. Affects all subsequent calculations.
     */
    void set_temperature(double temp)
    {
        temperature_ = temp;
    }

    /**
     * @brief Set concentration for phase corrections
     * @param conc_m Concentration in mol/L (molarity)
     *
     * Updates the concentration used for phase corrections in solution-phase
     * calculations. Automatically converts to mol/m³ for internal calculations.
     */
    void set_concentration(double conc_m)
    {
        concentration_m_      = conc_m;
        concentration_mol_m3_ = conc_m * 1000.0;
    }

    /**
     * @brief Get current temperature setting
     * @return Temperature in Kelvin
     */
    double get_temperature() const
    {
        return temperature_;
    }

    /**
     * @brief Get current concentration setting
     * @return Concentration in mol/L
     */
    double get_concentration_m() const
    {
        return concentration_m_;
    }

    /**
     * @brief Set sort column for results ordering
     * @param column Column number for sorting (2-10)
     *
     * Sets which column to use for sorting results (visual column numbers):
     *
     * High-KJ format (1-7):
     * - 1: Output name  - 2: G kJ/mol  - 3: G a.u  - 4: G eV  - 5: LowFQ  - 6: Status  - 7: PhCorr
     *
     * High-AU format (1-10):
     * - 1: Output name  - 2: E high a.u  - 3: E low a.u  - 4: ZPE a.u  - 5: TC a.u
     * - 6: TS a.u  - 7: H a.u  - 8: G a.u  - 9: LowFQ  - 10: PhaseCorr
     */
    void set_sort_column(int column)
    {
        if (column >= 1 && column <= 10)
        {
            sort_column_ = column;
        }
    }

    /**
     * @brief Get current sort column setting
     * @return Sort column number (2-10)
     */
    int get_sort_column() const
    {
        return sort_column_;
    }

    /** @} */  // end of Configuration group

private:
    double temperature_;           ///< Temperature for calculations (K)
    double concentration_m_;       ///< Concentration in mol/L
    double concentration_mol_m3_;  ///< Concentration in mol/m³ (for calculations)
    int    sort_column_;           ///< Column for sorting results (visual column numbers)
    bool   is_au_format_;          ///< Whether using AU format (affects column mapping)

    // Enhanced resource management
    std::shared_ptr<ProcessingContext> context_;      ///< Processing context with resource managers
    bool                               has_context_;  ///< Whether enhanced context is available

    // Thread pool for optimized parallel processing
    std::unique_ptr<ThreadPool> thread_pool_;  ///< Reusable thread pool for file processing

    /**
     * @defgroup EnergyExtraction Energy Extraction Helper Functions
     * @brief Private functions for extracting energy values from log files
     * @{
     */

    /**
     * @brief Extract SCF energy from high-level calculation file
     * @param filename Path to high-level log file
     * @return SCF energy in Hartree
     */
    double extract_high_level_scf(const std::string& filename);

    /**
     * @brief Extract specific energy value from log file using pattern matching
     * @param filename Path to log file
     * @param pattern Search pattern to locate energy line
     * @param field_index Field index in matched line (0-based)
     * @param occurrence Which occurrence to use (-1 for last)
     * @return Extracted energy value
     */
    double extract_value_from_file(const std::string& filename,
                                   const std::string& pattern,
                                   int                field_index,
                                   int                occurrence      = -1,
                                   bool               warn_if_missing = true);

    /**
     * @brief Extract thermal correction data from low-level calculation
     * @param parent_file Path to parent directory log file
     * @param data Reference to HighLevelEnergyData to populate
     * @return true if thermal data successfully extracted, false otherwise
     */
    bool extract_low_level_thermal_data(const std::string& parent_file, HighLevelEnergyData& data);

    /** @} */  // end of EnergyExtraction group

    /**
     * @defgroup CalculationHelpers Calculation Helper Functions
     * @brief Private functions for thermodynamic calculations and corrections
     * @{
     */

    /**
     * @brief Calculate phase correction for concentration effects
     * @param temp Temperature in Kelvin
     * @param concentration_mol_m3 Concentration in mol/m³
     * @return Phase correction in Hartree
     */
    double calculate_phase_correction(double temp, double concentration_mol_m3);

    /**
     * @brief Calculate thermal correction excluding zero-point energy
     * @param tc_with_zpe Total thermal correction including ZPE
     * @param zpe Zero-point energy correction
     * @return Thermal correction without ZPE
     */
    double calculate_thermal_only(double tc_with_zpe, double zpe);

    /**
     * @brief Calculate temperature × entropy term
     * @param entropy_total Total entropy
     * @param temp Temperature in Kelvin
     * @return T×S value in Hartree
     */
    double calculate_ts_value(double entropy_total, double temp);

    /**
     * @brief Extract lowest vibrational frequency from parent file
     * @param parent_file Path to frequency calculation log file
     * @return Lowest frequency in cm⁻¹
     */
    double extract_lowest_frequency(const std::string& parent_file);

    /**
     * @brief Determine job completion status from log file
     * @param filename Path to log file to analyze
     * @return Status string ("DONE", "ERROR", "UNDONE")
     */
    std::string determine_job_status(const std::string& filename);

    /** @} */  // end of CalculationHelpers group

    /**
     * @defgroup FileUtilities File System Utility Functions
     * @brief Private functions for file operations and path management
     * @{
     */

    /**
     * @brief Get corresponding parent directory file path
     * @param high_level_file Path to high-level calculation file
     * @return Path to corresponding parent directory file
     */
    std::string get_parent_file(const std::string& high_level_file);

    /**
     * @brief Check if file exists and is accessible
     * @param filename Path to file to check
     * @return true if file exists, false otherwise
     */
    bool file_exists(const std::string& filename);

    /**
     * @brief Read complete file content
     * @param filename Path to file to read
     * @return Complete file content as string
     */
    std::string read_file_content(const std::string& filename);

    /**
     * @brief Read last N lines from file
     * @param filename Path to file to read
     * @param lines Number of lines to read from end (default: 10)
     * @return String containing last N lines
     */
    std::string read_file_tail(const std::string& filename, int lines = 10);

    /** @} */  // end of FileUtilities group

    /**
     * @defgroup OutputFormatting Output Formatting Helper Functions
     * @brief Private functions for formatting output display
     * @{
     */

    /**
     * @brief Print header for Gibbs free energy format output
     * @param output_file Optional file stream for output (default: nullptr for console)
     */
    void print_gibbs_header(std::ostream* output_file = nullptr);

    /**
     * @brief Print header for energy components format output
     * @param output_file Optional file stream for output (default: nullptr for console)
     */
    void print_components_header(std::ostream* output_file = nullptr);

    /**
     * @brief Format filename for display with length limit
     * @param filename Original filename
     * @param max_length Maximum display length (default: 53)
     * @return Formatted filename string
     */
    std::string format_filename(const std::string& filename, int max_length = 53);

    /**
     * @brief Print summary information about calculation
     * @param last_parent_file Path to parent file used for thermal data
     * @param output_file Optional file stream for output (default: nullptr for console)
     */
    void print_summary_info(const std::string& last_parent_file, std::ostream* output_file = nullptr);

    /** @} */  // end of OutputFormatting group

public:
    /**
     * @defgroup DynamicOutputFormatting Dynamic Output Formatting
     * @brief Advanced formatting functions with dynamic column sizing
     * @{
     */

    /**
     * @brief Print results in Gibbs format with dynamic column widths
     * @param results Vector of calculation results to display
     * @param quiet Suppress header and summary information (default: false)
     * @param output_file Optional file stream for output (default: nullptr for console)
     *
     * Uses dynamic column sizing based on actual data content for better readability.
     * This is the recommended formatting method for most use cases.
     */
    void print_gibbs_format_dynamic(const std::vector<HighLevelEnergyData>& results,
                                    bool                                    quiet       = false,
                                    std::ostream*                           output_file = nullptr);

    /**
     * @brief Print results in components format with dynamic column widths
     * @param results Vector of calculation results to display
     * @param quiet Suppress header and summary information (default: false)
     * @param output_file Optional file stream for output (default: nullptr for console)
     *
     * Uses dynamic column sizing based on actual data content for better readability.
     * This is the recommended formatting method for most use cases.
     */
    void print_components_format_dynamic(const std::vector<HighLevelEnergyData>& results,
                                         bool                                    quiet       = false,
                                         std::ostream*                           output_file = nullptr);

    /**
     * @brief Print results in Gibbs format with static column widths
     * @param results Vector of calculation results to display
     * @param quiet Suppress header and summary information (default: false)
     * @param output_file Optional file stream for output (default: nullptr for console)
     *
     * Uses fixed column widths for consistent formatting. Primarily used for
     * backwards compatibility or when consistent column alignment is required.
     */
    void print_gibbs_format_static(const std::vector<HighLevelEnergyData>& results,
                                   bool                                    quiet       = false,
                                   std::ostream*                           output_file = nullptr);

    /**
     * @brief Print results in components format with static column widths
     * @param results Vector of calculation results to display
     * @param quiet Suppress header and summary information (default: false)
     * @param output_file Optional file stream for output (default: nullptr for console)
     *
     * Uses fixed column widths for consistent formatting. Primarily used for
     * backwards compatibility or when consistent column alignment is required.
     */
    void print_components_format_static(const std::vector<HighLevelEnergyData>& results,
                                        bool                                    quiet       = false,
                                        std::ostream*                           output_file = nullptr);

    /** @} */  // end of DynamicOutputFormatting group

private:
    /**
     * @defgroup DynamicFormatting Dynamic Column Formatting
     * @brief Helper functions for calculating optimal column widths
     * @{
     */

    /**
     * @brief Calculate optimal column widths for Gibbs format output
     * @param results Vector of calculation results
     * @return Vector of column widths for each data column
     */
    std::vector<int> calculate_gibbs_column_widths(const std::vector<HighLevelEnergyData>& results);

    /**
     * @brief Calculate optimal column widths for components format output
     * @param results Vector of calculation results
     * @return Vector of column widths for each data column
     */
    std::vector<int> calculate_components_column_widths(const std::vector<HighLevelEnergyData>& results);

    /**
     * @brief Print header for Gibbs format with dynamic column widths
     * @param column_widths Vector of column widths
     * @param output_file Optional file stream for output
     */
    void print_gibbs_header_dynamic(const std::vector<int>& column_widths, std::ostream* output_file = nullptr);

    /**
     * @brief Print header for components format with dynamic column widths
     * @param column_widths Vector of column widths
     * @param output_file Optional file stream for output
     */
    void print_components_header_dynamic(const std::vector<int>& column_widths, std::ostream* output_file = nullptr);

    /** @} */  // end of DynamicFormatting group

    /**
     * @defgroup ParallelProcessing Parallel Processing Functions
     * @brief Private functions for parallel file processing
     * @{
     */

    /**
     * @brief Compare two HighLevelEnergyData entries for sorting
     * @param a First data entry
     * @param b Second data entry
     * @param column Column number to sort by (2-10)
     * @return true if a should come before b in sorted order
     *
     * Compares entries based on visual column numbers:
     *
     * High-KJ format: 2=G kJ/mol, 3=G a.u, 4=G eV, 5=LowFQ, 6=Status, 7=PhCorr
     * High-AU format: 2=E high a.u, 3=E low a.u, 4=ZPE a.u, 5=TC a.u, 6=TS a.u, 7=H a.u, 8=G a.u, 9=LowFQ, 10=PhaseCorr
     */
    bool compare_results(const HighLevelEnergyData& a, const HighLevelEnergyData& b, int column);

    /**
     * @brief Worker function for parallel file processing
     * @param files Vector of files to process
     * @param start_index Starting index for this worker
     * @param end_index Ending index for this worker
     * @param results Shared results vector (thread-safe access required)
     * @param results_mutex Mutex for results vector access
     * @param progress_counter Atomic counter for progress tracking
     *
     * Processes a subset of files assigned to this worker thread.
     * Includes proper resource management and error handling.
     */
    void process_files_worker(const std::vector<std::string>&   files,
                              size_t                            start_index,
                              size_t                            end_index,
                              std::vector<HighLevelEnergyData>& results,
                              std::mutex&                       results_mutex,
                              std::atomic<size_t>&              progress_counter);

    /**
     * @brief Enhanced parallel processing using thread pool
     * @param files List of files to process
     * @param thread_count Number of threads to use
     * @param memory_limit_mb Memory limit in MB
     * @param quiet Suppress progress output
     * @return Vector of processed results
     *
     * Improved parallel processing implementation featuring:
     * - Reusable thread pool to eliminate creation overhead
     * - Asynchronous I/O with batched file operations
     * - Dynamic task distribution for optimal load balancing
     * - Enhanced memory management and progress reporting
     * - Cluster-safe resource utilization
     */
    std::vector<HighLevelEnergyData> process_files_with_thread_pool(const std::vector<std::string>& files,
                                                                    unsigned int                    thread_count,
                                                                    size_t                          memory_limit_mb,
                                                                    bool                            quiet);

    /**
     * @brief Optimized file reading with caching and batching
     * @param filenames List of files to read
     * @return Map of filename to file content
     *
     * Batch file reading with optimizations:
     * - Prefetch file metadata (size, existence)
     * - Read files in optimal order (smallest first)
     * - Use memory-mapped I/O for large files when available
     * - Cache frequently accessed portions
     */
    std::unordered_map<std::string, std::string> batch_read_files(const std::vector<std::string>& filenames);

    /**
     * @brief Process single file with enhanced error handling
     * @param filename File to process
     * @param file_content Pre-read file content (optional)
     * @return Processed energy data
     *
     * Enhanced single file processing with:
     * - Optional pre-read content to avoid duplicate I/O
     * - Comprehensive error recovery
     * - Memory usage tracking
     * - Performance metrics collection
     */
    HighLevelEnergyData process_single_file_enhanced(const std::string& filename, const std::string& file_content = "");

    /**
     * @brief Get or create thread pool with optimal size
     * @param thread_count Desired thread count
     * @return Reference to thread pool
     *
     * Lazy initialization of thread pool with optimal sizing:
     * - Creates pool only when needed
     * - Adjusts thread count based on system resources
     * - Respects cluster environment constraints
     */
    ThreadPool& get_thread_pool(unsigned int thread_count);

    /**
     * @brief Calculate optimal number of threads for processing
     * @param file_count Number of files to process
     * @param available_memory Available memory in MB
     * @return Optimal thread count
     *
     * Determines the best number of threads based on:
     * - Available CPU cores
     * - Memory constraints
     * - File count and estimated processing complexity
     */
    unsigned int calculate_optimal_threads(size_t file_count, size_t available_memory);

    /**
     * @brief Validate and prepare files for processing
     * @param files Vector of files to validate
     * @return Vector of validated, accessible files
     *
     * Pre-processes file list to:
     * - Check file existence and accessibility
     * - Estimate memory requirements
     * - Sort by processing priority
     */
    std::vector<std::string> validate_and_prepare_files(const std::vector<std::string>& files);

    /**
     * @brief Monitor processing progress and resource usage
     * @param total_files Total number of files to process
     * @param progress_counter Atomic counter tracking completed files
     * @param start_time Processing start time
     * @return true to continue monitoring, false to stop
     *
     * Provides real-time monitoring of:
     * - Processing progress
     * - Memory usage
     * - Estimated completion time
     * - Error rates
     */
    bool monitor_progress(size_t                                total_files,
                          const std::atomic<size_t>&            progress_counter,
                          std::chrono::steady_clock::time_point start_time,
                          bool                                  quiet = false);

    /** @} */  // end of ParallelProcessing group

    /**
     * @defgroup SafetyHelpers Safety and Validation Helpers
     * @brief Private functions for enhanced safety checks and validation
     * @{
     */

    /**
     * @brief Safe file reading with memory and handle management
     * @param filename Path to file to read
     * @param max_size_mb Maximum file size to read (MB)
     * @return File content or empty string on error
     *
     * Enhanced file reading that includes:
     * - File size validation
     * - Memory allocation checking
     * - File handle management
     * - Error reporting to context collector
     */
    std::string safe_read_file(const std::string& filename, size_t max_size_mb = 100);

    /**
     * @brief Safe energy value parsing with validation
     * @param value_str String containing energy value
     * @param context_info Context information for error reporting
     * @return Parsed energy value or 0.0 on error
     *
     * Enhanced parsing with:
     * - Range validation
     * - NaN/infinity checking
     * - Error reporting
     * - Context-aware logging
     */
    double safe_parse_energy(const std::string& value_str, const std::string& context_info);

    /**
     * @brief Validate processing context and resources
     * @return true if context is valid and resources available
     *
     * Comprehensive validation of:
     * - Memory availability
     * - File handle availability
     * - Processing context integrity
     * - System resource status
     */
    bool validate_processing_context();

    /** @} */  // end of SafetyHelpers group
};

/**
 * @namespace HighLevelEnergyUtils
 * @brief Utility functions supporting high-level energy calculations
 *
 * The HighLevelEnergyUtils namespace provides a comprehensive collection
 * of utility functions that support high-level energy calculations including
 * file operations, energy parsing, unit conversions, and data validation.
 */
namespace HighLevelEnergyUtils
{
    /**
     * @defgroup FileDirectoryUtils File and Directory Utilities
     * @brief Utilities for file system operations in high-level calculations
     * @{
     */

    /**
     * @brief Find all log files in specified directory
     * @param directory Target directory to search (default: current directory)
     * @return Vector of log file paths found in directory
     *
     * Searches for Gaussian log files suitable for high-level energy processing.
     * Filters files by extension and validates accessibility.
     */
    // This function is replaced by findLogFiles one in gaussian_extractor.h
    // std::vector<std::string> find_log_files(const std::string& directory = ".");

    /**
     * @brief Get name of current working directory
     * @return Current directory name (not full path)
     *
     * Extracts just the directory name for use in output formatting
     * and directory structure validation.
     */
    std::string get_current_directory_name();

    /**
     * @brief Validate that current directory is suitable for high-level calculations
     * @return true if directory structure is valid, false otherwise
     *
     * Checks that the current directory contains high-level calculation files
     * and that a corresponding parent directory with thermal data exists.
     */
    bool is_valid_high_level_directory();

    /**
     * @brief Validate directory using a specific extension and file size limit
     * @param extension Output file extension to look for (e.g., ".log", ".out")
     * @param max_file_size_mb Maximum file size to include (MB)
     * @return true if current directory has matching files and a parent directory exists
     */
    bool is_valid_high_level_directory(const std::string& extension, size_t max_file_size_mb);

    /** @} */  // end of FileDirectoryUtils group

    /**
     * @defgroup EnergyParsingUtils Energy Parsing Utilities
     * @brief Utilities for parsing energy values from Gaussian log files
     * @{
     */

    /**
     * @brief Parse energy value from a text line
     * @param line Text line containing energy information
     * @param field_index Index of field containing energy (0-based)
     * @return Parsed energy value
     *
     * Safely parses energy values from formatted text lines with
     * error handling for malformed input.
     */
    double parse_energy_value(const std::string& line, int field_index);

    /**
     * @brief Extract all vibrational frequencies from log file content
     * @param content Complete log file content
     * @return Vector of frequencies in cm⁻¹
     *
     * Parses frequency calculation output to extract all vibrational
     * frequencies for thermal property calculations.
     */
    std::vector<double> extract_frequencies(const std::string& content);

    /**
     * @brief Find lowest frequency from frequency list
     * @param frequencies Vector of frequencies to analyze
     * @return Lowest frequency value
     *
     * Identifies the lowest vibrational frequency, which is important
     * for identifying transition states and unstable structures.
     */
    double find_lowest_frequency(const std::vector<double>& frequencies);

    /** @} */  // end of EnergyParsingUtils group

    /**
     * @defgroup ConversionUtils Unit Conversion Utilities
     * @brief Utilities for converting between different energy units
     * @{
     */

    /**
     * @brief Convert energy from Hartree to kJ/mol
     * @param hartree Energy value in Hartree (atomic units)
     * @return Energy value in kJ/mol
     */
    double hartree_to_kj_mol(double hartree);

    /**
     * @brief Convert energy from Hartree to eV
     * @param hartree Energy value in Hartree (atomic units)
     * @return Energy value in electron volts
     */
    double hartree_to_ev(double hartree);

    /** @} */  // end of ConversionUtils group

    /**
     * @defgroup ValidationUtils Validation Utilities
     * @brief Utilities for validating calculation parameters and results
     * @{
     */

    /**
     * @brief Validate temperature value for physical reasonableness
     * @param temp Temperature value in Kelvin
     * @return true if temperature is physically reasonable, false otherwise
     *
     * Checks that temperature is within reasonable bounds for chemical
     * calculations (typically 0-2000 K range).
     */
    bool validate_temperature(double temp);

    /**
     * @brief Validate concentration value for chemical reasonableness
     * @param conc Concentration value in mol/L
     * @return true if concentration is chemically reasonable, false otherwise
     *
     * Checks that concentration is within reasonable bounds for solution
     * chemistry calculations (typically 10⁻⁶ to 10² M range).
     */
    bool validate_concentration(double conc);

    /**
     * @brief Validate complete energy data structure for consistency
     * @param data HighLevelEnergyData structure to validate
     * @return true if energy data is consistent and reasonable, false otherwise
     *
     * Performs comprehensive validation of energy data including:
     * - Energy value reasonableness
     * - Internal consistency between different energy components
     * - Proper completion status
     * - Frequency data validity
     */
    bool validate_energy_data(const HighLevelEnergyData& data);

    /** @} */  // end of ValidationUtils group
}  // namespace HighLevelEnergyUtils

#endif  // HIGH_LEVEL_ENERGY_H
