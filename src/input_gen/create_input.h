/**
 * @file create_input.h
 * @brief Header file for Gaussian input file creation from XYZ files
 * @author Le Nhan Pham
 * @date 2025
 *
 * This header defines the CreateInput class and related structures for
 * creating Gaussian input files from XYZ coordinate files.
 */

#ifndef CREATE_INPUT_H
#define CREATE_INPUT_H

#include "extraction/gaussian_extractor.h"
#include <memory>
#include <string>
#include <vector>

/**
 * @enum CalculationType
 * @brief Supported calculation types for Gaussian input generation
 */
enum class CalculationType
{
    SP,               ///< Single point energy calculation
    OPT_FREQ,         ///< Geometry optimization + frequency analysis
    TS_FREQ,          ///< Transition state search + frequency analysis
    OSS_TS_FREQ,      ///< Openshell singlet TS search + frequency analysis
    OSS_CHECK_SP,     ///< Openshell singlet check single point
    HIGH_SP,          ///< High-level single point with larger basis set
    IRC_FORWARD,      ///< IRC calculation in forward direction
    IRC_REVERSE,      ///< IRC calculation in reverse direction
    IRC,              ///< IRC calculation in both directions
    MODRE_TS_FREQ,    ///< Modredundant TS search + frequency analysis
    MODRE_OPT,        ///< Modredundant single point energy calculation
    TS_FREQ_FROM_CHK  ///< Transition state search using modre check file + frequency analysis
};

/**
 * @struct CreateSummary
 * @brief Statistical summary of input creation operations
 *
 * Collects statistics about input creation operations including file counts,
 * success rates, errors, and performance metrics.
 */
struct CreateSummary
{
    size_t                   total_files;      ///< Total number of XYZ files considered
    size_t                   processed_files;  ///< Number of files successfully processed
    size_t                   created_files;    ///< Number of input files successfully created
    size_t                   failed_files;     ///< Number of files where creation failed
    size_t                   skipped_files;    ///< Number of files skipped (existing inputs)
    std::vector<std::string> errors;           ///< Collection of error messages encountered
    double                   execution_time;   ///< Total execution time in seconds

    CreateSummary()
        : total_files(0), processed_files(0), created_files(0), failed_files(0), skipped_files(0), execution_time(0.0)
    {}
};

/**
 * @class CreateInput
 * @brief System for creating Gaussian input files from XYZ coordinate files
 *
 * Provides functionality to process XYZ files, read coordinates, and generate
 * Gaussian input files for various types of calculations. Supports parallel
 * processing with resource management integration.
 *
 * @section Key Features
 * - Multiple calculation types (SP, OPT+FREQ, TS, etc.)
 * - Configurable functionals and basis sets
 * - Solvent model support
 * - Multi-threaded processing
 * - Resource-aware operation
 * - Detailed reporting and error handling
 * - Support for complex multi-section inputs
 *
 * @section CreateInput Usage
 * The class processes XYZ files and creates corresponding Gaussian input files
 * with appropriate route sections, molecular specifications, and coordinate data.
 */
class CreateInput
{
public:
    /**
     * @brief Constructor
     * @param ctx Shared pointer to processing context
     * @param quiet Enable quiet mode (suppress non-essential output)
     */
    CreateInput(std::shared_ptr<ProcessingContext> ctx, bool quiet);

    /**
     * @brief Constructor with parameter file
     * @param ctx Shared pointer to processing context
     * @param param_file Path to parameter file
     * @param quiet Enable quiet mode (suppress non-essential output)
     */
    CreateInput(std::shared_ptr<ProcessingContext> ctx, const std::string& param_file, bool quiet);

    /**
     * @brief Load parameters from a parameter file
     * @param param_file Path to parameter file
     * @return true if successful, false otherwise
     */
    bool loadParameters(const std::string& param_file);

    /**
     * @brief Generate checkpoint section for input file
     * @param isomer_name Name of the isomer
     * @return Checkpoint section string
     */
    std::string generate_checkpoint_section(const std::string& isomer_name);

    /**
     * @brief Generate title for current calculation type
     * @return Title string
     */
    std::string generate_title();

    /**
     * @brief Overload Generate title for current calculation type
     * @return Title string
     */
    std::string generate_title(CalculationType calc_type);

    /**
     * @brief Smartly select the appropriate basis set based on calculation type
     * @return The selected basis set string
     */
    std::string select_basis_for_calculation() const;

    /**
     * @brief Check if the given basis string is GEN or GENECP (case insensitive)
     * @param basis_str The basis string to check
     * @return true if it's GEN/GENECP, false otherwise
     */
    bool is_gen_basis(const std::string& basis_str) const;

    /**
     * @brief Validates requirements for calculations with GEN/GENECP basis
     * @throws std::runtime_error if requirements are not met
     */
    void validate_gen_basis_requirements() const;

    ///**
    // * @brief Checks if the given solvent is a generic one, and the check the read keyword
    // * @param solvent_read The generic solvent and read strings
    // * @return true if valid, false otherwise
    // */
    //bool is_solvent_read(const std::string& solvent_read) const;

    /**
     * @brief Validates requirements for MODRE_TS_FREQ and OSS_TS_FREQ calculations
     * @throws
     * std::runtime_error if neither freeze_atoms nor modre is provided
     */
    void validate_modre_requirements() const;

    /**
     * @brief Validates requirements for solvent with Generic and Read keywords
     * @throws
     * std::runtime_error if tail is empty when solvent contains Generic and Read
     */
    void validate_solvent_tail_requirements() const;

    /**
     * @brief Main method to create input files from XYZ files
     * @param xyz_files List of XYZ files to process
     * @return Summary of the creation operation
     */
    CreateSummary create_inputs(const std::vector<std::string>& xyz_files);

    /**
     * @brief Set calculation type
     * @param type The calculation type to use
     */
    void set_calculation_type(CalculationType type);

    /**
     * @brief Set DFT functional
     * @param functional The functional name (e.g., "UWB97XD")
     */
    void set_functional(const std::string& functional);

    /**
     * @brief Set basis set
     * @param basis The basis set name (e.g., "Def2SVPP")
     */
    void set_basis(const std::string& basis);

    /**
     * @brief Set large basis set for high-level calculations
     * @param large_basis The larger basis set name
     */
    void set_large_basis(const std::string& large_basis);

    /**
     * @brief Set solvent parameters
     * @param solvent Solvent name (empty for gas phase)
     * @param model Solvent model (default: "smd")
     */
    void set_solvent(const std::string& solvent, const std::string& model = "smd");

    /**
     * @brief Set pound sign for route section
     * @param print_level The pound sign ("", "P", "T", etc.)
     */
    void set_print_level(const std::string& print_level);

    /**
     * @brief Set extra keywords
     * @param keywords Extra keywords string
     */
    void set_extra_keywords(const std::string& keywords);

    /**
     * @brief Set extra keyword section
     * @param section Extra keyword section string
     */
    void set_extra_keyword_section(const std::string& section);

    /**
     * @brief Set molecular charge and multiplicity
     * @param charge Molecular charge (default: 0)
     * @param mult Multiplicity (default: 1)
     */
    void set_molecular_specs(int charge = 0, int mult = 1);

    /**
     * @brief Set tail text for input file
     * @param tail Additional text at end of input
     */
    void set_tail(const std::string& tail);

    /**
     * @brief Set modredundant text for TS calculations
     * @param modre Modredundant text to replace frozen bond line
     */
    void set_modre(const std::string& modre);

    /**
     * @brief Set file extension for output
     * @param extension File extension (default: ".gau")
     */
    void set_extension(const std::string& extension);

    /**
     * @brief Set TS checkpoint path for high-level calculations
     * @param path Path to TS checkpoint file
     */
    void set_tschk_path(const std::string& path);

    /**
     * @brief Set atoms to freeze for OSS TS calculations
     * @param atom1 First atom index (1-based)
     * @param atom2 Second atom index (1-based)
     */
    void set_freeze_atoms(int atom1, int atom2);

    /**
     * @brief Set SCF maxcycle override
     * @param maxcycle SCF maxcycle value (-1 for default)
     */
    void set_scf_maxcycle(int maxcycle);

    /**
     * @brief Set OPT maxcycles override
     * @param maxcycles OPT maxcycles value (-1 for default)
     */
    void set_opt_maxcycles(int maxcycles);

    /**
     * @brief Set IRC maxpoints override
     * @param maxpoints IRC maxpoints value (-1 for default)
     */
    void set_irc_maxpoints(int maxpoints);

    /**
     * @brief Set IRC recalc override
     * @param recalc IRC recalc value (-1 for default)
     */
    void set_irc_recalc(int recalc);

    /**
     * @brief Set IRC maxcycle override
     * @param maxcycle IRC maxcycle value (-1 for default)
     */
    void set_irc_maxcycle(int maxcycle);

    /**
     * @brief Set IRC stepsize override
     * @param stepsize IRC stepsize value (-1 for default)
     */
    void set_irc_stepsize(int stepsize);

    /**
     * @brief Print summary of creation operation
     * @param summary Summary to print
     * @param operation Description of operation
     */
    void print_summary(const CreateSummary& summary, const std::string& operation);

private:
    std::shared_ptr<ProcessingContext> context;     ///< Processing context
    bool                               quiet_mode;  ///< Quiet mode flag

    // Calculation parameters
    CalculationType     calc_type_;              ///< Type of calculation
    std::string         functional_;             ///< DFT functional
    std::string         basis_;                  ///< Basis set
    std::string         large_basis_;            ///< Large basis set
    std::string         solvent_;                ///< Solvent name
    std::string         solvent_model_;          ///< Solvent model
    std::string         print_level_;            ///< Print level for route
    std::string         extra_keywords_;         ///< Extra keywords
    int                 charge_;                 ///< Molecular charge
    int                 mult_;                   ///< Multiplicity
    std::string         tail_;                   ///< Tail text
    std::string         modre_;                  ///< Modredundant text for TS calculations
    std::string         extra_keyword_section_;  ///< Extra keyword text for additional input sections
    std::string         extension_;              ///< Output file extension
    std::string         tschk_path_;             ///< TS checkpoint path
    std::pair<int, int> freeze_atoms_;           ///< Atoms to freeze (1-based indices)

    // Configurable cycle and optimization parameters (-1 means use default)
    int scf_maxcycle_;   ///< Override for SCF MaxCycle
    int opt_maxcycles_;  ///< Override for OPT MaxCycles
    int irc_maxpoints_;  ///< Override for IRC MaxPoints
    int irc_recalc_;     ///< Override for IRC Recalc
    int irc_maxcycle_;   ///< Override for IRC MaxCycle
    int irc_stepsize_;   ///< Override for IRC StepSize

    /**
     * @brief Create input file from single XYZ file
     * @param xyz_file Path to XYZ file
     * @param error_msg Error message output
     * @return true if successful, false otherwise
     */
    bool create_from_file(const std::string& xyz_file, std::string& error_msg);

    /**
     * @brief Generate Gaussian input content
     * @param isomer_name Name of the isomer (from XYZ filename)
     * @param coordinates Coordinate data as string
     * @return Complete Gaussian input content
     */
    std::string generate_input_content(const std::string& isomer_name, const std::string& coordinates);

    /**
     * @brief Generate route section for different calculation types
     * @param isomer_name Name of the isomer
     * @return Route section string
     */
    std::string generate_route_section(const std::string& isomer_name);

    /**
     * @brief Generate molecule section with coordinates
     * @param coordinates Coordinate data
     * @return Molecule section string
     */
    std::string generate_molecule_section(const std::string& coordinates);

    /**
     * @brief Generate content for a single-section calculation type
     * @param type The calculation type
     * @param isomer_name Name of the isomer
     * @param coordinates Coordinate data
     * @param checkpoint_suffix Suffix for checkpoint file (empty for default) used in IRC calculations for F and R
     * @return Complete section content
     */
    std::string generate_single_section_calc_type(CalculationType    type,
                                                  const std::string& isomer_name,
                                                  const std::string& coordinates,
                                                  const std::string& checkpoint_suffix = "");

    /**
     * @brief Generate route section for a single-section calculation type
     * @param type The calculation type
     * @param isomer_name Name of the isomer
     * @return Route section string
     */
    std::string generate_route_for_single_section_calc_type(CalculationType type, const std::string& isomer_name);

    /**
     * @brief Read coordinates from XYZ file
     * @param xyz_file Path to XYZ file
     * @return Coordinate data as string
     */
    std::string read_xyz_coordinates(const std::string& xyz_file);

    /**
     * @brief Write input file to disk
     * @param input_path Output file path
     * @param content Input file content
     * @return true if successful, false otherwise
     */
    bool write_input_file(const std::string& input_path, const std::string& content);

    /**
     * @brief Generate output filename(s) from XYZ filename
     * @param xyz_file Input XYZ file path
     * @return Vector of output input file paths
     */
    std::vector<std::string> generate_input_filename(const std::string& xyz_file);

    /**
     * @brief Report progress during processing
     * @param current Current file index
     * @param total Total number of files
     */
    void report_progress(size_t current, size_t total);

    /**
     * @brief Log a message (respects quiet mode)
     * @param message Message to log
     */
    void log_message(const std::string& message);

    /**
     * @brief Log an error
     * @param error Error message
     */
    void log_error(const std::string& error);

    /**
     * @brief Parse extra keywords string to handle multiple keywords separated by delimiters
     * @param keywords_str Raw keywords string from parameter file
     * @return Parsed keywords string with single spaces between keywords
     */
    std::string parseExtraKeywords(const std::string& keywords_str);

    /**
     * @brief Parse freeze atoms string into vector of atom indices
     * @param freeze_str String containing atom indices (comma or space separated)
     * @return Vector of parsed atom indices
     */
    std::vector<int> parseFreezeAtomsString(const std::string& freeze_str);
};

#endif  // CREATE_INPUT_H
