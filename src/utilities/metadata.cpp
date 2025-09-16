/**
 * @file metadata.cpp
 * @brief Implementation of metadata utilities
 */

#include "metadata.h"
#include "version.h"

#include <sstream>

namespace Metadata {

static std::string create_header_line(const std::string& content) {
    const size_t total_width = 61; // Including asterisks
    std::string line = "* " + content;
    while (line.length() < total_width - 2) line += " ";
    line += " *";
    return line;
}

std::string header() {
    std::ostringstream oss;
    oss << "                                                             \n"
        << "*************************************************************\n"
        << create_header_line(GaussianExtractor::get_header_info()) << "\n"
        << create_header_line(GAUSSIAN_EXTRACTOR_REPOSITORY) << "\n"
        << "*************************************************************\n"
        << "                                                             \n";
    return oss.str();
}

} // namespace Metadata


