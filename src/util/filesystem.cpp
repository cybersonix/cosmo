#include "cosmo/util/filesystem.hpp"
#include "cosmo/logger/logger.hpp"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace cosmo {
namespace util {

bool file_exists(const std::string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0) && S_ISREG(buffer.st_mode);
}

bool directory_exists(const std::string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0) && S_ISDIR(buffer.st_mode);
}

bool create_directory(const std::string& path)
{
    if (directory_exists(path)) {
        return true;
    }
    
    // Try to create the directory
    int result = mkdir(path.c_str(), 0755);
    if (result == 0) {
        return true;
    }
    
    // If it failed, it might be because parent directories don't exist
    // Try to create parent directories recursively
    if (errno == EEXIST) {
        return true;
    }
    
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        std::string parent = path.substr(0, pos);
        if (!directory_exists(parent)) {
            if (!create_directory(parent)) {
                return false;
            }
        }
        // Try again to create the directory
        result = mkdir(path.c_str(), 0755);
        if (result == 0 || errno == EEXIST) {
            return true;
        }
    }
    
    return false;
}

std::string file_extension(const std::string& path)
{
    size_t pos = path.find_last_of('.');
    if (pos != std::string::npos) {
        return path.substr(pos);
    }
    return "";
}

std::string file_name_without_extension(const std::string& path)
{
    size_t slash_pos = path.find_last_of("/\\");
    size_t dot_pos = path.find_last_of('.');
    
    if (dot_pos == std::string::npos) {
        // No extension
        if (slash_pos == std::string::npos) {
            // No directory separator
            return path;
        } else {
            // Has directory separator
            return path.substr(slash_pos + 1);
        }
    } else {
        // Has extension
        if (slash_pos == std::string::npos) {
            // No directory separator
            return path.substr(0, dot_pos);
        } else {
            // Has directory separator
            if (dot_pos > slash_pos) {
                return path.substr(slash_pos + 1, dot_pos - slash_pos - 1);
            } else {
                return path.substr(slash_pos + 1);
            }
        }
    }
}

std::string parent_directory(const std::string& path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(0, pos);
    }
    return "";
}

std::string read_file(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        cosmo::log::error("Failed to open file: " + path);
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool write_file(const std::string& path, const std::string& content)
{
    std::ofstream file(path);
    if (!file.is_open()) {
        cosmo::log::error("Failed to open file for writing: " + path);
        return false;
    }
    
    file << content;
    return file.good();
}

std::vector<std::string> list_files(const std::string& path)
{
    std::vector<std::string> files;
    
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        cosmo::log::error("Failed to open directory: " + path);
        return files;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            files.push_back(entry->d_name);
        }
    }
    
    closedir(dir);
    return files;
}

} // namespace util
} // namespace cosmo