# Makefile for Gaussian Extractor
# Enhanced Safety Edition v0.3

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O3 -pthread
DEBUGFLAGS = -g -DDEBUG_BUILD -fsanitize=address -fno-omit-frame-pointer
LDFLAGS = -pthread

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lrt
endif
ifeq ($(UNAME_S),Darwin)
    # macOS specific flags if needed
endif

# Source files
SOURCES = main.cpp gaussian_extractor.cpp job_scheduler.cpp
HEADERS = gaussian_extractor.h job_scheduler.h
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = gaussian_extractor.x

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile source files to object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug build with additional safety checks
debug: CXXFLAGS += $(DEBUGFLAGS)
debug: LDFLAGS += -fsanitize=address
debug: clean $(TARGET)

# Release build with optimizations
release: CXXFLAGS += -O3 -DNDEBUG -march=native
release: clean $(TARGET)

# Cluster-safe build (conservative optimizations)
cluster: CXXFLAGS += -O2 -DCLUSTER_BUILD
cluster: clean $(TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install to system (requires sudo)
install: $(TARGET)
	mkdir -p /usr/local/bin
	cp $(TARGET) /usr/local/bin/
	chmod +x /usr/local/bin/$(TARGET)

# Install to user's local bin
install-user: $(TARGET)
	mkdir -p ~/bin
	cp $(TARGET) ~/bin/
	chmod +x ~/bin/$(TARGET)

# Create a test build with maximum warnings
test-build: CXXFLAGS += -Wpedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy \
                        -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op \
                        -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept \
                        -Wold-style-cast -Woverloaded-virtual -Wredundant-decls \
                        -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel \
                        -Wstrict-overflow=5 -Wswitch-default -Wundef
test-build: clean $(TARGET)

# Quick test with sample files
test: $(TARGET)
	@echo "Testing Gaussian Extractor..."
	@if [ -f test-1.log ] && [ -f test-2.log ]; then \
		./$(TARGET) --resource-info; \
		./$(TARGET) -q -f csv; \
		echo "Test completed. Check output files."; \
	else \
		echo "Test files not found. Please ensure test-1.log and test-2.log exist."; \
	fi

# Check for memory leaks (requires valgrind)
memcheck: debug
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		./$(TARGET) -q; \
	else \
		echo "Valgrind not found. Install valgrind for memory checking."; \
	fi

# Create distribution package
dist: clean
	@mkdir -p gaussian-extractor-$(shell date +%Y%m%d)
	@cp *.cpp *.h Makefile CMakeLists.txt README.MD SAFETY_IMPROVEMENTS.md gaussian-extractor-$(shell date +%Y%m%d)/
	@tar czf gaussian-extractor-$(shell date +%Y%m%d).tar.gz gaussian-extractor-$(shell date +%Y%m%d)/
	@rm -rf gaussian-extractor-$(shell date +%Y%m%d)/
	@echo "Distribution package created: gaussian-extractor-$(shell date +%Y%m%d).tar.gz"

# Help target
help:
	@echo "Gaussian Extractor Makefile - Available targets:"
	@echo ""
	@echo "  all          - Build the program (default)"
	@echo "  debug        - Build with debug symbols and AddressSanitizer"
	@echo "  release      - Build optimized release version"
	@echo "  cluster      - Build cluster-safe version"
	@echo "  test-build   - Build with maximum compiler warnings"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install to /usr/local/bin (requires sudo)"
	@echo "  install-user - Install to ~/bin"
	@echo "  test         - Run basic functionality test"
	@echo "  memcheck     - Run with valgrind memory checker"
	@echo "  dist         - Create distribution package"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build with default settings"
	@echo "  make debug              # Build debug version"
	@echo "  make cluster            # Build for cluster deployment"
	@echo "  make clean install-user # Clean build and install to user bin"

# Declare phony targets
.PHONY: all debug release cluster clean install install-user test-build test memcheck dist help
