cmake_minimum_required(VERSION 3.14)
project("libcosmo" VERSION 0.1.0 LANGUAGES CXX C)
message(STATUS "COSMO version: ${CMAKE_PROJECT_VERSION}")

# ============================================================================
# 构建选项
# ============================================================================

option(BUILD_SHARED_LIBS "Build shared library instead of static" OFF)
option(COSMO_BUILD_TESTS "Build unit tests" ON)
option(COSMO_BUILD_EXAMPLES "Build example programs" ON)
option(COSMO_BUILD_CLI "Build CLI executable" ON)
option(COSMO_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(COSMO_ENABLE_SANITIZERS "Enable address sanitizer (Debug only)" OFF)

# ============================================================================
# 编译标准与全局配置
# ============================================================================

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(CTest)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/bin>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/bin>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/lib>)

# ============================================================================
# 编译器警告和优化选项
# ============================================================================

if(COSMO_ENABLE_WARNINGS)
    if(MSVC)
        add_compile_options(/W4 /wd4251)
    else()
        add_compile_options(
            -Wall -Wextra -Wpedantic
            -Wshadow -Wnon-virtual-dtor -Wcast-align
            -Wunused -Woverloaded-virtual
            -Wformat=2
        )
        # 以下选项仅在较新编译器中支持
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "5.0")
            add_compile_options(-Wnull-dereference -Wdouble-promotion)
        endif()
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "3.8")
            add_compile_options(-Wnull-dereference -Wdouble-promotion)
        endif()
    endif()
endif()

if(COSMO_ENABLE_SANITIZERS AND NOT MSVC)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# ============================================================================
# 主库目标
# ============================================================================

add_library(libcosmo)

target_include_directories(libcosmo PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(libcosmo PRIVATE ${CMAKE_DL_LIBS})

# C11标准支持（用于C API的bool类型）
if(NOT MSVC)
    target_compile_features(libcosmo PUBLIC c_std_99)
endif()

# 设置C API导出宏
target_compile_definitions(libcosmo PRIVATE
    COSMO_BUILDING_LIBRARY
)

target_sources(libcosmo PRIVATE
    # C API入口
    src/cosmo/cosmo.cpp
    # 基础设施
    src/logger/console_logger.cpp
    src/util/filesystem.cpp
    src/util/xml_parser.cpp
    src/util/dynamic_library.cpp
    # FMI模块
    src/fmi/model_description.cpp
    src/fmi/fmu_processor.cpp
    src/fmi/fmi_interface.cpp
    src/fmi/fmi1_interface.cpp
    src/fmi/fmi3_interface.cpp
    # 仿真引擎
    src/simulation/simulation.cpp
    src/simulation/model_instance.cpp
    src/simulation/fmi_model_instance.cpp
    src/simulation/simulation_structure.cpp
    src/simulation/simulation_runner.cpp
    src/simulation/scenario.cpp
    # 监听器
    src/listeners/csv_writer.cpp
    # SSP支持
    src/ssp/ssp_loader.cpp
    # 数据管理
    src/data/data_processor.cpp
    src/data/data_router.cpp
    # 远程执行
    src/remote/remote_executor.cpp
    # CLI
    src/cli/cli.cpp
)

set_target_properties(libcosmo PROPERTIES
    VERSION ${CMAKE_PROJECT_VERSION}
    SOVERSION ${CMAKE_PROJECT_MAJOR}.${CMAKE_PROJECT_MINOR}
    OUTPUT_NAME cosmo
    CXX_VISIBILITY_PRESET hidden
)

add_library(cosmo::cosmo ALIAS libcosmo)

# ============================================================================
# CLI 可执行文件
# ============================================================================

if(COSMO_BUILD_CLI)
    add_executable(cosmo_cli src/cli/cli_main.cpp)
    target_link_libraries(cosmo_cli PRIVATE libcosmo)
    set_target_properties(cosmo_cli PROPERTIES OUTPUT_NAME cosmo)
    install(TARGETS cosmo_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# ============================================================================
# 测试
# ============================================================================

if(COSMO_BUILD_TESTS)
    add_subdirectory(src/test)
endif()

# ============================================================================
# 示例
# ============================================================================

if(COSMO_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================================
# 安装规则
# ============================================================================

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cosmoConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cosmoConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/cosmo/cmake
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cosmoConfigVersion.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(TARGETS libcosmo
    EXPORT cosmo-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

install(EXPORT cosmo-targets
    FILE cosmoTargets.cmake
    NAMESPACE cosmo::
    DESTINATION ${CMAKE_INSTALL_DATADIR}/cosmo/cmake
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cosmoConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cosmoConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_DATADIR}/cosmo/cmake
)

# ============================================================================
# CPack 打包配置
# ============================================================================

set(CPACK_PACKAGE_NAME "COSMO")
set(CPACK_PACKAGE_VENDOR "COSMO Development Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Co-simulation engine for multi-FMU联合仿真")
set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17)")
set(CPACK_DEBIAN_PACKAGE_SECTION "science")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")

include(CPack)
