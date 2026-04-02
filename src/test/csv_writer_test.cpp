#include "cosmo/listeners/csv_writer.hpp"
#include "cosmo/test/test_framework.hpp"
#include "cosmo/logger/logger.hpp"
#include <fstream>
#include <cstdio>

using namespace cosmo::listeners;
using namespace cosmo::test;

TEST_CASE(csv_config_defaults)
{
    csv_config config;
    REQUIRE_EQ(config.get_downsample_factor(), 1u);
    REQUIRE_FALSE(config.get_clear_on_reset());
    REQUIRE(config.get_registered_variables().empty());
}

TEST_CASE(csv_config_register)
{
    csv_config config;
    config.register_variable("model1::variable1");
    config.register_variable("model2", "variable2");

    const auto& vars = config.get_registered_variables();
    REQUIRE_EQ(vars.size(), 2u);
    REQUIRE_EQ(vars[0], std::string("model1::variable1"));
    REQUIRE_EQ(vars[1], std::string("model2::variable2"));
}

TEST_CASE(csv_config_downsample_clamp)
{
    csv_config config;
    config.set_downsample_factor(5);
    REQUIRE_EQ(config.get_downsample_factor(), 5u);

    config.set_downsample_factor(0);
    REQUIRE_EQ(config.get_downsample_factor(), 1u);

    config.set_downsample_factor(-1);
    REQUIRE_EQ(config.get_downsample_factor(), 1u);
}

TEST_CASE(csv_config_clear_on_reset)
{
    csv_config config;
    REQUIRE_FALSE(config.get_clear_on_reset());
    config.set_clear_on_reset(true);
    REQUIRE(config.get_clear_on_reset());
}

TEST_CASE(csv_writer_basic)
{
    csv_config config;
    config.register_variable("time");
    config.register_variable("value");

    std::string filename = "/tmp/cosmo_test_basic.csv";
    auto writer = create_csv_writer(filename, config);

    REQUIRE_EQ(writer->get_name(), std::string("csv_writer"));
    REQUIRE_EQ(writer->get_output_path(), filename);

    writer->on_pre_init(0.0);
    writer->on_post_init(0.0);
    writer->on_pre_step(0.0, 0.1);
    writer->on_post_step(0.0, 0.1);
    writer->on_pre_step(0.1, 0.1);
    writer->on_post_step(0.1, 0.1);
    writer->on_pre_terminate();
    writer->on_post_terminate();

    std::remove(filename.c_str());
}

TEST_CASE(csv_writer_file_generation)
{
    std::string filename = "/tmp/cosmo_test_gen.csv";
    csv_config config;
    config.register_variable("time");
    config.register_variable("x");
    config.register_variable("y");

    auto writer = create_csv_writer(filename, config);

    writer->on_pre_init(0.0);
    writer->on_post_init(0.0);

    for (int i = 0; i < 5; ++i) {
        double time = i * 0.1;
        writer->on_pre_step(time, 0.1);
        writer->on_post_step(time, 0.1);
    }

    writer->on_pre_terminate();
    writer->on_post_terminate();

    std::ifstream file(filename);
    REQUIRE(file.is_open());

    std::string line;
    std::getline(file, line);
    REQUIRE_EQ(line, std::string("time,x,y"));

    int line_count = 0;
    while (std::getline(file, line)) {
        if (!line.empty()) line_count++;
    }
    // 1 初始行 + 5 步数据行
    REQUIRE_EQ(line_count, 6);

    file.close();
    std::remove(filename.c_str());
}

TEST_CASE(csv_writer_downsampling)
{
    std::string filename = "/tmp/cosmo_test_downsample.csv";
    csv_config config;
    config.register_variable("time");
    config.set_downsample_factor(2);

    auto writer = create_csv_writer(filename, config);

    writer->on_pre_init(0.0);
    writer->on_post_init(0.0);

    for (int i = 0; i < 10; ++i) {
        double time = i * 0.1;
        writer->on_pre_step(time, 0.1);
        writer->on_post_step(time, 0.1);
    }

    writer->on_pre_terminate();
    writer->on_post_terminate();

    std::ifstream file(filename);
    REQUIRE(file.is_open());

    std::string line;
    std::getline(file, line); // skip header

    int line_count = 0;
    while (std::getline(file, line)) {
        if (!line.empty()) line_count++;
    }
    // 初始行 + 步进数据中偶数步(step_counter: 2,4,6,8,10 -> 0,2,4,6,8)
    // 由于降采样逻辑: factor=2, 只有 step_counter % factor == 0 时写入
    // 初始行(在 on_post_init, step_counter=0, 0%2==0 -> 写入)
    // step 1: counter=1, 1%2!=0 -> 跳过
    // step 2: counter=2, 2%2==0 -> 写入
    // step 3: counter=3, 跳过
    // step 4: counter=4, 写入
    // step 5: 跳过
    // step 6: 写入
    // step 7: 跳过
    // step 8: 写入
    // step 9: 跳过
    // step 10: 写入
    // 总计: 初始1 + 5步 = 6行
    REQUIRE_EQ(line_count, 6);

    file.close();
    std::remove(filename.c_str());
}

TEST_CASE(csv_writer_reset)
{
    std::string filename = "/tmp/cosmo_test_reset.csv";
    csv_config config;
    config.register_variable("time");
    config.set_clear_on_reset(true);

    auto writer = create_csv_writer(filename, config);

    writer->on_pre_init(0.0);
    writer->on_post_init(0.0);
    writer->on_pre_step(0.0, 0.1);
    writer->on_post_step(0.0, 0.1);

    writer->on_reset();

    writer->on_pre_step(0.1, 0.1);
    writer->on_post_step(0.1, 0.1);

    writer->on_pre_terminate();
    writer->on_post_terminate();

    std::ifstream file(filename);
    REQUIRE(file.is_open());

    std::string line;
    std::getline(file, line); // header

    // 只有一行数据（重置后的）
    std::getline(file, line);
    REQUIRE_FALSE(line.empty());

    // 没有更多行
    REQUIRE_FALSE(static_cast<bool>(std::getline(file, line)));

    file.close();
    std::remove(filename.c_str());
}

TEST_CASE(csv_writer_empty_vars)
{
    std::string filename = "/tmp/cosmo_test_empty.csv";
    csv_config config;
    auto writer = create_csv_writer(filename, config);

    writer->on_pre_init(0.0);
    writer->on_post_init(0.0);
    writer->on_pre_step(0.0, 0.1);
    writer->on_post_step(0.0, 0.1);
    writer->on_pre_terminate();
    writer->on_post_terminate();

    std::ifstream file(filename);
    REQUIRE(file.is_open());
    file.close();

    std::remove(filename.c_str());
}

TEST_CASE(csv_writer_performance)
{
    std::string filename = "/tmp/cosmo_test_perf.csv";
    csv_config config;
    config.register_variable("time");
    config.register_variable("x");
    config.register_variable("y");
    config.register_variable("z");

    auto writer = create_csv_writer(filename, config);

    writer->on_pre_init(0.0);
    writer->on_post_init(0.0);

    for (int i = 0; i < 1000; ++i) {
        double time = i * 0.001;
        writer->on_pre_step(time, 0.001);
        writer->on_post_step(time, 0.001);
    }

    writer->on_pre_terminate();
    writer->on_post_terminate();

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    REQUIRE(file.is_open());
    REQUIRE(file.tellg() > 0);

    file.close();
    std::remove(filename.c_str());
}

int main()
{
    cosmo::log::set_logging_level(cosmo::log::warn_level);
    cosmo::log::info("Starting CSV writer test suite");
    return RUN_ALL_TESTS();
}
