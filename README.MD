# **Gaussian Extractor v0.3**.1

**Gaussian Extractor** is a high-performance, cluster-safe program designed to extract energies from Gaussian computational chemistry outputs. It processes multiple log files in parallel with comprehensive safety features to prevent system overload and ensure stable operation in shared computing environments.

## Key Features

* **Parallel Processing**: Efficiently processes multiple Gaussian output files simultaneously
* **Energy Extraction**: Extracts Gibbs free energies with phase correction (gas phase 1 atm → 1M solution)
* **Automatic Sorting**: Results sorted by energy values for easy analysis
* **Cluster Safety**: Intelligent resource management prevents head node overload
* **Dynamic Memory Management**: Intelligent memory limits based on system RAM and thread count
* **Error Recovery**: Robust error handling with graceful degradation
* **Multi-Platform**: Supports Linux, macOS, and Windows with multiple compilers

## Safety Improvements in v0.3

⚠️ **Critical Safety Features for Cluster Environments:**

* **Resource Limiting**: Automatic detection of cluster environments with conservative CPU usage
* **Smart Memory Allocation**: Dynamic memory limits that scale with system resources and thread count
* **File Handle Management**: Prevents file descriptor exhaustion with pooled access
* **Graceful Shutdown**: Proper signal handling for job management systems
* **Error Isolation**: Thread-safe error handling prevents crashes from corrupted files

## Installation and Compilation

### Prerequisites

**Compiler Requirements:**
* **C++20 Standard**: GCC 10+, Clang 10+, or MSVC 2019+
* **Threading Support**: pthread library (Linux/macOS) or Windows threading
* **Filesystem Support**: std::filesystem (included in modern compilers)

### Compilation Options

### Note that I do not use Windows and MacOS computers or servers to test the program; I am not sure if it is going to work properly on these OSs. Only linux was tested well.

#### Option 1: Automatic Build (Recommended)

**Linux/macOS:**
```bash
make                    # Safe default build
make cluster           # Cluster-optimized build
make debug             # Debug build with safety checks
make release           # High-performance release
```

**Windows:**
```cmd
build.bat              # Automatic compiler detection and build
```

#### Option 2: CMake (Cross-platform)
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

The build will create the executable in the `build` directory. For development, the project supports different build types:

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Enable extra warnings
cmake -DENABLE_EXTRA_WARNINGS=ON ..
```

#### Option 3: Manual Compilation
```bash
# Linux/macOS
g++ -std=c++20 -O2 -Wall -Wextra -pthread -o gaussian_extractor.x src/main.cpp src/core/gaussian_extractor.cpp src/core/job_scheduler.cpp

# Windows (MinGW)
g++ -std=c++20 -O2 -Wall -Wextra -pthread -lpsapi -o gaussian_extractor.x.exe src/main.cpp src/core/gaussian_extractor.cpp src/core/job_scheduler.cpp
```

### Installation
```bash
# System-wide installation (requires sudo)
make install

# User installation
make install-user

# Or add to PATH manually
export PATH=$PATH:/path/to/gaussian-extractor
```

## Usage

### Quick Start

**Basic execution** (processes all .log files in current directory):
```bash
./gaussian_extractor.x
```

**Default behavior:**
* **Temperature**: Read from files (default: 298.15 K if not found)
* **Concentration**: 1 M for phase correction
* **Threads**: Half of available CPU cores (cluster-safe)
* **Output**: Sorted by Gibbs free energy (kJ/mol)
* **Format**: A table of text 

### Safety-First Usage for Clusters head node

**Recommended usage if you run the program on your head node:**

```bash
# Set a limit number of cores which will not over use your head node
./gaussian_extractor.x -nt 4 -q

# You can check resource limits before running
./gaussian_extractor.x --resource-info

# Maximum safety on shared head nodes
./gaussian_extractor.x -nt 2 -q -f csv
```

### You can specify the number of threads

**Thread Control:**
```bash
./gaussian_extractor.x -nt 4        # Use exactly 4 threads
./gaussian_extractor.x -nt half     # Use half cores (default, safest)
./gaussian_extractor.x -nt max      # Use all cores (with safety limits)
```

**⚠️ Cluster Safety Warning:**

- Program automatically detects cluster environments (SLURM, PBS, SGE, LSF)
- In clusters: Maximum threads = min(requested, cores/4, 8)
- On head nodes: **Never use "max"** - it can overload shared resources
- Recommended: `-nt 2` or `-nt 4` for head nodes

**Resource Monitoring:**

```bash
./gaussian_extractor.x --resource-info  # Show system resources before running
```

**Quiet Mode (Recommended for Batch Jobs):**
```bash
./gaussian_extractor.x -q              # Minimal output, perfect for scripts
```



### Output Formats

**Text Format (Default):**
```bash
./gaussian_extractor.x                  # A table of text
```

**CSV Format:**
```bash
./gaussian_extractor.x -f csv          # CSV
./gaussian_extractor.x -f csv -q       # CSV with minimal console output on your terminal 
```

**Output Files:**

- **Text**: `base_directory_name.results`
- **CSV**: `base_directory_name.csv`

### How the program works

The program automatically:
1. **Scans** current directory for .log/.out files
2. **Validates** file sizes (skips files >100MB for safety)
3. **Processes** files in parallel with resource monitoring
4. **Extracts** energies, frequencies, and thermodynamic data
5. **Sorts** results by specified column (default: Gibbs free energy)
6. **Generates** output file with comprehensive metadata
7. **Reports** processing statistics and resource usage
  
    

**Sample Console Output:**
```
Found 150 .log files
Using 4 threads (requested: half)
Processed 75/150 files (50%)
Processed 150/150 files (100%)

Results written to project_directory.results
Total execution time: 12.347 seconds
Memory usage: 256.7 MB / 4.0 GB
```

![results](results.png)

***Output explanation:***

* 1st column (*Output name*): the name of gaussian output files
* 2nd column (*ETG Kj/mol*): the Gibbs free energy with all correction (entropy at corresponding temperature and phase correction from gas phase 1 atm to 1 M)
* 3rd column (*Low FC*): the lowest vibrational frequency. This value is useful when you want to see if a gaussian output has imaginary modes or not (geometrical optimization and transition sate search). *Note that the program just prints out the lowest frequency mode. If your outputs have more than two imaginary modes, the program won't let you know. You have to check this yourself for example TS search.* I will implement this  function soon. 
* 4th column (*ETG a.u*): the same as the 2nd one but the unit is hartree or atomic units.
* 5th column (*Nuclear E  au*): Repulsive nuclear energy. This is useful because nuclear energy is very sensitive to coordinates of molecular systems. Two different coordinate systems will have different nuclear energy.
* 6th column (*SCFE*): SCF energy in hartree
* 7th column (*ZPE*): Zero-point energy
* 8th column (*Status*): current status of a gaussian output: DONE or UNDONE (not finished)
* 9th column (*PCorr*): Whether phase correction (gas phase 1atm to 1M in solution) is corrected or not. This value will be YES if your calcualtions were done in solvent (scrf=solvent), and NO if your calculations were done in gas phase. 
* 10th column (*Round*): numbers of gaussian rounds in a gaussian outputs. If your inputs have 1 --link1--, this value will be 2 indicating 2 sections of run in your outputs.

## Command-Line Options

### Complete Options Reference

| Option | Description | Values | Default |
|--------|-------------|---------|---------|
| `-t, --temp` | Temperature (K) | Positive number | 298.15 |
| `-c, --cm` | Concentration (M) | Positive integer | 1 |
| `-col, --column` | Sort column | 2-10 | 2 (ETG kJ/mol) |
| `-e, --ext` | File extension | log, out | log |
| `-f, --format` | Output format | text, csv | text |
| `-nt, --threads` | Thread count | number, half, max | half |
| `-q, --quiet` | Quiet mode | - | false |
| `--max-file-size` | Max file size (MB) | Positive integer | 100 |
| `--memory-limit` | Memory limit (MB) | Positive integer | Auto-calculated |
| `--resource-info` | Show resources | - | false |
| `-h, --help` | Show help | - | false |

### Important Notes

**Temperature Setting (`-t, --temp`):**
- **Consistency Critical**: When specified, applies to ALL files for consistent phase correction
- **Entropy Warning**: Ensure Gaussian inputs use same temperature for proper entropy corrections
- **File Override**: Without `-t`, program reads temperature from each file individually

**Column Sorting (`-col, --column`):**
- **2**: ETG kJ/mol (Gibbs free energy) - **Default**
- **3**: Lowest frequency (for transition state analysis)
- **4**: ETG a.u (Gibbs free energy in atomic units)
- **5**: Nuclear repulsion energy
- **6**: SCF energy
- **7**: Zero-point energy
- **10**: Round count (number of Gaussian calculation rounds)

### Usage Examples

#### Basic Examples
```bash
# Simple processing with defaults
./gaussian_extractor.x

# Process with specific temperature
./gaussian_extractor.x -t 300

# Sort by SCF energy with 2M concentration
./gaussian_extractor.x -c 2 -col 6

# Generate CSV output quietly
./gaussian_extractor.x -f csv -q
```

#### Production Examples
```bash
# Cluster-safe processing
./gaussian_extractor.x -nt 4 -q -f csv -t 298.15

# High-throughput processing
./gaussian_extractor.x -nt 8 -c 5 -col 4 -f csv

# Conservative head node usage
./gaussian_extractor.x -nt 2 -q --resource-info

# Process larger files (increase 100MB default limit)
./gaussian_extractor.x --max-file-size 500

# Set specific memory limit for high-memory systems
./gaussian_extractor.x --memory-limit 8192 -nt 16
```

#### Advanced Examples
```bash
# Complete parameter specification
./gaussian_extractor.x -t 310.15 -c 5 -col 4 -nt 6 -f csv -q

# Process .out files instead of .log
./gaussian_extractor.x -e out -nt half

# Debug problematic files
./gaussian_extractor.x -nt 1 --resource-info

# Handle large files (increase size limit)
./gaussian_extractor.x --max-file-size 200 -nt 4

# High-performance setup for 32GB+ systems
./gaussian_extractor.x --memory-limit 16384 -nt 20 --max-file-size 500
```



## Safety and Performance Guidelines

### Dynamic Memory Management 

The program now **automatically calculates optimal memory limits** based on:
- **System RAM**: Detects total available memory
- **Thread Count**: More threads = higher memory allocation
- **Environment**: Conservative limits in cluster environments

**Memory Allocation Strategy:**
- **1-4 threads**: 30% of system RAM
- **5-8 threads**: 40% of system RAM  
- **9-16 threads**: 50% of system RAM
- **17+ threads**: 60% of system RAM
- **Cluster environments**: 70% of calculated amount

**Example on 32GB system:**
```bash
./gaussian_extractor.x -nt 4   # Uses ~9.6GB (30% of 32GB)
./gaussian_extractor.x -nt 8   # Uses ~12.8GB (40% of 32GB)  
./gaussian_extractor.x -nt 16  # Uses ~16GB (50% of 32GB)
./gaussian_extractor.x -nt 40  # Uses ~19.2GB (60% of 32GB)
```

### For Cluster Head Nodes 
```bash
# RECOMMENDED: Conservative settings
./gaussian_extractor.x -nt 2 -q -f csv

# CHECK FIRST: Resource availability  
./gaussian_extractor.x --resource-info

# AVOID: Resource-intensive operations
./gaussian_extractor.x -nt max  # DON'T USE ON HEAD NODES
```

### For Compute Nodes
```bash
# Optimal performance settings
./gaussian_extractor.x -nt half    # Usually optimal
./gaussian_extractor.x -nt max     # If node is dedicated to your job

# High-performance setups (adjust based on your system)
./gaussian_extractor.x -nt 20 --memory-limit 16384  # 16GB limit, 20 threads
./gaussian_extractor.x -nt 32 --memory-limit 24576  # 24GB limit, 32 threads
```

### Error Handling and Recovery
- **Automatic**: Continues processing if individual files fail
- **Reporting**: Comprehensive error and warning collection
- **Graceful Shutdown**: Responds properly to Ctrl+C and job termination signals
- **Memory Safety**: Automatically limits memory usage to prevent crashes

## Troubleshooting

### Common Issues

**Build Errors:**
```bash
# Check compiler version
g++ --version  # Need GCC 10+ for C++20

# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential

# Try alternative build
make debug  # Often reveals compilation issues
```

**Performance Issues:**
```bash
# Reduce thread count
./gaussian_extractor.x -nt 2

# Check file sizes
ls -lh *.log  # Files >100MB are automatically skipped

# Monitor resources
./gaussian_extractor.x --resource-info
```

**Memory Issues:**
- Default limit: Auto-calculated based on system RAM and thread count
- Large files (>100MB) automatically skipped by default
- Use `--max-file-size 200` to process larger files
- Use `--memory-limit 8192` to set specific memory limits
- Check system memory with `--resource-info`
- Reduce threads if memory pressure occurs

### Getting Help
```bash
./gaussian_extractor.x --help        # Complete help
./gaussian_extractor.x --resource-info  # System information
```

## Version History

- **v0.3** (Enhanced Safety Edition): Complete resource management, cluster awareness, comprehensive error handling
- **v0.2** (Original): Basic parallel processing
- **v0.1** (Initial): Single-threaded processing

## Contributing and Issues

For bug reports, feature requests, or safety concerns, please create an issue at the project repository. When reporting issues, include:
- System specifications (OS, compiler, cluster environment)
- Command used and error messages
- Sample files (if possible)
- Resource usage information (`--resource-info` output)

---

**⚠️ Important**: This enhanced version prioritizes system stability and cluster safety. Always test in your environment before production use, especially on shared systems.

   
