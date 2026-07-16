#ifndef TEST_HPP
#define TEST_HPP

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>

namespace mini_test {

    // ---------------- COUNTERS ----------------
    inline int tests_run = 0;
    inline int tests_failed = 0;
    inline bool current_test_failed = false;

    // ---------------- COLORS ----------------
    inline constexpr const char* GREEN  = "\033[32m";
    inline constexpr const char* RED    = "\033[31m";
    inline constexpr const char* YELLOW = "\033[33m";
    inline constexpr const char* RESET  = "\033[0m";

    // ---------------- FAIL MACRO ----------------
    #define FAIL(msg) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            std::cerr << mini_test::RED << "[FAILED] " << mini_test::RESET << oss.str() << "\n"; \
            mini_test::current_test_failed = true; \
        } while (0)

    // ---------------- REQUIRE MACROS ----------------
    #define REQUIRE(cond) \
        do { \
            if (!(cond)) { \
                std::ostringstream oss; \
                oss << __FILE__ << ":" << __LINE__ \
                    << " -- REQUIRE(" << #cond << ") failed."; \
                std::cerr << mini_test::RED << "[FAILED] " << mini_test::RESET << oss.str() << "\n"; \
                mini_test::current_test_failed = true; \
            } \
        } while (0)

    #define REQUIRE_EQ(a, b) \
        do { \
            auto _a = (a); \
            auto _b = (b); \
            if (!(_a == _b)) { \
                std::ostringstream oss; \
                oss << __FILE__ << ":" << __LINE__ \
                    << " -- REQUIRE_EQ(" << #a << ", " << #b << ") failed\n" \
                    << "   left:  " << _a << "\n" \
                    << "   right: " << _b; \
                std::cerr << mini_test::RED << "[FAILED] " << mini_test::RESET << oss.str() << "\n"; \
                mini_test::current_test_failed = true; \
            } \
        } while (0)

    #define REQUIRE_NEAR(a, b, eps) \
        do { \
            auto _a = (a); \
            auto _b = (b); \
            if (std::abs(_a - _b) > (eps)) { \
                std::ostringstream oss; \
                oss << __FILE__ << ":" << __LINE__ \
                    << " -- REQUIRE_NEAR(" << #a << ", " << #b << ", " << eps << ") failed\n" \
                    << "   left:  " << _a << "\n" \
                    << "   right: " << _b << "\n" \
                    << "   eps:   " << eps; \
                std::cerr << mini_test::RED << "[FAILED] " << mini_test::RESET << oss.str() << "\n"; \
                mini_test::current_test_failed = true; \
            } \
        } while (0)

    // ---------------- TEST CASE ----------------
    #define TEST_CASE(name) \
        void name(); \
        struct name##_registrar { \
            name##_registrar() { \
                mini_test::current_test_failed = false; \
                try { \
                    name(); \
                } catch (const std::exception& e) { \
                    std::cerr << mini_test::RED << "[FAILED] " << mini_test::RESET << #name \
                              << ": Exception: " << e.what() << "\n"; \
                    mini_test::current_test_failed = true; \
                } catch (...) { \
                    std::cerr << mini_test::RED << "[FAILED] " << mini_test::RESET << #name \
                              << ": Unknown exception\n"; \
                    mini_test::current_test_failed = true; \
                } \
                if (mini_test::current_test_failed) { \
                    ++mini_test::tests_failed; \
                } else { \
                    ++mini_test::tests_run; \
                    std::cout << mini_test::GREEN << "[PASSED] " << mini_test::RESET << #name << "\n"; \
                } \
            } \
        }; \
        static name##_registrar name##_instance; \
        void name()

    // ---------------- SUMMARY ----------------
    struct TestSummary {
        ~TestSummary() {
            std::cout << "\n========================================\n";
            std::cout << "Test Summary\n";
            std::cout << "----------------------------------------\n";
            std::cout << "Total  : " << mini_test::tests_run + mini_test::tests_failed << "\n";
            std::cout << "Passed : " << mini_test::GREEN << mini_test::tests_run << mini_test::RESET << "\n";
            std::cout << "Failed : " << (mini_test::tests_failed ? mini_test::RED : mini_test::GREEN)
                      << mini_test::tests_failed << mini_test::RESET << "\n";
            std::cout << "========================================\n";

            if (mini_test::tests_failed == 0) {
                std::cout << mini_test::GREEN << "ALL TESTS PASSED" << mini_test::RESET << "\n\n";
            } else {
                std::cout << mini_test::RED << "TEST FAILURES DETECTED" << mini_test::RESET << "\n\n";
            }
        }
    };
    inline TestSummary summary;

} // namespace mini_test

#endif // TEST_HPP
