/**
 * @file metadata.h
 * @brief Metadata utilities for standardized header and (future) summary blocks
 */

#ifndef METADATA_H
#define METADATA_H

#include <string>

namespace Metadata {

/**
 * @brief Build the standard header banner shown at the top of outputs
 * @return A multi-line string containing the header box
 */
std::string header();

} // namespace Metadata

#endif // METADATA_H


