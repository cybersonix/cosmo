#include "cosmo/cosmo.h"
#include "cosmo/logger/logger.hpp"

#include <iostream>

int main()
{
    // Test C API
    cosmo_info("Starting COSMO test program");
    cosmo_set_log_level("debug");
    cosmo_debug("Debug message from C API");
    cosmo_warn("Warning message from C API");
    cosmo_error("Error message from C API");
    
    // Test C++ API
    cosmo::log::info("Starting COSMO test program (C++ API)");
    cosmo::log::set_logging_level(cosmo::log::debug_level);
    cosmo::log::debug("Debug message from C++ API");
    cosmo::log::warn("Warning message from C++ API");
    cosmo::log::error("Error message from C++ API");
    
    // Test version
    cosmo_version version = cosmo_library_version();
    std::cout << "COSMO version: " << version.major << "." << version.minor << "." << version.patch << std::endl;
    
    return 0;
}
