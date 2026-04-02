/**
 * @file filesystem_test.cpp
 * @brief 文件系统工具单元测试
 *
 * @copyright Copyright (c) 2026 COSMO Development Team
 */

#include "cosmo/util/filesystem.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"

#include <cstdio>

TEST_CASE(filesystem_write_and_read)
{
    std::string test_file = "test_fs_write.txt";
    std::string test_content = "Hello, COSMO!";

    REQUIRE(cosmo::util::write_file(test_file, test_content));

    std::string content = cosmo::util::read_file(test_file);
    REQUIRE_EQ(content, test_content);

    std::remove(test_file.c_str());
}

TEST_CASE(filesystem_exists)
{
    std::string test_file = "test_fs_exists.txt";
    REQUIRE(!cosmo::util::file_exists(test_file));

    REQUIRE(cosmo::util::write_file(test_file, "x"));
    REQUIRE(cosmo::util::file_exists(test_file));

    std::remove(test_file.c_str());
}

TEST_CASE(filesystem_extension)
{
    REQUIRE_EQ(cosmo::util::file_extension("file.txt"), std::string(".txt"));
    REQUIRE_EQ(cosmo::util::file_extension("path/to/file.csv"), std::string(".csv"));
    REQUIRE_EQ(cosmo::util::file_extension("noext"), std::string(""));
    REQUIRE_EQ(cosmo::util::file_extension("multiple.dots.xml"), std::string(".xml"));
}

TEST_CASE(filesystem_name_without_extension)
{
    REQUIRE_EQ(cosmo::util::file_name_without_extension("file.txt"), std::string("file"));
    REQUIRE_EQ(cosmo::util::file_name_without_extension("path/to/file.csv"), std::string("file"));
    REQUIRE_EQ(cosmo::util::file_name_without_extension("noext"), std::string("noext"));
}

TEST_CASE(filesystem_parent_directory)
{
    REQUIRE_EQ(cosmo::util::parent_directory("path/to/file.txt"), std::string("path/to"));
    REQUIRE_EQ(cosmo::util::parent_directory("file.txt"), std::string(""));
    REQUIRE_EQ(cosmo::util::parent_directory("a/b/c/d"), std::string("a/b/c"));
}

TEST_CASE(filesystem_read_nonexistent)
{
    std::string content = cosmo::util::read_file("/nonexistent/path/file.txt");
    REQUIRE(content.empty());
}

TEST_CASE(filesystem_write_empty)
{
    std::string test_file = "test_fs_empty.txt";
    REQUIRE(cosmo::util::write_file(test_file, ""));

    std::string content = cosmo::util::read_file(test_file);
    REQUIRE_EQ(content, std::string(""));

    std::remove(test_file.c_str());
}

int main()
{
    cosmo::log::set_logging_level(cosmo::log::off_level);
    return RUN_ALL_TESTS();
}
