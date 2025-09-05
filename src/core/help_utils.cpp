/**
 * @file help_utils.cpp
 * @brief Implementation of help utility functions
 * @author Le Nhan Pham
 * @date 2025
 *
 * This file contains implementations for help-related utility functions
 * used throughout the Gaussian Extractor application.
 */

#include "help_utils.h"
#include "command_system.h"
#include "config_manager.h"
#include "gaussian_extractor.h"
#include "version.h"
#include <iostream>

namespace HelpUtils
{

    void print_help(const std::string& program_name)
    {
        std::cout << "Gaussian Extractor (Version " << GaussianExtractor::get_version_info() << ")\n\n";
        std::cout << "Usage: " << program_name << " [command] [options]\n\n";
        std::cout << "Commands:\n";
        std::cout << "  extract           Extract thermodynamic data from Gaussian log files (default)\n";
        std::cout << "  done              Check and move completed Gaussian jobs\n";
        std::cout << "  errors            Check and move jobs with errors\n";
        std::cout << "  pcm               Check and move jobs with PCM convergence failures\n";
        std::cout << "  imode             Check and move jobs with imaginary frequencies\n";
        std::cout << "  check             Run all job checks (done, errors, pcm)\n";
        std::cout << "  high-kj           Calculate high-level energies in kJ/mol\n";
        std::cout << "  high-au           Calculate high-level energies in atomic units\n";
        std::cout << "  xyz               Extract final coordinates to XYZ format\n";
        std::cout << "  ci                Create inputs from xyz coordinate files\n";
        std::cout << "\nOptions:\n";
        std::cout << "  -h, --help        Show this help message\n";
        std::cout << "  -v, --version     Show version information\n";
        std::cout << "  --config-help     Show configuration file help\n";
        std::cout << "  --create-config   Create a default configuration file\n";
        std::cout << "  --show-config     Show current configuration settings\n";
        std::cout << "\nRun '" << program_name << " <command> --help' for command-specific help.\n";
        std::cout << "\n";
    }

    void print_command_help(CommandType command, const std::string& program_name)
    {
        std::string cmd_name = CommandParser::get_command_name(command);
        std::cout << "Help for command: " << cmd_name << "\n\n";

        switch (command)
        {
            case CommandType::EXTRACT:
                std::cout << "Description: Extract thermodynamic data from Gaussian log files\n\n";
                std::cout << "This is the default command when no command is specified.\n";
                std::cout << "Extracts electronic energies, thermal corrections, and other\n";
                std::cout << "thermodynamic properties from Gaussian log files.\n\n";
                std::cout << "Additional Options:\n";
                std::cout << "  -t, --temp <K>          Temperature in Kelvin (default: 298.15)\n";
                std::cout << "  -c, --concentration <M> Concentration in M for phase correction (default: 1.0)\n";
                std::cout << "  -f, --format <fmt>      Output format: text|csv (default: text)\n";
                std::cout << "  -col, --column <N>      Sort column 1-7 (default: 2)\n";
                std::cout
                    << "                        1=Name, 2=G kJ/mol, 3=G a.u, 4=G eV, 5=LowFQ, 6=Status, 7=PhCorr\n";
                std::cout << "  --input-temp            Use temperature from input files\n";
                std::cout << "  --show-resources        Show system resource information\n";
                std::cout << "  --memory-limit <MB>     Maximum memory usage in MB (default: auto)\n";
                break;

            case CommandType::CHECK_DONE:
                std::cout << "Description: Check and organize completed Gaussian jobs\n\n";
                std::cout << "This command looks for 'Normal termination' in log files\n";
                std::cout << "and moves completed jobs along with their .gau and .chk files\n";
                std::cout << "to a directory named {current_dir}-done/ by default.\n\n";
                break;

            case CommandType::CHECK_ERRORS:
                std::cout << "Description: Check and organize Gaussian jobs that failed\n\n";
                std::cout << "This command looks for error terminations in log files and\n";
                std::cout << "moves failed jobs along with their .gau and .chk files to errorJobs/\n\n";
                break;

            case CommandType::CHECK_PCM:
                std::cout << "Description: Check and organize Gaussian jobs with PCM convergence failures\n\n";
                std::cout << "This command looks for 'failed in PCMMkU' messages in log files\n";
                std::cout << "and moves failed jobs along with their .gau and .chk files to PCMMkU/\n\n";
                break;

            case CommandType::CHECK_IMAGINARY:
                std::cout << "Description: Check and organize jobs with imaginary frequencies\n\n";
                std::cout << "This command identifies Gaussian jobs with negative vibrational\n";
                std::cout << "frequencies and moves them to a designated directory.\n\n";
                break;

            case CommandType::CHECK_ALL:
                std::cout << "Description: Run all job checks (done, errors, pcm) in sequence\n\n";
                break;

            case CommandType::HIGH_LEVEL_KJ:
                std::cout << "Description: Calculate high-level energies with output in kJ/mol units\n";
                std::cout
                    << "             Uses high-level electronic energy combined with low-level thermal corrections\n\n";
                std::cout << "This command reads high-level electronic energies from current directory\n";
                std::cout << "and thermal corrections from parent directory (../) to compute final\n";
                std::cout << "thermodynamic quantities. Output format focuses on final Gibbs energies.\n\n";
                std::cout << "Additional Options:\n";
                std::cout << "  -t, --temp <K>          Temperature in Kelvin (default: from input or 298.15)\n";
                std::cout << "  -c, --concentration <M> Concentration in M for phase correction (default: 1.0)\n";
                std::cout << "  -f, --format <fmt>      Output format: text|csv (default: text)\n";
                std::cout << "  -col, --column <N>      Sort column 1-7 (default: 2)\n";
                std::cout
                    << "                        1=Name, 2=G kJ/mol, 3=G a.u, 4=G eV, 5=LowFQ, 6=Status, 7=PhCorr\n\n";
                break;

            case CommandType::HIGH_LEVEL_AU:
                std::cout << "Description: Calculate detailed energy components in atomic units\n";
                std::cout
                    << "             Uses high-level electronic energy combined with low-level thermal corrections\n\n";
                std::cout << "This command reads high-level electronic energies from current directory\n";
                std::cout << "and thermal corrections from parent directory (../) to compute detailed\n";
                std::cout << "energy component breakdown including ZPE, TC, TS, H, and G values.\n\n";
                std::cout << "Additional Options:\n";
                std::cout << "  -t, --temp <K>          Temperature in Kelvin (default: from input or 298.15)\n";
                std::cout << "  -c, --concentration <M> Concentration in M for phase correction (default: 1.0)\n";
                std::cout << "  -f, --format <fmt>      Output format: text|csv (default: text)\n";
                std::cout << "  -col, --column <N>      Sort column 1-10 (default: 2)\n";
                std::cout
                    << "                          1=Name, 2=E high, 3=E low, 4=ZPE, 5=TC, 6=TS, 7=H, 8=G, 9=LowFQ, "
                       "10=PhCorr\n\n";
                break;

            case CommandType::EXTRACT_COORDS:
                std::cout << "Description: Extract final Cartesian coordinates from Gaussian log files\n";
                std::cout << "             Saves coordinates in XYZ format and organizes based on job status\n\n";
                std::cout << "This command processes specified or all Gaussian log files in the current directory,\n";
                std::cout << "extracts the last set of coordinates, converts to XYZ format, and moves\n";
                std::cout << "the XYZ files to:\n";
                std::cout << "  - {current_dir}_final_coord/   for completed jobs\n";
                std::cout << "  - {current_dir}_running_coord/ for incomplete/failed jobs\n\n";
                std::cout << "Directories are created only if needed.\n\n";
                std::cout << "Additional Options:\n";
                std::cout
                    << "  -f, --files <file1[,file2,...]> Single file or comma-separated list of files to process\n";
                break;

            case CommandType::CREATE_INPUT:
                std::cout << "Description: Create Gaussian input files from XYZ coordinate files\n\n";
                std::cout << "This command processes XYZ files in the current directory and generates\n";
                std::cout << "corresponding Gaussian input files (.gau) with proper formatting,\n";
                std::cout << "route sections, and molecular specifications for various types of\n";
                std::cout << "quantum chemical calculations.\n\n";

                std::cout << "Supported Calculation Types:\n";
                std::cout << "  sp                    Single point energy calculation\n";
                std::cout << "  opt_freq              Geometry optimization + frequency analysis\n";
                std::cout << "  ts_freq               Transition state search + frequency analysis\n";
                std::cout << "  oss_ts_freq           Openshell singlet TS search + frequency analysis\n";
                std::cout << "  modre_ts_freq         Modredundant TS search + frequency analysis\n";
                std::cout << "  oss_check_sp          Openshell singlet stability check\n";
                std::cout << "  high_sp               High-level single point with larger basis set\n";
                std::cout << "  irc_forward           IRC calculation in forward direction\n";
                std::cout << "  irc_reverse           IRC calculation in reverse direction\n";
                std::cout << "  irc                   IRC calculation in both directions\n\n";

                std::cout << "Calculation Type Details:\n";
                std::cout << "  sp: Basic single point energy calculation on provided coordinates\n";
                std::cout << "  opt_freq: Full geometry optimization followed by vibrational frequency analysis\n";
                std::cout << "  ts_freq: Transition state optimization with frequency analysis (requires TS guess)\n";
                std::cout << "  oss_ts_freq: Openshell singlet TS search with stability check and frequency analysis\n";
                std::cout << "  modre_ts_freq: TS search using modredundant to freeze bond, then optimization\n";
                std::cout << "  oss_check_sp: Check openshell singlet stability of optimized geometry\n";
                std::cout << "  high_sp: High-level single point using larger basis set (requires TS checkpoint)\n";
                std::cout << "  irc_forward: Intrinsic reaction coordinate from TS to reactants\n";
                std::cout << "  irc_reverse: Intrinsic reaction coordinate from TS to products\n";
                std::cout << "  irc: Complete IRC calculation in both directions (creates two files)\n\n";

                std::cout << "Additional Options:\n";
                std::cout << "  --calc-type <type>       Calculation type (see list above, default: sp)\n";
                std::cout << "  --functional <func>      DFT functional (default: UWB97XD)\n";
                std::cout << "  --basis <basis>          Basis set (default: Def2SVPP)\n";
                std::cout << "  --large-basis <basis>    Large basis set for TS/high-level calcs\n";
                std::cout << "  --solvent <solvent>      Solvent name for implicit solvation\n";
                std::cout << "  --solvent-model <model>  Solvent model: smd|cpcm|iefpcm (default: smd)\n";
                std::cout << "  --charge <num>           Molecular charge (default: 0)\n";
                std::cout << "  --mult <num>             Multiplicity (default: 1)\n";
                std::cout << "  --freeze-atoms <a1> <a2> Freeze bond between atoms (for TS calculations)\n";
                std::cout << "  --print-level <sign>     Route section modifier: N|P|T (Gaussian versions)\n";
                std::cout << "  --extra-keywords <kw>    Additional Gaussian keywords\n";
                std::cout << "  --extension <ext>        Output file extension (default: .gau)\n";
                std::cout << "  --tschk-path <path>      Path to TS checkpoint files (for high_sp/IRC)\n\n";

                std::cout << "Generation of Gaussian keywords (template parameter file):\n";
                std::cout << "  --genci-params [type] [dir]  Generate parameter template for input creation\n";
                std::cout << "                                    (type defaults to opt_freq, dir defaults to current "
                             "directory)\n";
                std::cout << "  --genci-all-params [dir]     Generate all parameter templates for input creation in "
                             "directory\n";
                std::cout << "                                    (optional dir, defaults to current directory)\n";
                std::cout << "  --param-file <file>          Load parameters from file for input creation\n\n";

                std::cout << "Parameter Files:\n";
                std::cout << "  The program can generate parameter template files that you can edit and reuse.\n";
                std::cout << "  Use --genci-params to create a template (type defaults to general)\n";
                std::cout << "  (optionally specify type and/or directory, defaults to current directory).\n";
                std::cout << "  Use --genci-all-params to create templates for all supported types\n";
                std::cout << "  (optionally specify a directory, defaults to current directory).\n";
                std::cout << "  Then use --param-file to load the parameters from your customized template.\n";
                break;
        }

        std::cout << "Options:\n";
        std::cout << "  -e, --ext <ext>       File extension: log|out (default: log)\n";
        std::cout << "  -nt, --threads <N>    Thread count: number|max|half (default: half)\n";
        std::cout << "  -q, --quiet           Quiet mode (minimal output)\n";
        std::cout << "  --max-file-size <MB>  Maximum file size in MB (default: 100)\n";
        std::cout << "  --batch-size <N>      Batch size for large directories (default: auto)\n";

        if (command == CommandType::CHECK_DONE)
        {
            std::cout << "  --dir-suffix <suffix> Directory suffix (default: done)\n";
            std::cout << "                        Creates {current_dir}-{suffix}/\n";
        }

        if (command == CommandType::CHECK_ERRORS || command == CommandType::CHECK_PCM)
        {
            std::cout << "  --target-dir <name>   Custom target directory name\n";
        }

        if (command == CommandType::CHECK_ERRORS)
        {
            std::cout << "  --show-details        Show actual error messages found\n";
        }

        std::cout << "  -h, --help            Show this help message\n";
        std::cout << "  -v, --version         Show version information\n\n";

        std::cout << "Examples:\n";
        if (command == CommandType::HIGH_LEVEL_KJ)
        {
            std::cout << "  " << program_name << " " << cmd_name << "              # Basic usage\n";
            std::cout << "  " << program_name << " " << cmd_name << " -q           # Quiet mode\n";
            std::cout << "  " << program_name << " " << cmd_name << " -col 5       # Sort by frequency\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " -t 300 -col 2 -f csv  # Custom temp, sort by G kJ/mol, CSV\n";
        }
        else if (command == CommandType::HIGH_LEVEL_AU)
        {
            std::cout << "  " << program_name << " " << cmd_name << "              # Basic usage\n";
            std::cout << "  " << program_name << " " << cmd_name << " -q           # Quiet mode\n";
            std::cout << "  " << program_name << " " << cmd_name << " -col 8       # Sort by Gibbs energy\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " -t 273 -col 4 -f csv  # Custom temp, sort by ZPE, CSV\n";
        }
        else if (command == CommandType::EXTRACT_COORDS)
        {
            std::cout << "  " << program_name << " " << cmd_name << "              # Basic usage\n";
            std::cout << "  " << program_name << " " << cmd_name << " -q           # Quiet mode\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " -f file1.log,file2.log  # Process specific files\n";
        }
        else if (command == CommandType::CREATE_INPUT)
        {
            std::cout << "  " << program_name << " " << cmd_name
                      << "                                       # Basic usage\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " -q                                    # Quiet mode\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " --calc-type opt_freq                  # Optimization + frequency\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " --calc-type modre_ts_freq --freeze-atoms 1 2  # TS search\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " --calc-type high_sp --tschk-path ../ts/       # High-level SP\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " -nt 4                                 # Use 4 threads\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " file1.xyz,file2.xyz,file3.xyz         # Process multiple XYZ files (comma-separated)\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " file1.xyz file2.xyz,file3.xyz         # Mixed space and comma separators\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " --calc-type ts_freq file1.xyz,file2.xyz       # TS calc on multiple files\n";
            std::cout
                << "  " << program_name << " " << cmd_name
                << " --genci-params                        # Generate default template (general) in current dir\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " --genci-params opt_freq               # Generate template for opt_freq in current dir\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " --genci-params ./my_templates         # Generate default template (general) in specific "
                         "directory\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " --genci-params opt_freq ./my_templates        # Generate in specific directory\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " --genci-all-params                    # Generate all templates in current dir\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " --genci-all-params ./templates        # Generate all in specific directory\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " --param-file opt_freq.params          # Use parameters from file\n";
            std::cout << "  " << program_name << " " << cmd_name
                      << " --param-file                          # Use default parameter file\n";
        }
        else
        {
            std::cout << "  " << program_name << " " << cmd_name << "              # Basic usage\n";
            std::cout << "  " << program_name << " " << cmd_name << " -q           # Quiet mode\n";
            std::cout << "  " << program_name << " " << cmd_name << " -nt 4        # Use 4 threads\n";
        }

        if (command == CommandType::CHECK_DONE)
        {
            std::cout << "  " << program_name << " " << cmd_name
                      << " --dir-suffix completed  # Use 'completed' suffix\n";
        }

        std::cout << "\n";
    }

    void print_config_help()
    {
        std::cout << "Gaussian Extractor Configuration System\n\n";
        std::cout << ConfigUtils::get_config_search_paths();
        std::cout << std::endl;

        std::cout << "Commands:\n";
        std::cout << "  --show-config     Show current configuration\n";
        std::cout << "  --create-config   Create default configuration file\n";
        std::cout << "  --config-help     Show this configuration help\n\n";

        std::cout << "Configuration file format:\n";
        std::cout << "  # Lines starting with # are comments\n";
        std::cout << "  key = value\n";
        std::cout << "  # Values can be quoted: key = \"value with spaces\"\n\n";

        std::cout << "Example configuration entries:\n";
        std::cout << "  default_temperature = 298.15\n";
        std::cout << "  default_concentration = 2.0\n";
        std::cout << "  output_extensions = .log,.out\n";
        std::cout << "  input_extensions = .com,.gjf,.gau\n";
        std::cout << "  default_threads = 4\n\n";
    }

    void create_default_config()
    {
        std::cout << "Creating default configuration file...\n";

        if (g_config_manager.create_default_config_file())
        {
            std::string home_dir = g_config_manager.get_user_home_directory();
            if (!home_dir.empty())
            {
                std::cout << "Configuration file created at: " << home_dir << "/.gaussian_extractor.conf" << std::endl;
            }
            else
            {
                std::cout << "Configuration file created at: ./.gaussian_extractor.conf" << std::endl;
            }
            std::cout << "Edit this file to customize your default settings.\n";
        }
        else
        {
            std::cout << "Failed to create configuration file.\n";
            std::cout << "You can create it manually using the template below:\n\n";
            g_config_manager.print_config_file_template();
        }
    }

}  // namespace HelpUtils