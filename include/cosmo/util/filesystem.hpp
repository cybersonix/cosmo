#ifndef COSMO_UTIL_FILESYSTEM_HPP
#define COSMO_UTIL_FILESYSTEM_HPP

#include <string>
#include <vector>
#include <fstream>

namespace cosmo {
namespace util {

// Check if a file exists
bool file_exists(const std::string& path);

// Check if a directory exists
bool directory_exists(const std::string& path);

// Create a directory
bool create_directory(const std::string& path);

// Get the file extension
std::string file_extension(const std::string& path);

// Get the file name without extension
std::string file_name_without_extension(const std::string& path);

// Get the parent directory
std::string parent_directory(const std::string& path);

// Read entire file content into a string
std::string read_file(const std::string& path);

// Write string content to a file
bool write_file(const std::string& path, const std::string& content);

// List files in a directory
std::vector<std::string> list_files(const std::string& path);

} // namespace util
} // namespace cosmo

#endif // COSMO_UTIL_FILESYSTEM_HPP