Usage Guide
===========

This comprehensive guide covers all features and commands of Gaussian Extractor with detailed examples and explanations.

Quick Start
-----------

**Basic Usage:**

.. code-block:: bash

   # Extract data from all .log files in current directory
   gaussian_extractor.x

   # Process with custom settings
   gaussian_extractor.x -t 300 -c 2 -f csv -q

   # Get help
   gaussian_extractor.x --help

Command Overview
----------------

Gaussian Extractor supports the following commands:

+------------------+--------------------------------------------------+
| Command          | Description                                      |
+==================+==================================================+
| ``extract``      | Extract thermodynamic data (default)             |
+------------------+--------------------------------------------------+
| ``done``         | Check and organize completed jobs                |
+------------------+--------------------------------------------------+
| ``errors``       | Check and organize failed jobs                   |
+------------------+--------------------------------------------------+
| ``pcm``          | Check PCM convergence failures                   |
+------------------+--------------------------------------------------+
| ``imode``        | Check jobs with imaginary frequencies            |
+------------------+--------------------------------------------------+
| ``check``        | Run all job checks                               |
+------------------+--------------------------------------------------+
| ``high-kj``      | Calculate high-level energies (kJ/mol)           |
+------------------+--------------------------------------------------+
| ``high-au``      | Calculate high-level energies (atomic units)     |
+------------------+--------------------------------------------------+
| ``xyz``          | Extract coordinates to XYZ format                |
+------------------+--------------------------------------------------+
| ``ci``           | Create Gaussian input files from XYZ             |
+------------------+--------------------------------------------------+
| ``interactive``  | Launch interactive mode (Windows)                |
+------------------+--------------------------------------------------+

Core Commands
=============

1. Extract (Default Command)
-----------------------------

**Purpose:** Extract thermodynamic data from Gaussian log files.

**Basic Usage:**

.. code-block:: bash

   # Process all .log files in current directory
   gaussian_extractor.x

   # Process .out files instead
   gaussian_extractor.x -e out

**Output Formats:**

**Text Format (Default):**

.. code-block:: bash

   gaussian_extractor.x

**CSV Format:**

.. code-block:: bash

   gaussian_extractor.x -f csv

**Sample Output:**

.. code-block::

   Found 150 .log files
   Using 4 threads (requested: half)
   Processed 75/150 files (50%)
   Processed 150/150 files (100%)

   Results written to project_directory.results
   Total execution time: 12.347 seconds
   Memory usage: 256.7 MB / 4.0 GB

**Output Columns Explanation:**

+------------------+--------------------------------------------------+
| Column           | Description                                      |
+==================+==================================================+
| Output name      | Name of the Gaussian output file                 |
+------------------+--------------------------------------------------+
| ETG kJ/mol       | Gibbs free energy with phase correction          |
+------------------+--------------------------------------------------+
| Low FC           | Lowest vibrational frequency                     |
+------------------+--------------------------------------------------+
| ETG a.u          | Gibbs free energy in atomic units                |
+------------------+--------------------------------------------------+
| Nuclear E au     | Nuclear repulsion energy                         |
+------------------+--------------------------------------------------+
| SCFE             | SCF energy in atomic units                       |
+------------------+--------------------------------------------------+
| ZPE              | Zero-point energy                                |
+------------------+--------------------------------------------------+
| Status           | Job status (DONE/UNDONE)                         |
+------------------+--------------------------------------------------+
| PCorr            | Phase correction applied (YES/NO)                |
+------------------+--------------------------------------------------+
| Round            | Number of Gaussian calculation rounds            |
+------------------+--------------------------------------------------+

**Advanced Options:**

.. code-block:: bash

   # Custom temperature and concentration
   gaussian_extractor.x -t 310.15 -c 5

   # Sort by different column (2=ETG kJ/mol, 3=Low FC, 6=SCFE)
   gaussian_extractor.x -col 6

   # Quiet mode (minimal output)
   gaussian_extractor.x -q

   # Use specific number of threads
   gaussian_extractor.x -nt 8

   # Process files larger than default 100MB limit
   gaussian_extractor.x --max-file-size 500

**Temperature and Phase Correction:**

.. code-block:: bash

   # Use temperature from input files (default)
   gaussian_extractor.x

   # Override with custom temperature
   gaussian_extractor.x -t 298.15

   # Custom concentration for phase correction
   gaussian_extractor.x -c 2  # 2M concentration

**Understanding Phase Correction:**

Phase correction converts gas-phase energies to solution-phase:

- **Gas phase (1 atm)** → **Solution (1M standard state)**
- Applied automatically for SCRF calculations
- Critical for accurate free energy comparisons

2. Job Status Commands
-----------------------

**Purpose:** Check and organize Gaussian jobs by completion status.

**Check Completed Jobs:**

.. code-block:: bash

   # Move completed jobs to {dirname}-done/
   gaussian_extractor.x done

   # Custom directory suffix
   gaussian_extractor.x done --dir-suffix completed

**Check Failed Jobs:**

.. code-block:: bash

   # Move failed jobs to errorJobs/
   gaussian_extractor.x errors

   # Show detailed error messages
   gaussian_extractor.x errors --show-details

   # Custom target directory
   gaussian_extractor.x errors --target-dir failed_calculations

**Check PCM Failures:**

.. code-block:: bash

   # Move PCM convergence failures to PCMMkU/
   gaussian_extractor.x pcm

**Check Imaginary Frequencies:**

.. code-block:: bash

   # Move jobs with imaginary frequencies to imaginary_freqs/
   gaussian_extractor.x imode

**Run All Checks:**

.. code-block:: bash

   # Execute all job checks in sequence
   gaussian_extractor.x check

**Workflow Example:**

.. code-block:: bash

   # 1. Run calculations
   # (submit your Gaussian jobs)

   # 2. Check completion status
   gaussian_extractor.x done

   # 3. Check for failures
   gaussian_extractor.x errors

   # 4. Check PCM issues
   gaussian_extractor.x pcm

   # 5. Check vibrational analysis
   gaussian_extractor.x imode

3. High-Level Energy Calculations
----------------------------------

**Purpose:** Calculate energies using high-level electronic energies combined with low-level thermal corrections.

**Directory Structure:**

.. code-block::

   project/
   ├── low_level/          # Opt + Freq calculations
   │   ├── molecule1.log
   │   └── molecule2.log
   └── high_level/         # Single point calculations
       ├── molecule1.log
       └── molecule2.log

**Basic Usage:**

.. code-block:: bash

   # Navigate to high-level directory
   cd high_level

   # Calculate energies in kJ/mol
   gaussian_extractor.x high-kj

   # Calculate detailed energies in atomic units
   gaussian_extractor.x high-au

**Energy Combination Process:**

1. **High-level electronic energy** from current directory
2. **Thermal corrections** from parent directory (../)
3. **Combined result**: E_high + (E_low_thermal - E_low_electronic)

**Output Formats:**

**high-kj (kJ/mol):**

.. code-block::

   Name              G kJ/mol    Status
   molecule1         -1234.56    DONE
   molecule2         -2345.67    DONE

**high-au (Atomic Units - Detailed):**

.. code-block::

   Name    E high    E low     ZPE      TC       TS       H        G
   mol1    -456.78   -450.12   0.123    0.456    0.789    -455.67  -456.46

**Advanced Options:**

.. code-block:: bash

   # Custom temperature
   gaussian_extractor.x high-kj -t 310.15

   # Custom concentration
   gaussian_extractor.x high-kj -c 2

   # Sort by different column
   gaussian_extractor.x high-kj -col 4

   # CSV output
   gaussian_extractor.x high-kj -f csv

4. Coordinate Extraction
-------------------------

**Purpose:** Extract final Cartesian coordinates from Gaussian log files and organize them.

**Basic Usage:**

.. code-block:: bash

   # Extract coordinates from all log files
   gaussian_extractor.x xyz

   # Extract from specific files
   gaussian_extractor.x xyz -f molecule1.log molecule2.log

**Output Organization:**

.. code-block::

   current_directory/
   ├── molecule1.log
   ├── molecule2.log
   └── current_directory_final_coord/
       ├── molecule1.xyz
       └── molecule2.xyz

**Directory Structure:**

- **Completed jobs** → ``{dirname}_final_coord/``
- **Incomplete jobs** → ``{dirname}_running_coord/``

**XYZ File Format:**

.. code-block::

   12
   molecule1.log Final coordinates
   C     0.000000    0.000000    0.000000
   H     1.089000    0.000000    0.000000
   H    -0.363000    1.032000    0.000000
   ...

**Advanced Usage:**

.. code-block:: bash

   # Process .out files
   gaussian_extractor.x xyz -e out

   # Use multiple threads
   gaussian_extractor.x xyz -nt 8

   # Quiet mode
   gaussian_extractor.x xyz -q

5. Create Input Files (ci)
---------------------------

**Purpose:** Generate Gaussian input files from XYZ coordinate files.

**Supported Calculation Types:**

+------------------+--------------------------------------------------+
| Type             | Description                                      |
+==================+==================================================+
| ``sp``           | Single point energy (default)                    |
+------------------+--------------------------------------------------+
| ``opt_freq``     | Geometry optimization + frequency                |
+------------------+--------------------------------------------------+
| ``ts_freq``      | Transition state search + frequency              |
+------------------+--------------------------------------------------+
| ``oss_ts_freq``  | Open-shell singlet TS + frequency                |
+------------------+--------------------------------------------------+
| ``modre_ts_freq``| Modredundant TS + frequency                      |
+------------------+--------------------------------------------------+
| ``oss_check_sp`` | Open-shell singlet stability check               |
+------------------+--------------------------------------------------+
| ``high_sp``      | High-level single point                          |
+------------------+--------------------------------------------------+
| ``irc_forward``  | IRC calculation (forward direction)              |
+------------------+--------------------------------------------------+
| ``irc_reverse``  | IRC calculation (reverse direction)              |
+------------------+--------------------------------------------------+
| ``irc``          | IRC calculation (both directions)                |
+------------------+--------------------------------------------------+

**Basic Examples:**

.. code-block:: bash

   # Single point energy calculation (default)
   gaussian_extractor.x ci

   # Geometry optimization + frequency
   gaussian_extractor.x ci --calc-type opt_freq

   # Transition state search
   gaussian_extractor.x ci --calc-type ts_freq

**Advanced Examples:**

.. code-block:: bash

   # Transition state with frozen bond
   gaussian_extractor.x ci --calc-type modre_ts_freq --freeze-atoms 1 2

   # High-level single point with custom functional
   gaussian_extractor.x ci --calc-type high_sp --functional B3LYP --basis 6-311+G**

   # Solvent calculation
   gaussian_extractor.x ci --calc-type opt_freq --solvent water --solvent-model smd

   # IRC from transition state
   gaussian_extractor.x ci --calc-type irc --tschk-path ../ts_checkpoints

   # Custom settings
   gaussian_extractor.x ci --calc-type opt_freq --charge 1 --mult 2

**Multiple XYZ Files:**

.. code-block:: bash

   # Process multiple files (comma-separated)
   gaussian_extractor.x ci file1.xyz,file2.xyz,file3.xyz

   # Mixed separators (space and comma)
   gaussian_extractor.x ci file1.xyz file2.xyz,file3.xyz

   # With calculation type
   gaussian_extractor.x ci --calc-type opt_freq file1.xyz,file2.xyz

**Template System:**

**Generate Templates:**

.. code-block:: bash

   # Generate template for specific calculation type
   gaussian_extractor.x ci --genci-params opt_freq

   # Generate all available templates
   gaussian_extractor.x ci --genci-all-params

   # Generate in specific directory
   gaussian_extractor.x ci --genci-params opt_freq ./my_templates

**Use Templates:**

.. code-block:: bash

   # Use specific parameter file
   gaussian_extractor.x ci --param-file opt_freq.params

   # Use default parameter file
   gaussian_extractor.x ci --param-file

**Template Workflow:**

1. **Generate Template:**

   .. code-block:: bash

      gaussian_extractor.x ci --genci-params opt_freq

2. **Edit Template (opt_freq.params):**

   .. code-block::

      calc_type = opt_freq
      functional = B3LYP
      basis = 6-31G*
      solvent = chloroform
      charge = 1
      mult = 2
      extra_keywords = Int=UltraFine

3. **Use Template:**

   .. code-block:: bash

      gaussian_extractor.x ci --param-file opt_freq.params

**Generated Input File Example:**

.. code-block::

   %chk=molecule1.chk
   # B3LYP/6-31G* Opt Freq

   molecule1 B3LYP/6-31G* Opt Freq

   1 2
   C     0.000000    0.000000    0.000000
   H     1.089000    0.000000    0.000000
   ...

Configuration and Customization
===============================

Configuration File
-------------------

**Create Default Configuration:**

.. code-block:: bash

   gaussian_extractor.x --create-config

**Configuration File Location:**

- **Linux/macOS:** ``~/.gaussian_extractor.conf``
- **Windows:** ``%USERPROFILE%\.gaussian_extractor.conf``

**Sample Configuration:**

.. code-block::

   # Default temperature for calculations
   default_temperature = 298.15

   # Default concentration for phase correction
   default_concentration = 1.0

   # Default output format
   output_format = text

   # Default thread count (half, max, or number)
   default_threads = half

   # File extensions to process
   output_extensions = .log,.out
   input_extensions = .com,.gjf,.gau

**Configuration Options:**

.. table:: Configuration Options

  +-------------------+--------------------------------------------------+
  | Option            | Description                                      |
  +===================+==================================================+
  | default_temperature | Default temperature (K)                        |
  +-------------------+--------------------------------------------------+
  | default_concentration | Default concentration (M)                    |
  +-------------------+--------------------------------------------------+
  | output_format     | Default output format (text/csv)                 |
  +-------------------+--------------------------------------------------+
  | default_threads   | Default thread count (half/max/number)           |
  +-------------------+--------------------------------------------------+
  | output_extensions | File extensions to process                       |
  +-------------------+--------------------------------------------------+
  | input_extensions  | Input file extensions                            |
  +-------------------+--------------------------------------------------+
  | max_file_size     | Maximum file size (MB)                           |
  +-------------------+--------------------------------------------------+
  | memory_limit      | Memory usage limit (MB)                          |
  +-------------------+--------------------------------------------------+

Performance and Resource Management
====================================

Thread Management
-----------------

**Automatic Thread Detection:**

.. code-block:: bash

   # Use half of available cores (recommended)
   gaussian_extractor.x -nt half

   # Use all available cores
   gaussian_extractor.x -nt max

   # Use specific number
   gaussian_extractor.x -nt 8

**Cluster Safety:**

.. code-block:: bash

   # Conservative settings for head nodes
   gaussian_extractor.x -nt 2 -q

   # Optimal for compute nodes
   gaussian_extractor.x -nt half

Memory Management
-----------------

**Automatic Memory Limits:**

.. code-block:: bash

   # Check current resource usage
   gaussian_extractor.x --resource-info

   # Set custom memory limit
   gaussian_extractor.x --memory-limit 8192

**Memory Allocation Strategy:**

- **1-4 threads:** 30% of system RAM
- **5-8 threads:** 40% of system RAM
- **9-16 threads:** 50% of system RAM
- **17+ threads:** 60% of system RAM

File Size Handling
------------------

**Large File Processing:**

.. code-block:: bash

   # Increase file size limit (default: 100MB)
   gaussian_extractor.x --max-file-size 500

   # Process very large files
   gaussian_extractor.x --max-file-size 1000

Batch Processing
----------------

**Large Directory Handling:**

.. code-block:: bash

   # Enable batch processing
   gaussian_extractor.x --batch-size 50

   # Auto batch size (default)
   gaussian_extractor.x --batch-size 0

Safety Features
===============

Cluster Environment Detection
------------------------------

Gaussian Extractor automatically detects cluster environments:

- **SLURM:** ``sbatch``, ``srun`` detection
- **PBS/Torque:** ``qsub``, ``qstat`` detection
- **SGE:** ``qsub`` detection
- **LSF:** ``bsub``, ``bjobs`` detection

**Cluster-Specific Behavior:**

- Conservative thread limits on head nodes
- Automatic resource detection
- Safe memory allocation

Graceful Shutdown
-----------------

**Signal Handling:**

.. code-block:: bash

   # Program responds to SIGINT (Ctrl+C) and SIGTERM
   # Press Ctrl+C to gracefully stop processing

**Shutdown Process:**

1. Signal received
2. Current file processing completes
3. Results written to disk
4. Clean exit with proper resource cleanup

Error Handling
--------------

**File Processing Errors:**

- Corrupted log files are skipped with warnings
- Large files (>100MB) automatically skipped by default
- Memory limits prevent system overload
- Thread-safe error reporting

**Common Error Scenarios:**

.. code-block:: bash

   # Handle large files
   gaussian_extractor.x --max-file-size 500

   # Reduce memory usage
   gaussian_extractor.x --memory-limit 4096 -nt 2

   # Check system resources
   gaussian_extractor.x --resource-info

Advanced Workflows
==================

Complete Computational Chemistry Workflow
------------------------------------------

**Step 1: Generate Input Files**

.. code-block:: bash

   # Create optimization inputs
   gaussian_extractor.x ci --calc-type opt_freq

   # Submit jobs to queue
   # (use your cluster's job submission system)

**Step 2: Check Job Status**

.. code-block:: bash

   # Check completed jobs
   gaussian_extractor.x done

   # Check for failures
   gaussian_extractor.x errors

   # Check vibrational analysis
   gaussian_extractor.x imode

**Step 3: Extract Results**

.. code-block:: bash

   # Extract thermodynamic data
   gaussian_extractor.x -t 298.15 -c 1

   # Extract coordinates for next step
   gaussian_extractor.x xyz

**Step 4: High-Level Calculations**

.. code-block:: bash

   # Navigate to high-level directory
   cd high_level

   # Calculate refined energies
   gaussian_extractor.x high-kj

High-Throughput Processing
---------------------------

**Batch Processing Setup:**

.. code-block:: bash

   # Process large datasets
   gaussian_extractor.x -nt 16 --max-file-size 500 --memory-limit 16384

   # Quiet mode for scripts
   gaussian_extractor.x -q -f csv

   # Resource monitoring
   gaussian_extractor.x --resource-info

**Script Integration:**

.. code-block:: bash

   #!/bin/bash
   # Process multiple directories

   for dir in dataset1 dataset2 dataset3; do
       cd $dir
       gaussian_extractor.x -q -f csv
       cd ..
   done

Template-Based Automation
-------------------------

**Create Reusable Templates:**

.. code-block:: bash

   # Generate comprehensive template library
   gaussian_extractor.x ci --generate-all-templates ./templates

   # Customize templates for different methods
   # Edit template files with your preferred settings

**Automated Processing:**

.. code-block:: bash

   # Process different molecule types
   gaussian_extractor.x ci --param-file ./templates/opt_freq.params molecule1.xyz
   gaussian_extractor.x ci --param-file ./templates/ts_freq.params molecule2.xyz
   gaussian_extractor.x ci --param-file ./templates/high_sp.params molecule3.xyz

Troubleshooting
===============

Common Issues and Solutions
---------------------------

**Memory Issues:**

.. code-block:: bash

   # Reduce thread count
   gaussian_extractor.x -nt 2

   # Set memory limit
   gaussian_extractor.x --memory-limit 4096

   # Check system resources
   gaussian_extractor.x --resource-info

**File Processing Issues:**

.. code-block:: bash

   # Handle large files
   gaussian_extractor.x --max-file-size 500

   # Process different file types
   gaussian_extractor.x -e out

   # Check file permissions
   ls -la *.log

**Performance Issues:**

.. code-block:: bash

   # Optimize thread usage
   gaussian_extractor.x -nt half

   # Use batch processing
   gaussian_extractor.x --batch-size 25

   # Monitor progress
   gaussian_extractor.x  # (remove -q for progress display)

**Configuration Issues:**

.. code-block:: bash

   # Reset configuration
   gaussian_extractor.x --create-config

   # Check configuration
   gaussian_extractor.x --show-config

   # Validate setup
   gaussian_extractor.x --resource-info

Best Practices
==============

**For Interactive Use:**

.. code-block:: bash

   # Start with resource check
   gaussian_extractor.x --resource-info

   # Use conservative settings
   gaussian_extractor.x -nt 4 -q

   # Monitor progress
   gaussian_extractor.x

**For Batch Processing:**

.. code-block:: bash

   # Optimize for throughput
   gaussian_extractor.x -nt half --max-file-size 500 -q -f csv

   # Use templates for consistency
   gaussian_extractor.x ci --param-file template.params

**For Cluster Environments:**

.. code-block:: bash

   # Head node safety
   gaussian_extractor.x -nt 2 -q

   # Compute node optimization
   gaussian_extractor.x -nt max --memory-limit 16384

**Data Management:**

- Use descriptive filenames
- Organize by calculation type
- Keep raw log files for reference
- Use CSV format for data analysis
- Backup important results

Command Reference
=================

**Global Options:**

+---------------------+----------------------------------+
| Option              | Description                      |
+=====================+==================================+
| ``-h, --help``      | Show help message                |
+---------------------+----------------------------------+
| ``-v, --version``   | Show version information         |
+---------------------+----------------------------------+
| ``-q, --quiet``     | Quiet mode                       |
+---------------------+----------------------------------+
| ``-nt, --threads``  | Thread count (number/half/max)   |
+---------------------+----------------------------------+
| ``-e, --ext``       | File extension (.log/.out)       |
+---------------------+----------------------------------+
| ``--max-file-size`` | Maximum file size (MB)           |
+---------------------+----------------------------------+
| ``--memory-limit``  | Memory limit (MB)                |
+---------------------+----------------------------------+
| ``--resource-info`` | Show system resource information |
+---------------------+----------------------------------+

**Extract Command Options:**

+---------------------+----------------------------------+
| Option              | Description                      |
+=====================+==================================+
| ``-t, --temp``      | Temperature (K)                  |
+---------------------+----------------------------------+
| ``-c, --cm``        | Concentration (M)                |
+---------------------+----------------------------------+
| ``-col, --column``  | Sort column (2-10)               |
+---------------------+----------------------------------+
| ``-f, --format``    | Output format (text/csv)         |
+---------------------+----------------------------------+
| ``--use-input-temp``| Use temperature from files       |
+---------------------+----------------------------------+

**Job Checker Options:**

+---------------------+----------------------------------+
| Option              | Description                      |
+=====================+==================================+
| ``--dir-suffix``    | Directory suffix for done jobs   |
+---------------------+----------------------------------+
| ``--target-dir``    | Custom target directory          |
+---------------------+----------------------------------+
| ``--show-details``  | Show detailed error messages     |
+---------------------+----------------------------------+

**Create Input Options:**

+---------------------+----------------------------------+
| Option              | Description                      |
+=====================+==================================+
| ``--calc-type``     | Calculation type                 |
+---------------------+----------------------------------+
| ``--functional``    | DFT functional                   |
+---------------------+----------------------------------+
| ``--basis``         | Basis set                        |
+---------------------+----------------------------------+
| ``--solvent``       | Solvent name                     |
+---------------------+----------------------------------+
| ``--charge``        | Molecular charge                 |
+---------------------+----------------------------------+
| ``--mult``          | Multiplicity                     |
+---------------------+----------------------------------+
| ``--freeze-atoms``  | Atoms to freeze for TS           |
+---------------------+----------------------------------+
| ``--genci-params``  | Generate parameter template      |
+---------------------+----------------------------------+
| ``--param-file``    | Use parameter file               |
+---------------------+----------------------------------+


Getting Help
============

**Built-in Help:**

.. code-block:: bash

   # General help
   gaussian_extractor.x --help

   # Command-specific help
   gaussian_extractor.x extract --help
   gaussian_extractor.x ci --help

   # Configuration help
   gaussian_extractor.x --config-help

**Resource Information:**

.. code-block:: bash

   # System resource check
   gaussian_extractor.x --resource-info

   # Configuration status
   gaussian_extractor.x --show-config

**Version Information:**

.. code-block:: bash

   gaussian_extractor.x --version

This guide covers all major features and usage patterns of Gaussian Extractor. For the most up-to-date information, always refer to the built-in help system.