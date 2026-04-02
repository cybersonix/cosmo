#ifndef COSMO_TEST_TEST_FRAMEWORK_HPP
#define COSMO_TEST_TEST_FRAMEWORK_HPP

#include <string>
#include <vector>
#include <iostream>
#include <functional>

namespace cosmo {
namespace test {

// Test case structure
struct test_case
{
    std::string name;
    std::function<void()> func;
    
    test_case(const std::string& n, std::function<void()> f)
        : name(n), func(f) {}
};

// Test registry
class test_registry
{
private:
    std::vector<test_case> tests;
    
public:
    static test_registry& instance()
    {
        static test_registry registry;
        return registry;
    }
    
    void register_test(const std::string& name, std::function<void()> func)
    {
        tests.emplace_back(name, func);
    }
    
    int run_all_tests()
    {
        int passed = 0;
        int failed = 0;
        
        std::cout << "Running " << tests.size() << " tests...\n";
        
        for (const auto& test : tests) {
            try {
                std::cout << "Running test: " << test.name << " ... ";
                test.func();
                std::cout << "PASSED\n";
                passed++;
            } catch (const std::exception& e) {
                std::cout << "FAILED (" << e.what() << ")\n";
                failed++;
            } catch (...) {
                std::cout << "FAILED (unknown exception)\n";
                failed++;
            }
        }
        
        std::cout << "\nResults: " << passed << " passed, " << failed << " failed\n";
        return failed;
    }
};

// Macros for defining tests
#define TEST_CASE(name) \
    void test_##name(); \
    static bool registered_##name = []() { \
        cosmo::test::test_registry::instance().register_test(#name, test_##name); \
        return true; \
    }(); \
    void test_##name()

#define REQUIRE(condition) \
    if (!(condition)) { \
        throw std::runtime_error("REQUIRE failed: " #condition); \
    }

#define REQUIRE_EQ(a, b) \
    if (!((a) == (b))) { \
        throw std::runtime_error("REQUIRE_EQ failed: " #a " != " #b); \
    }

#define REQUIRE_FALSE(condition) \
    if (condition) { \
        throw std::runtime_error("REQUIRE_FALSE failed: " #condition); \
    }

#define RUN_ALL_TESTS() \
    cosmo::test::test_registry::instance().run_all_tests()

} // namespace test
} // namespace cosmo

#endif // COSMO_TEST_TEST_FRAMEWORK_HPP