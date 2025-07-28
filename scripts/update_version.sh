#!/bin/bash

# Version Update Script for Gaussian Extractor
# This script updates version information across all relevant files
# Usage: ./scripts/update_version.sh <new_version>
# Example: ./scripts/update_version.sh v0.5.0

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if version argument is provided
if [ $# -ne 1 ]; then
    print_error "Usage: $0 <new_version>"
    print_info "Example: $0 v0.5.0"
    exit 1
fi

NEW_VERSION="$1"

# Validate version format (should be vX.Y.Z)
if [[ ! $NEW_VERSION =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    print_error "Version must be in format vX.Y.Z (e.g., v0.5.0)"
    exit 1
fi

# Extract version components
VERSION_NO_V=${NEW_VERSION#v}  # Remove 'v' prefix
IFS='.' read -r MAJOR MINOR PATCH <<< "$VERSION_NO_V"

print_info "Updating version to: $NEW_VERSION"
print_info "Major: $MAJOR, Minor: $MINOR, Patch: $PATCH"

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ] || [ ! -f "src/core/version.h" ]; then
    print_error "Please run this script from the root directory of the Gaussian Extractor project"
    exit 1
fi

# Backup files before modification
print_info "Creating backups..."
cp CMakeLists.txt CMakeLists.txt.backup
cp src/core/version.h src/core/version.h.backup

# Update CMakeLists.txt
print_info "Updating CMakeLists.txt..."
sed -i.tmp "s/VERSION v[0-9]\+\.[0-9]\+\.[0-9]\+/VERSION $NEW_VERSION/g" CMakeLists.txt
rm -f CMakeLists.txt.tmp

# Update version.h
print_info "Updating src/core/version.h..."
sed -i.tmp "s/#define GAUSSIAN_EXTRACTOR_VERSION_MAJOR [0-9]\+/#define GAUSSIAN_EXTRACTOR_VERSION_MAJOR $MAJOR/g" src/core/version.h
sed -i.tmp "s/#define GAUSSIAN_EXTRACTOR_VERSION_MINOR [0-9]\+/#define GAUSSIAN_EXTRACTOR_VERSION_MINOR $MINOR/g" src/core/version.h
sed -i.tmp "s/#define GAUSSIAN_EXTRACTOR_VERSION_PATCH [0-9]\+/#define GAUSSIAN_EXTRACTOR_VERSION_PATCH $PATCH/g" src/core/version.h
sed -i.tmp "s/#define GAUSSIAN_EXTRACTOR_VERSION_STRING \"v[0-9]\+\.[0-9]\+\.[0-9]\+\"/#define GAUSSIAN_EXTRACTOR_VERSION_STRING \"$NEW_VERSION\"/g" src/core/version.h
rm -f src/core/version.h.tmp

# Update README.MD if it contains version information
if [ -f "README.MD" ] && grep -q "v[0-9]\+\.[0-9]\+\.[0-9]\+" README.MD; then
    print_info "Updating README.MD..."
    sed -i.tmp "s/v[0-9]\+\.[0-9]\+\.[0-9]\+/$NEW_VERSION/g" README.MD
    rm -f README.MD.tmp
fi

# Verify changes
print_info "Verifying changes..."

# Check CMakeLists.txt
CMAKE_VERSION=$(grep "VERSION" CMakeLists.txt | head -1 | sed 's/.*VERSION \(v[0-9]\+\.[0-9]\+\.[0-9]\+\).*/\1/')
if [ "$CMAKE_VERSION" != "$NEW_VERSION" ]; then
    print_error "Failed to update CMakeLists.txt correctly"
    exit 1
fi

# Check version.h
VERSION_H_STRING=$(grep "GAUSSIAN_EXTRACTOR_VERSION_STRING" src/core/version.h | sed 's/.*"\(v[0-9]\+\.[0-9]\+\.[0-9]\+\)".*/\1/')
if [ "$VERSION_H_STRING" != "$NEW_VERSION" ]; then
    print_error "Failed to update version.h correctly"
    exit 1
fi

print_info "Version update completed successfully!"
print_info "Updated files:"
print_info "  - CMakeLists.txt: $CMAKE_VERSION"
print_info "  - src/core/version.h: $VERSION_H_STRING"

# Suggest next steps
print_info ""
print_info "Next steps:"
print_info "1. Review the changes: git diff"
print_info "2. Test compilation: make clean && make"
print_info "3. Test version output: ./gaussian_extractor.x --version"
print_info "4. Update changelog/release notes if needed"
print_info "5. Commit changes: git add -A && git commit -m 'Update version to $NEW_VERSION'"

# Clean up backups if everything went well
print_info "Cleaning up backup files..."
rm -f CMakeLists.txt.backup src/core/version.h.backup

print_info "Version update script completed!"
