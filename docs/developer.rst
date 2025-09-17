Developer Guide
===============

This guide is for developers who want to contribute to the Gaussian Extractor project. It covers the codebase structure, development setup, coding standards, and contribution guidelines.

Project Overview
================

Architecture
------------

Gaussian Extractor is a C++20 application designed for high-performance processing of Gaussian computational chemistry log files. The codebase follows a modular architecture with clear separation of concerns:

.. code-block::
├── src/
│   ├── main.cpp                           # Application entry point
│   ├── extraction/
│   │   ├── coord_extractor.cpp
│   │   ├── coord_extractor.h
│   │   ├── gaussian_extractor.cpp
│   │   └── gaussian_extractor.h
│   ├── high_level/
│   │   ├── high_level_energy.cpp
│   │   └── high_level_energy.h
│   ├── input_gen/
│   │   ├── create_input.cpp
│   │   ├── create_input.h
│   │   ├── parameter_parser.cpp
│   │   └── parameter_parser.h
│   ├── job_management/
│   │   ├── job_checker.cpp
│   │   ├── job_checker.h
│   │   ├── job_scheduler.cpp
│   │   └── job_scheduler.h
│   ├── ui/
│   │   ├── help_utils.cpp
│   │   ├── help_utils.h
│   │   ├── interactive_mode.cpp
│   │   └── interactive_mode.h
│   └── utilities/
│       ├── command_system.cpp
│       ├── command_system.h
│       ├── config_manager.cpp
│       ├── config_manager.h
│       ├── metadata.cpp
│       ├── metadata.h
│       ├── module_executor.cpp
│       ├── module_executor.h
│       ├── utils.cpp
│       ├── utils.h
│       └── version.h
├── tests/
├── docs/
├── resources/
├── CMakeLists.txt                      # CMake build configuration
├── Doxyfile                            # Doxygen configuration
├── LICENSE                             # Project license
├── Makefile                            # Make build system
└── README.MD                           # User documentation


New Modules in v0.5.0
---------------------

**Interactive Mode (interactive_mode.h/.cpp)**
    - Windows-specific interactive interface
    - Menu-driven command selection
    - Automatic extraction before entering interactive mode

**Coordinate Processing (coord_extractor.h/.cpp)**
    - Extract final Cartesian coordinates from log files
    - XYZ format conversion and organization
    - Support for completed and running job separation

**Input Generation (create_input.h/.cpp)**
    - Generate Gaussian input files from XYZ coordinates
    - Template system for reusable parameter sets
    - Support for multiple calculation types (SP, OPT, TS, IRC)

**High-Level Energy Calculations (high_level_energy.h/.cpp)**
    - Combine high-level electronic energies with low-level thermal corrections
    - Support for kJ/mol and atomic unit outputs
    - Directory-based energy combination workflow

**Job Status Management (job_checker.h/.cpp)**
    - Comprehensive job status checking and organization
    - Support for multiple error types (PCM, imaginary frequencies)
    - Automated file organization by job status

**Metadata Handling (metadata.h/.cpp)**
    - File metadata extraction and validation
    - Job completion status detection
    - File size and timestamp tracking

**Parameter File Parsing (parameter_parser.h/.cpp)**
    - Template parameter file parsing
    - Configuration file format support
    - Validation and error reporting

Key Design Principles
---------------------

**Modularity**
   - Each module has a single responsibility
   - Clear interfaces between components
   - Easy to test and maintain

**Performance**
   - Multi-threaded processing
   - Memory-efficient algorithms
   - Cluster-aware resource management

**Safety**
   - Comprehensive error handling
   - Resource cleanup on failures
   - Graceful shutdown handling

**Usability**
   - Intuitive command-line interface
   - Extensive help system
   - Configuration file support

Development Setup
=================

Prerequisites
-------------

**Required Tools:**

- **C++ Compiler**: GCC 10+, Intel oneAPI, or Clang 10+
- **Build System**: Make (included with most Linux distributions)
- **Documentation**: Sphinx (for building documentation)
- **Git**: Version control system

**Optional Tools:**

- **CMake**: Alternative build system
- **Doxygen**: API documentation generation
- **Valgrind**: Memory debugging
- **Clang-Tidy**: Code analysis

Getting the Source Code
-----------------------

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/lenhanpham/gaussian-extractor.git
   cd gaussian-extractor

   # Create a development branch
   git checkout -b feature/your-feature-name

Building for Development
------------------------

**Debug Build:**

.. code-block:: bash

   # Build with debug symbols and safety checks
   make debug

   # Or with CMake
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make

**Release Build:**

.. code-block:: bash

   # Optimized release build
   make release

   # Or with CMake
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make

**Development Build with All Features:**

.. code-block:: bash

   # Full development build
   make -j $(nproc)

Testing
-------

**Running Tests:**

.. code-block:: bash

   # Build and run tests
   make test

   # Run specific test suite
   ./test_runner --suite extraction_tests

   # Run with verbose output
   ./test_runner -v

**Test Coverage:**

.. code-block:: bash

   # Generate coverage report
   make coverage

   # View coverage in browser
   firefox coverage_report/index.html

Code Quality Tools
------------------

**Static Analysis:**

.. code-block:: bash

   # Run clang-tidy
   clang-tidy src/core/*.cpp -- -std=c++20 -Isrc

   # Run cppcheck
   cppcheck --enable=all --std=c++20 src/

**Code Formatting:**

.. code-block:: bash

   # Format code with clang-format
   find src/ -name "*.cpp" -o -name "*.h" | xargs clang-format -i

   # Check formatting
   find src/ -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run -Werror

Documentation
-------------

**Building Documentation:**

.. code-block:: bash

   # Install Sphinx
   pip install sphinx sphinx-rtd-theme

   # Build HTML documentation
   cd docs
   make html

   # View documentation
   firefox _build/html/index.html

**API Documentation:**

.. code-block:: bash

   # Generate Doxygen documentation
   doxygen Doxyfile

   # View API docs
   firefox doxygen/html/index.html

Coding Standards
================

Code Style
----------

**Naming Conventions:**

.. code-block:: cpp

   // Classes and structs
   class CommandParser;
   struct CommandContext;

   // Functions and methods
   void parse_command_line(int argc, char* argv[]);
   CommandContext create_context();

   // Variables
   int thread_count;
   std::string output_file;

   // Constants
   const int DEFAULT_THREAD_COUNT = 4;
   const std::string CONFIG_FILE_NAME = ".gaussian_extractor.conf";

   // Member variables (with m_ prefix)
   class MyClass {
   private:
       int m_thread_count;
       std::string m_config_file;
   };

**File Organization:**

- **Headers (.h)**: Class declarations, function prototypes, constants
- **Implementations (.cpp)**: Function definitions, implementation details
- **One class per file** when possible
- **Related functionality grouped** in modules

Documentation Standards
-----------------------

**Doxygen Comments:**

.. code-block:: cpp

   /**
    * @brief Brief description of the function/class
    *
    * Detailed description explaining what the function does,
    * its parameters, return values, and any important notes.
    *
    * @param param1 Description of first parameter
    * @param param2 Description of second parameter
    * @return Description of return value
    *
    * @section Usage Example
    * @code
    * // Example usage
    * int result = my_function(param1, param2);
    * @endcode
    *
    * @note Important notes about usage or limitations
    * @warning Warnings about potential issues
    * @see Related functions or classes
    */
   int my_function(int param1, const std::string& param2);

**Inline Comments:**

.. code-block:: cpp

   // Use comments for complex logic
   if (condition) {
       // Explain why this condition is important
       do_something();
   }

   // Use TODO comments for future improvements
   // TODO: Optimize this loop for better performance

Error Handling
--------------

**Exception Safety:**

.. code-block:: cpp

   try {
       // Operation that might fail
       process_files(file_list);
   } catch (const std::invalid_argument& e) {
       // Handle invalid arguments
       std::cerr << "Invalid argument: " << e.what() << std::endl;
       return 1;
   } catch (const std::runtime_error& e) {
       // Handle runtime errors
       std::cerr << "Runtime error: " << e.what() << std::endl;
       return 2;
   } catch (const std::exception& e) {
       // Handle all other exceptions
       std::cerr << "Unexpected error: " << e.what() << std::endl;
       return 3;
   }

**Return Codes:**

.. code-block:: cpp

   /**
    * @return 0 on success
    * @return 1 on general error
    * @return 2 on invalid arguments
    * @return 3 on resource unavailable
    * @return 4 on operation interrupted
    */
   int process_data(const std::string& input_file);

Memory Management
-----------------

**RAII Pattern:**

.. code-block:: cpp

   class FileProcessor {
   public:
       FileProcessor(const std::string& filename)
           : m_file(filename) {
           if (!m_file.is_open()) {
               throw std::runtime_error("Failed to open file");
           }
       }

       ~FileProcessor() {
           // Automatic cleanup
           if (m_file.is_open()) {
               m_file.close();
           }
       }

   private:
       std::ifstream m_file;
   };

**Smart Pointers:**

.. code-block:: cpp

   // Use unique_ptr for exclusive ownership
   std::unique_ptr<CommandContext> context = std::make_unique<CommandContext>();

   // Use shared_ptr for shared ownership
   std::shared_ptr<ConfigManager> config = std::make_shared<ConfigManager>();

Thread Safety
-------------

**Thread-Safe Classes:**

.. code-block:: cpp

   class ThreadSafeCounter {
   public:
       void increment() {
           std::lock_guard<std::mutex> lock(m_mutex);
           ++m_count;
       }

       int get_count() const {
           std::lock_guard<std::mutex> lock(m_mutex);
           return m_count;
       }

   private:
       mutable std::mutex m_mutex;
       int m_count{0};
   };

**Threading Guidelines:**

- Document thread safety guarantees
- Use appropriate synchronization primitives
- Avoid global mutable state
- Test concurrent access patterns

Contributing
============

Development Workflow
--------------------

**1. Choose an Issue:**

.. code-block:: bash

   # Check available issues
   # Visit: https://github.com/lenhanpham/gaussian-extractor

**2. Create a Branch:**

.. code-block:: bash

   # Create and switch to feature branch
   git checkout -b feature/descriptive-name

   # Or for bug fixes
   git checkout -b bugfix/issue-number-description

**3. Make Changes:**

.. code-block:: bash

   # Make your changes following coding standards
   # Add tests for new functionality
   # Update documentation as needed

**4. Test Your Changes:**

.. code-block:: bash

   # Build and test
   make debug
   make test

   # Run code quality checks
   make lint

**5. Commit Your Changes:**

.. code-block:: bash

   # Stage your changes
   git add .

   # Commit with descriptive message
   git commit -m "feat: add new feature description

   - What was changed
   - Why it was changed
   - How it was tested"

**6. Push and Create Pull Request:**

.. code-block:: bash

   # Push your branch
   git push origin feature/your-feature-name

   # Create pull request on GitHub

Pull Request Guidelines
-----------------------

**PR Title Format:**

.. code-block::

   type(scope): description

   Types: feat, fix, docs, style, refactor, test, chore

**PR Description Template:**

.. code-block::

   ## Description
   Brief description of the changes

   ## Type of Change
   - [ ] Bug fix
   - [ ] New feature
   - [ ] Breaking change
   - [ ] Documentation update

   ## Testing
   - [ ] Unit tests added/updated
   - [ ] Integration tests added/updated
   - [ ] Manual testing performed

   ## Checklist
   - [ ] Code follows style guidelines
   - [ ] Documentation updated
   - [ ] Tests pass
   - [ ] No breaking changes

Code Review Process
-------------------

**Review Checklist:**

- [ ] Code follows established patterns
- [ ] Appropriate error handling
- [ ] Thread safety considerations
- [ ] Performance implications
- [ ] Documentation updated
- [ ] Tests included
- [ ] No security vulnerabilities

**Review Comments:**

- Be constructive and specific
- Suggest improvements, don't just point out problems
- Reference coding standards when applicable
- Acknowledge good practices

Testing Guidelines
==================

Unit Testing
------------

**Test Structure:**

.. code-block:: cpp

   #include <gtest/gtest.h>
   #include "core/command_system.h"

   class CommandParserTest : public ::testing::Test {
   protected:
       void SetUp() override {
           // Setup code
       }

       void TearDown() override {
           // Cleanup code
       }
   };

   TEST_F(CommandParserTest, ParseExtractCommand) {
       // Test extract command parsing
       char* argv[] = {"gaussian_extractor.x", "extract", "-t", "300"};
       CommandContext context = CommandParser::parse(4, argv);

       EXPECT_EQ(context.command, CommandType::EXTRACT);
       EXPECT_EQ(context.temp, 300.0);
   }

**Running Tests:**

.. code-block:: bash

   # Run all tests
   make test

   # Run specific test
   ./test_runner --gtest_filter=CommandParserTest.ParseExtractCommand

   # Run with coverage
   make coverage

Integration Testing
-------------------

**End-to-End Tests:**

.. code-block:: bash

   # Test complete workflows
   ./test_integration.sh

   # Test with sample data
   ./gaussian_extractor.x -f test_data/ --output test_results/

Performance Testing
-------------------

**Benchmarking:**

.. code-block:: bash

   # Run performance benchmarks
   make benchmark

   # Profile application
   valgrind --tool=callgrind ./gaussian_extractor.x [args]

   # Memory profiling
   valgrind --tool=massif ./gaussian_extractor.x [args]

Continuous Integration
======================

CI/CD Pipeline
--------------

**Automated Testing:**

- **Build**: Compile on multiple platforms (Linux, Windows)
- **Test**: Run unit and integration tests
- **Lint**: Code quality checks
- **Docs**: Build documentation
- **Release**: Automated releases

**GitHub Actions Workflow:**

.. code-block:: yaml

   name: CI
   on: [push, pull_request]
   jobs:
     build:
       runs-on: ubuntu-latest
       steps:
         - uses: actions/checkout@v2
         - name: Build
           run: make -j 4
         - name: Test
           run: make test
         - name: Lint
           run: make lint

Release Process
===============

Version Numbering
-----------------

**Semantic Versioning:**

.. code-block::

   MAJOR.MINOR.PATCH

   - MAJOR: Breaking changes
   - MINOR: New features (backward compatible)
   - PATCH: Bug fixes (backward compatible)

**Release Checklist:**

- [ ] Update version in version.h
- [ ] Update CHANGELOG.md
- [ ] Update documentation
- [ ] Create release branch
- [ ] Run full test suite
- [ ] Create GitHub release
- [ ] Update package repositories

**Release Commands:**

.. code-block:: bash

   # Create release branch
   git checkout -b release/v1.2.3

   # Update version
   echo "1.2.3" > VERSION

   # Commit and tag
   git add VERSION
   git commit -m "Release v1.2.3"
   git tag -a v1.2.3 -m "Release v1.2.3"

   # Push release
   git push origin release/v1.2.3
   git push origin v1.2.3

Support and Communication
=========================

**Communication Channels:**

- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: General questions and discussions
- **Pull Request Comments**: Code review discussions

**Getting Help:**

- Check existing issues and documentation first
- Use descriptive titles for issues
- Provide minimal reproducible examples
- Include system information and versions

**Community Guidelines:**

- Be respectful and constructive
- Help newcomers learn and contribute
- Follow the code of conduct
- Acknowledge contributions from others

This developer guide provides comprehensive information for contributing to the Gaussian Extractor project. Following these guidelines ensures high-quality, maintainable code that benefits the entire community.