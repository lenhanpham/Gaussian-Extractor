API Documentation
=================

This section provides detailed API documentation for Gaussian Extractor, including class descriptions, method signatures, and usage examples extracted from the Doxygen comments in the source code.

Core Classes
============

CommandType Enumeration
-----------------------

.. doxygenenum:: CommandType

The CommandType enumeration defines all available commands that can be executed by the Gaussian Extractor application.

**Enumeration Values:**

- **EXTRACT**: Default command - extract thermodynamic data from log files
- **CHECK_DONE**: Check and organize completed job calculations
- **CHECK_ERRORS**: Check and organize jobs that terminated with errors
- **CHECK_PCM**: Check and organize jobs with PCM convergence failures
- **CHECK_IMAGINARY**: Check and organize jobs with imaginary frequencies
- **CHECK_ALL**: Run comprehensive checks for all job types
- **HIGH_LEVEL_KJ**: Calculate high-level energies with output in kJ/mol units
- **HIGH_LEVEL_AU**: Calculate high-level energies with detailed output in atomic units
- **EXTRACT_COORDS**: Extract coordinates from log files and organize XYZ files
- **CREATE_INPUT**: Create Gaussian input files from XYZ files

CommandContext Structure
------------------------

.. doxygenstruct:: CommandContext
   :members:

The CommandContext structure contains all parameters, options, and state information needed to execute any command in the Gaussian Extractor.

**Key Components:**

**Core Command Identification:**
   - ``command``: The command to execute (CommandType)
   - ``warnings``: Collected warnings from parsing

**Common Parameters:**
   - ``quiet``: Suppress non-essential output
   - ``requested_threads``: Number of threads requested by user
   - ``max_file_size_mb``: Maximum individual file size in MB
   - ``extension``: File extension to process (default: ".log")

**Extract-Specific Parameters:**
   - ``temp``: Temperature for calculations (K)
   - ``concentration``: Concentration for phase corrections (mM)
   - ``sort_column``: Column number for result sorting
   - ``output_format``: Output format ("text", "csv", etc.)
   - ``use_input_temp``: Use temperature from input files

**Job Checker Parameters:**
   - ``target_dir``: Custom directory name for organizing files
   - ``show_error_details``: Display detailed error messages from log files
   - ``dir_suffix``: Custom suffix for completed job directory

**Create Input Parameters:**
   - ``ci_calc_type``: Calculation type (sp, opt_freq, ts, etc.)
   - ``ci_functional``: DFT functional (UWB97XD, B3LYP, etc.)
   - ``ci_basis``: Basis set (def2-SVPP, etc.)
   - ``ci_solvent``: Solvent name
   - ``ci_charge``: Molecular charge
   - ``ci_mult``: Multiplicity

CommandParser Class
-------------------

.. doxygenclass:: CommandParser
   :members:

The CommandParser class provides a complete command-line argument parsing system for the Gaussian Extractor application.

**Key Methods:**

**parse(int argc, char* argv[])**
   Main entry point for command-line parsing. Processes all arguments, applies configuration defaults, validates parameters, and returns a complete CommandContext ready for command execution.

**print_help(const std::string& program_name)**
   Displays comprehensive help including application overview, available commands, common options, configuration file information, and usage examples.

**print_command_help(CommandType command, const std::string& program_name)**
   Displays detailed help for a specific command including command description, command-specific options, usage examples, and related commands.

**print_config_help()**
   Displays information about configuration file locations, available configuration options, configuration file format, and how to create default configuration.

**get_command_name(CommandType command)**
   Converts CommandType enum to string representation.

**create_default_config()**
   Generates a default configuration file with all available options and their descriptions for user customization.

Module Executor Functions
=========================

Command Execution Functions
---------------------------

These functions implement the actual command logic for each supported command type. They receive a fully configured CommandContext and perform the requested operation, returning appropriate exit codes.

**Exit Code Conventions:**

- **0**: Successful execution
- **1**: General error or failure
- **2**: Invalid arguments or configuration
- **3**: Resource unavailable (memory, files, etc.)
- **4**: Operation interrupted by user or system

Extract Command
~~~~~~~~~~~~~~~

.. doxygenfunction:: execute_extract_command(const CommandContext& context)

Executes the extract command to process Gaussian log files and extract thermodynamic data.

**Parameters:**
   - ``context``: CommandContext containing all parameters and options

**Returns:**
   - ``0`` on success
   - Non-zero exit code on failure

**Functionality:**
   - Processes all .log/.out files in current directory
   - Extracts thermodynamic properties (Gibbs free energy, enthalpy, entropy)
   - Applies temperature and phase corrections
   - Supports parallel processing with thread management
   - Outputs results in text or CSV format

Job Checker Commands
~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: execute_check_done_command(const CommandContext& context)

Checks and organizes completed Gaussian job calculations.

**Parameters:**
   - ``context``: CommandContext with job checker parameters

**Functionality:**
   - Scans log files for "Normal termination" messages
   - Moves completed jobs to organized directory structure
   - Preserves .gau, .chk, and .log files together
   - Creates directory with customizable suffix

.. doxygenfunction:: execute_check_errors_command(const CommandContext& context)

Checks and organizes Gaussian jobs that terminated with errors.

**Parameters:**
   - ``context``: CommandContext with error checking parameters

**Functionality:**
   - Identifies various error termination patterns
   - Moves failed jobs to errorJobs/ directory
   - Optionally displays detailed error messages
   - Supports custom target directory names

.. doxygenfunction:: execute_check_pcm_command(const CommandContext& context)

Checks and organizes jobs with PCM convergence failures.

**Parameters:**
   - ``context``: CommandContext with PCM checking parameters

**Functionality:**
   - Scans for "failed in PCMMkU" messages
   - Moves PCM-failed jobs to PCMMkU/ directory
   - Handles solvent model convergence issues

.. doxygenfunction:: execute_check_imaginary_command(const CommandContext& context)

Checks and organizes jobs with imaginary frequencies.

**Parameters:**
   - ``context``: CommandContext with imaginary frequency parameters

**Functionality:**
   - Identifies jobs with negative vibrational frequencies
   - Moves problematic jobs to designated directory
   - Useful for transition state analysis

.. doxygenfunction:: execute_check_all_command(const CommandContext& context)

Runs comprehensive checks for all job types in sequence.

**Parameters:**
   - ``context``: CommandContext for comprehensive checking

**Functionality:**
   - Executes done, errors, pcm, and imaginary checks
   - Provides complete job status overview
   - Organizes all job types appropriately

High-Level Energy Commands
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: execute_high_level_kj_command(const CommandContext& context)

Calculates high-level energies with output in kJ/mol units.

**Parameters:**
   - ``context``: CommandContext with high-level energy parameters

**Functionality:**
   - Combines high-level electronic energies with low-level thermal corrections
   - Outputs final Gibbs energies in kJ/mol
   - Reads energies from current directory
   - Reads thermal corrections from parent directory

.. doxygenfunction:: execute_high_level_au_command(const CommandContext& context)

Calculates high-level energies with detailed output in atomic units.

**Parameters:**
   - ``context``: CommandContext with detailed energy parameters

**Functionality:**
   - Provides comprehensive energy component breakdown
   - Includes ZPE, TC, TS, H, and G values
   - Outputs in atomic units for detailed analysis
   - Supports custom temperature and concentration

Coordinate Processing Commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: execute_extract_coords_command(const CommandContext& context)

Extracts coordinates from log files and organizes XYZ files.

**Parameters:**
   - ``context``: CommandContext with coordinate extraction parameters

**Functionality:**
   - Extracts final Cartesian coordinates from Gaussian log files
   - Converts to standard XYZ format
   - Organizes files by job completion status
   - Supports processing of specific files or all files

.. doxygenfunction:: execute_create_input_command(const CommandContext& context)

Creates Gaussian input files from XYZ coordinate files.

**Parameters:**
   - ``context``: CommandContext with input creation parameters

**Functionality:**
   - Processes XYZ files in current directory
   - Generates Gaussian input files with customizable parameters
   - Supports multiple calculation types (sp, opt_freq, ts_freq, etc.)
   - Includes template system for reusable parameter sets

Configuration Management
========================

ConfigManager Class
-------------------

.. doxygenclass:: ConfigManager
   :members:

The ConfigManager class handles configuration file loading, parsing, and management for the Gaussian Extractor application.

**Key Methods:**

**load_config()**
   Loads configuration from default and user-specified locations.

**get_config_search_paths()**
   Returns list of directories where configuration files are searched.

**create_default_config_file()**
   Creates a default configuration file with all available options.

**print_config_file_template()**
   Displays a template configuration file for user reference.

**get_load_errors()**
   Returns any errors or warnings encountered during configuration loading.

Job Scheduler Integration
=========================

JobResources Structure
----------------------

.. doxygenstruct:: JobResources
   :members:

Contains job scheduler resource information and cluster environment detection.

**Key Members:**

- ``scheduler_type``: Detected job scheduler (SLURM, PBS, SGE, LSF)
- ``is_cluster``: Whether running in cluster environment
- ``max_threads``: Maximum recommended threads for environment
- ``available_memory``: Available system memory
- ``core_count``: Number of available CPU cores

JobScheduler Class
------------------

.. doxygenclass:: JobScheduler
   :members:

Provides integration with various job schedulers and cluster environments.

**Key Methods:**

**detect_scheduler()**
   Automatically detects the job scheduler environment.

**get_resource_limits()**
   Returns appropriate resource limits for the detected environment.

**is_cluster_environment()**
   Determines if running in a cluster environment.

**get_recommended_thread_count()**
   Provides thread count recommendations based on environment.

Utility Classes
===============

HelpUtils Namespace
-------------------

.. doxygennamespace:: HelpUtils

Contains utility functions for generating help text and documentation.

**Key Functions:**

**print_help(const std::string& program_name)**
   Displays general help information for the application.

**print_command_help(CommandType command, const std::string& program_name)**
   Displays detailed help for a specific command.

**print_config_help()**
   Displays configuration system help and file format information.

**create_default_config()**
   Creates a default configuration file for user customization.

Version Information
===================

.. doxygenfunction:: GaussianExtractor::get_version_info()

Returns version information for the Gaussian Extractor application.

**Returns:**
   - String containing version number and build information

Error Handling
==============

The API uses standard C++ exception handling and return codes for error management:

**Exception Types:**
   - ``std::invalid_argument``: Invalid command syntax or parameters
   - ``std::out_of_range``: Parameter values outside valid ranges
   - ``std::runtime_error``: Configuration or resource errors

**Error Return Codes:**
   - **0**: Success
   - **1**: General error
   - **2**: Invalid arguments
   - **3**: Resource unavailable
   - **4**: Operation interrupted

Thread Safety
=============

**Thread-Safe Components:**
   - CommandParser: Static methods, no shared state
   - ConfigManager: Singleton pattern with proper synchronization
   - JobScheduler: Read-only operations after initialization

**Threading Considerations:**
   - Command execution functions are not thread-safe internally
   - Each command should be executed in its own process or carefully synchronized
   - File I/O operations use appropriate locking mechanisms

Memory Management
=================

**Automatic Memory Management:**
   - RAII (Resource Acquisition Is Initialization) pattern used throughout
   - Smart pointers for dynamic memory management
   - Automatic cleanup on exceptions

**Memory Limits:**
   - Configurable memory limits via CommandContext
   - Automatic detection of system memory
   - Graceful degradation under memory pressure

**Resource Cleanup:**
   - Proper file handle management
   - Thread cleanup on termination
   - Signal handler integration for graceful shutdown

Performance Considerations
==========================

**Optimization Features:**
   - Multi-threaded file processing
   - Memory-mapped file I/O for large files
   - Batch processing for large directories
   - Automatic resource detection and optimization

**Scalability:**
   - Linear scaling with thread count (up to system limits)
   - Efficient memory usage patterns
   - Cluster-aware resource allocation

**Benchmarking:**
   - Built-in resource monitoring (--resource-info)
   - Performance timing for all operations
   - Memory usage tracking

Integration Examples
===================

**Basic Command Execution:**

.. code-block:: cpp

   #include "core/command_system.h"
   #include "core/module_executor.h"

   int main(int argc, char* argv[]) {
       try {
           // Parse command line arguments
           CommandContext context = CommandParser::parse(argc, argv);

           // Execute based on command type
           int result = 0;
           switch (context.command) {
               case CommandType::EXTRACT:
                   result = execute_extract_command(context);
                   break;
               case CommandType::CHECK_DONE:
                   result = execute_check_done_command(context);
                   break;
               // ... other commands
           }

           return result;
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << std::endl;
           return 1;
       }
   }

**Configuration Integration:**

.. code-block:: cpp

   #include "core/config_manager.h"

   // Load configuration
   if (!g_config_manager.load_config()) {
       // Handle configuration errors
       auto errors = g_config_manager.get_load_errors();
       for (const auto& error : errors) {
           std::cerr << "Config warning: " << error << std::endl;
       }
   }

**Resource-Aware Execution:**

.. code-block:: cpp

   #include "core/job_scheduler.h"

   // Detect environment and set appropriate limits
   JobResources resources = JobScheduler::detect_resources();
   if (resources.is_cluster) {
       // Use conservative settings for clusters
       context.requested_threads = std::min(context.requested_threads,
                                           resources.max_threads);
   }

This API documentation provides comprehensive information about all classes, functions, and data structures in the Gaussian Extractor codebase. For the most up-to-date information, refer to the source code comments and documentation.