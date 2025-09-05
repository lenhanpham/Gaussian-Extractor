#include "utils.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Utils
{

    std::string
    read_file_unified(const std::string& file_path, FileReadMode mode, size_t tail_lines, const std::string& pattern)
    {
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open file: " + file_path);
        }

        std::streamsize file_size = file.tellg();

        // For FULL mode or empty files, read entire content
        if (mode == FileReadMode::FULL || file_size == 0)
        {
            file.seekg(0, std::ios::beg);
            std::ostringstream buffer;
            buffer << file.rdbuf();
            file.close();
            return buffer.str();
        }

        // For TAIL or SMART mode
        if (mode == FileReadMode::TAIL || mode == FileReadMode::SMART)
        {
            if (tail_lines == 0)
            {
                file.close();
                return "";
            }

            const size_t   CHUNK_SIZE = 4096;
            std::string    accumulated;
            size_t         lines_found = 0;
            std::streampos pos         = file_size;

            // Read backwards in chunks until we have enough lines or pattern is found
            while (pos > 0 && lines_found < tail_lines + 1)
            {
                std::streampos read_pos =
                    std::max(static_cast<std::streamoff>(0),
                             static_cast<std::streamoff>(pos) - static_cast<std::streamoff>(CHUNK_SIZE));
                size_t chunk_to_read = pos - read_pos;
                pos                  = read_pos;

                // Read chunk
                std::vector<char> buffer(chunk_to_read);
                file.seekg(pos);
                file.read(buffer.data(), chunk_to_read);

                // Count newlines in this chunk and prepend to accumulated
                size_t chunk_lines = 0;
                for (size_t i = 0; i < chunk_to_read; ++i)
                {
                    if (buffer[i] == '\n')
                    {
                        chunk_lines++;
                    }
                }
                accumulated.insert(0, buffer.data(), chunk_to_read);
                lines_found += chunk_lines;
            }

            // Trim to desired number of lines
            size_t start_pos        = accumulated.length();
            size_t newlines_to_find = tail_lines;
            while (start_pos > 0 && newlines_to_find > 0)
            {
                start_pos--;
                if (accumulated[start_pos] == '\n')
                {
                    newlines_to_find--;
                }
            }
            if (start_pos < accumulated.length() && accumulated[start_pos] == '\n')
            {
                start_pos++;
            }
            std::string result = start_pos < accumulated.length() ? accumulated.substr(start_pos) : accumulated;

            // For SMART mode, check if pattern is found; if not, fall back to full read
            if (mode == FileReadMode::SMART && !pattern.empty() && result.find(pattern) == std::string::npos)
            {
                file.seekg(0, std::ios::beg);
                std::ostringstream buffer;
                buffer << file.rdbuf();
                result = buffer.str();
            }

            file.close();
            return result;
        }

        // Fallback to full read
        file.seekg(0, std::ios::beg);
        std::ostringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return buffer.str();
    }

    std::filesystem::path generate_unique_filename(const std::filesystem::path& base_path)
    {
        if (!std::filesystem::exists(base_path))
        {
            return base_path;
        }

        // Get current timestamp
        auto now    = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm     = *std::localtime(&time_t);

        // Format timestamp as YYYYMMDD_HHMMSS
        std::ostringstream timestamp;
        timestamp << std::put_time(&tm, "%Y%m%d_%H%M%S");

        // Create new filename with timestamp suffix
        std::filesystem::path parent_path = base_path.parent_path();
        std::string           stem        = base_path.stem().string();
        std::string           extension   = base_path.extension().string();

        std::filesystem::path new_path = parent_path / (stem + "_" + timestamp.str() + extension);

        return new_path;
    }

}  // namespace Utils