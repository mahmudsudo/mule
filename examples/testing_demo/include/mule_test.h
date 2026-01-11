#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>

namespace mule {
    struct TestCase {
        std::string name;
        std::function<void()> func;
    };

    inline std::vector<TestCase>& get_tests() {
        static std::vector<TestCase> tests;
        return tests;
    }

    struct TestRegistrar {
        TestRegistrar(const std::string& name, std::function<void()> func) {
            get_tests().push_back({name, func});
        }
    };

    inline void assert_true(bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error("Assertion failed: " + message);
        }
    }
}

#define MULE_TEST(name) \
    void mule_test_##name(); \
    static mule::TestRegistrar mule_registrar_##name(#name, mule_test_##name); \
    void mule_test_##name()

#define MULE_ASSERT(condition) mule::assert_true(condition, #condition)
