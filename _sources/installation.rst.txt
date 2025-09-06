Installation Guide
==================

This guide covers the installation of Gaussian Extractor on various platforms including Linux, Windows, and cluster environments.

System Requirements
-------------------

**Minimum Requirements:**

- **Operating System**: Linux, Windows 10+, or macOS (not tested)
- **Compiler**: C++20 compatible compiler
- **Memory**: 2GB RAM minimum, 8GB recommended
- **Storage**: 100MB for installation

**Recommended Requirements:**

- **Operating System**: Linux (Ubuntu 20.04+, CentOS 7+, RHEL 8+)
- **Compiler**: GCC 10+, Intel oneAPI, or Clang 10+
- **Memory**: 16GB+ RAM for large datasets
- **Storage**: 1GB+ for processing large log files

Supported Compilers
-------------------

Gaussian Extractor supports multiple C++20 compilers:

- **GCC**: 10.0+ (recommended for Linux)
- **Intel oneAPI**: icpx/icpc (recommended for clusters)
- **Intel Classic**: icpc (legacy support)
- **Clang**: 10.0+ (alternative Linux compiler)
- **MSVC**: 2019+ (Windows)

.. note::
   Intel compilers provide the best performance on Intel-based cluster systems.

Installation Methods
====================

**Windows Users:**

Pre-compiled Binary (Easiest)
-----------------------------------------

1. Download the latest release from GitHub
2. Extract the ZIP file to your desired location
3. Add the ``bin`` directory to your system PATH
4. Test installation:

   .. code-block:: batch

      gaussian_extractor.x --version

      double-click gaussian_extractor.x to run interactive mode

**Linux/macOS Users:**

Build from Source)
----------------------------------------------------------

Prerequisites
~~~~~~~~~~~~~

**Ubuntu/Debian:**

.. code-block:: bash

   sudo apt update
   sudo apt install build-essential cmake git
   # Optional: Install additional compilers
   sudo apt install gcc-10 g++-10 clang-10

**CentOS/RHEL:**

.. code-block:: bash

   sudo yum groupinstall "Development Tools"
   sudo yum install cmake git
   # For newer GCC versions, consider SCL:
   sudo yum install centos-release-scl
   sudo yum install devtoolset-10-gcc devtoolset-10-gcc-c++

**macOS (Limited Support):**

.. code-block:: bash

   # Install Xcode command line tools
   xcode-select --install
   # Install Homebrew
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   brew install cmake gcc

Automatic Build (Recommended)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Basic Build:**

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/lenhanpham/gaussian-extractor.git
   cd gaussian-extractor

   # Build with auto-detected compiler
   make -j $(nproc)

   # The binary will be created as gaussian_extractor.x

**Build Variants:**

.. code-block:: bash

   # Cluster-optimized build
   make cluster -j $(nproc)

   # Debug build with additional safety checks
   make debug -j $(nproc)

   # High-performance release build
   make release -j $(nproc)

**Force Specific Compiler:**

.. code-block:: bash

   # Intel oneAPI compiler (recommended for clusters)
   CXX=icpx make -j $(nproc)

   # Intel Classic compiler
   CXX=icpc make -j $(nproc)

   # GNU compiler
   CXX=g++ make -j $(nproc)

CMake Build (Cross-platform)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Standard CMake Build:**

.. code-block:: bash

   # Create build directory
   mkdir build && cd build

   # Configure with auto-detected compiler
   cmake ..

   # Build
   cmake --build . -j $(nproc)

   # Optional: Install system-wide
   sudo make install

**Advanced CMake Options:**

.. code-block:: bash

   # Specify compiler explicitly
   CXX=icpx cmake -DCMAKE_BUILD_TYPE=Release ..

   # Enable additional debugging
   cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG=ON ..

   # Custom installation directory
   cmake -DCMAKE_INSTALL_PREFIX=/opt/gaussian_extractor ..

Cluster Installation
-------------------------------

**Load Required Modules:**

.. code-block:: bash

   # Load Intel compilers and TBB
   module load intel-compiler-llvm
   module load intel-tbb

   # Alternative: Load GCC
   module load gcc

**Build for Cluster:**

.. code-block:: bash

   # Use cluster-optimized build
   make cluster -j 8

   # Or with CMake
   mkdir build && cd build
   CXX=icpx cmake ..
   make -j 8

**Intel TBB Library Setup:**

If using Intel compilers, ensure TBB library is available:

.. code-block:: bash

   # Check if TBB is loaded
   echo $TBBROOT

   # If not loaded, add to LD_LIBRARY_PATH
   export LD_LIBRARY_PATH=$TBBROOT/lib:$LD_LIBRARY_PATH

   # Make permanent in your shell profile
   echo 'export LD_LIBRARY_PATH=$TBBROOT/lib:$LD_LIBRARY_PATH' >> ~/.bashrc

Post-Installation Setup
=======================

Configuration File
------------------

Create a default configuration file:

.. code-block:: bash

   # Generate default configuration
   gaussian_extractor.x --create-config

   # This creates ~/.gaussian_extractor.conf
   # Edit this file to set your preferred defaults

Environment Setup
-----------------

**Linux/macOS:**

.. code-block:: bash

   # Add to PATH (add to ~/.bashrc or ~/.zshrc)
   export PATH=$PATH:/path/to/gaussian_extractor

   # Optional: Create alias
   alias gx='gaussian_extractor.x'

**Windows:**

1. Open System Properties → Advanced → Environment Variables
2. Add the Gaussian Extractor directory to PATH
3. Open new Command Prompt and test:

   .. code-block:: batch

      gaussian_extractor.x --version

Verification
------------

Test your installation:

.. code-block:: bash

   # Check version
   gaussian_extractor.x --version

   # Show help
   gaussian_extractor.x --help

   # Show system resource information
   gaussian_extractor.x --resource-info

   # Test with sample data (if available)
   gaussian_extractor.x

Troubleshooting
===============

Common Build Issues
-------------------

**Compiler Not Found:**

.. code-block:: bash

   # Check available compilers
   which g++ icpx icpc clang++

   # Install missing compiler
   sudo apt install g++-10  # Ubuntu/Debian
   sudo yum install gcc-c++ # CentOS/RHEL

**C++20 Support Missing:**

.. code-block:: bash

   # Check compiler version
   g++ --version

   # Upgrade compiler if needed
   sudo apt install g++-10
   sudo update-alternatives --config g++

**Library Issues:**

.. code-block:: bash

   # Check for required libraries
   ldconfig -p | grep stdc++

   # Rebuild if libraries are missing
   make clean && make

**Permission Issues:**

.. code-block:: bash

   # Fix permissions
   chmod +x gaussian_extractor.x

   # Install to user directory if system install fails
   make install-user

Runtime Issues
--------------

**Memory Errors:**

.. code-block:: bash

   # Reduce thread count
   gaussian_extractor.x -nt 2

   # Set explicit memory limit
   gaussian_extractor.x --memory-limit 4096

**File Permission Issues:**

.. code-block:: bash

   # Check file permissions
   ls -la *.log

   # Fix permissions if needed
   chmod 644 *.log

**Library Path Issues (Intel TBB):**

.. code-block:: bash

   # Check TBB library path
   echo $LD_LIBRARY_PATH

   # Add TBB path
   export LD_LIBRARY_PATH=/opt/intel/tbb/lib:$LD_LIBRARY_PATH

Cluster-Specific Issues
-----------------------

**SLURM Environment:**

.. code-block:: bash

   # Check available modules
   module avail

   # Load appropriate compiler
   module load intel-compiler-llvm
   module load intel-tbb

**PBS/Torque:**

.. code-block:: bash

   # Load modules
   module load gcc
   module load cmake

**SGE/Grid Engine:**

.. code-block:: bash

   # Load environment
   module load gcc
   module load cmake

Performance Optimization
========================

**Compiler Selection:**

- **Intel oneAPI (icpx)**: Best performance on Intel systems
- **GCC 10+**: Good general performance
- **Clang**: Alternative with good optimization

**Build Optimization:**

.. code-block:: bash

   # Release build with full optimization
   make release -j $(nproc)

   # Use all available cores for compilation
   make -j $(nproc)

**Runtime Optimization:**

.. code-block:: bash

   # Use optimal thread count
   gaussian_extractor.x -nt half

   # For large files, increase memory limit
   gaussian_extractor.x --max-file-size 500

Uninstallation
==============

**Binary Installation:**

.. code-block:: bash

   # Remove binary
   rm /usr/local/bin/gaussian_extractor.x

   # Remove configuration
   rm ~/.gaussian_extractor.conf

**Source Installation:**

.. code-block:: bash

   # From build directory
   make uninstall

   # Or manually remove files
   rm -rf /usr/local/bin/gaussian_extractor.x
   rm -rf /usr/local/share/gaussian_extractor

Getting Help
============

If you encounter issues:

1. Check the :doc:`usage` guide for proper usage
2. Use ``gaussian_extractor.x --help`` for command-line help
3. Check system requirements and compiler compatibility
4. Report issues on GitHub with system information

.. code-block:: bash

   # Get detailed system information
   gaussian_extractor.x --resource-info

   # Check compiler information
   make compiler-info