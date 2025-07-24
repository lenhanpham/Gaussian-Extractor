# Gaussian Extractor Development Guide

## Project Structure

The project follows modern C++ development practices and is organized as follows:

### Core Directories

```
gaussian-extractor/
├── src/                  # Source code directory
│   ├── core/            # Core functionality
│   │   ├── gaussian_extractor.cpp/.h  # Main extraction logic
│   │   └── job_scheduler.cpp/.h       # Parallel processing management
│   └── main.cpp         # Application entry point
├── tests/              # Test files
├── docs/               # Documentation
├── scripts/           # Build scripts
└── build/             # Build outputs (generated)
```

## Development Guidelines

### Code Style

1. **Naming Conventions**
   - Use meaningful and descriptive names
   - Classes: PascalCase (e.g., `GaussianExtractor`)
   - Functions: camelCase (e.g., `processFile`)
   - Variables: camelCase (e.g., `energyValue`)
   - Constants: UPPER_SNAKE_CASE (e.g., `MAX_THREADS`)

2. **File Organization**
   - Header files (.h) should contain declarations only
   - Implementation files (.cpp) should contain definitions
   - One class per file (with rare exceptions)
   - Keep files focused and reasonably sized

3. **Code Documentation**
   - Document all public interfaces
   - Use clear and concise comments
   - Explain "why" rather than "what"
   - Keep comments up-to-date with code changes

### Memory Management

1. **Resource Management**
   - Use RAII principles
   - Prefer smart pointers over raw pointers
   - Avoid manual memory management
   - Use containers from the Standard Library

2. **Thread Safety**
   - Document thread safety guarantees
   - Use appropriate synchronization primitives
   - Be explicit about ownership transfer
   - Avoid global state

### Error Handling

1. **Exception Usage**
   - Use exceptions for exceptional conditions
   - Document exception guarantees
   - Maintain exception safety
   - Use appropriate exception types

2. **Error Reporting**
   - Provide meaningful error messages
   - Include context in error reports
   - Log errors appropriately
   - Consider error recovery strategies

## Building and Testing 

### Build System

The project uses CMake as its build system. Key build targets:

```bash
# Configure build
cmake -B build -S .

# Build specific targets
cmake --build build --target gaussian_extractor  # Main executable
cmake --build build --target tests               # Test suite
```

### Build Types

```bash
# Debug build with safety checks
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Enable extra compiler warnings
cmake -DENABLE_EXTRA_WARNINGS=ON ..
```

### Testing

1. **Unit Tests**
   - Write tests for new functionality
   - Maintain existing tests
   - Ensure good test coverage
   - Use meaningful test names

2. **Test Data**
   - Store test files in `tests/data/`
   - Document test file purpose
   - Keep test files minimal
   - Version control test data

## Contributing

### Making Changes

1. **Before Starting**
   - Review existing issues
   - Discuss major changes
   - Check design documentation
   - Understand requirements

2. **Development Process**
   - Create feature branch
   - Write tests first
   - Implement changes
   - Update documentation
   - Review code yourself
   - Submit pull request

### Code Review

1. **Review Checklist**
   - Code meets style guidelines
   - Tests are included
   - Documentation is updated
   - No performance regressions
   - Error handling is appropriate
   - Thread safety maintained

2. **Performance Considerations**
   - Profile significant changes
   - Consider memory usage
   - Test with large datasets
   - Verify thread scaling

## Performance Guidelines

1. **Memory Efficiency**
   - Minimize allocations
   - Use appropriate containers
   - Consider cache effects
   - Profile memory usage

2. **CPU Efficiency**
   - Use efficient algorithms
   - Leverage modern C++ features
   - Consider vectorization
   - Profile hot paths

3. **I/O Performance**
   - Batch I/O operations
   - Use buffered I/O
   - Consider async I/O
   - Profile I/O patterns

## Safety Considerations

1. **Thread Safety**
   - Document thread safety
   - Use thread-safe containers
   - Implement proper synchronization
   - Avoid race conditions

2. **Resource Management**
   - Implement proper cleanup
   - Handle system limits
   - Monitor resource usage
   - Implement graceful degradation

3. **Error Recovery**
   - Handle all error cases
   - Provide meaningful messages
   - Implement proper logging
   - Allow for graceful shutdown

## Version Control

1. **Commit Guidelines**
   - Write meaningful commit messages
   - Keep commits focused
   - Reference issues
   - Follow branching strategy

2. **Branch Strategy**
   - main: stable releases
   - develop: integration branch
   - feature/*: new features
   - fix/*: bug fixes

## Documentation

1. **Code Documentation**
   - Document public interfaces
   - Explain complex algorithms
   - Update docs with changes
   - Include examples

2. **User Documentation**
   - Keep README.MD updated
   - Document new features
   - Include usage examples
   - Explain error messages

## Maintenance

1. **Code Quality**
   - Regular refactoring
   - Remove dead code
   - Update dependencies
   - Address technical debt

2. **Testing**
   - Maintain test suite
   - Add regression tests
   - Update test data
   - Profile performance

3. **Release Process**
   - Version numbering
   - Change documentation
   - Release notes
   - Binary distributions