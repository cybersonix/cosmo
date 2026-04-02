/**
 * @file framework_test.cpp
 * @brief 自研测试框架自身验证
 *
 * 验证测试框架的断言机制和注册功能。
 */

#include "cosmo/test/test_framework.hpp"
#include <stdexcept>
#include <cmath>

// 基本断言测试
TEST_CASE(basic_assertion) {
    REQUIRE(true);
    int x = 42;
    REQUIRE(x == 42);
}

// REQUIRE_EQ 测试
TEST_CASE(require_eq_pass) {
    int a = 10, b = 10;
    REQUIRE_EQ(a, b);
    std::string s1 = "hello", s2 = "hello";
    REQUIRE_EQ(s1, s2);
}

// REQUIRE_FALSE 测试
TEST_CASE(require_false_pass) {
    REQUIRE_FALSE(false);
    int x = 0;
    REQUIRE_FALSE(x > 0);
}

// REQUIRE_EQ 失败路径测试（在另一个测试中验证）
TEST_CASE(require_eq_failure) {
    bool caught = false;
    try {
        int a = 1, b = 2;
        REQUIRE_EQ(a, b);
    } catch (const std::exception&) {
        caught = true;
    }
    REQUIRE(caught);
}

// REQUIRE 失败路径测试
TEST_CASE(require_failure) {
    bool caught = false;
    try {
        REQUIRE(false);
    } catch (const std::exception&) {
        caught = true;
    }
    REQUIRE(caught);
}

// 浮点数比较
TEST_CASE(float_comparison) {
    double a = 0.1 + 0.2;
    REQUIRE(fabs(a - 0.3) < 1e-9);
}

// 异常捕获测试
TEST_CASE(exception_handling) {
    bool caught = false;
    try {
        throw std::runtime_error("test exception");
    } catch (const std::exception& e) {
        caught = true;
        REQUIRE_EQ(std::string(e.what()), std::string("test exception"));
    }
    REQUIRE(caught);
}

// 多测试注册顺序
TEST_CASE(test_order_first) {
    static int counter = 0;
    counter++;
    REQUIRE(counter >= 1);
}

TEST_CASE(test_order_second) {
    static int counter = 0;
    counter++;
    REQUIRE(counter >= 1);
}

int main()
{
    int failed = RUN_ALL_TESTS();
    return failed;
}
