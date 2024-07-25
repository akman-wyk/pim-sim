//
// Created by wyk on 2024/7/18.
//

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "test_macro.h"

#if defined(WIN32)
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define WIFEXITED(status)   (((status) & 0x7f) == 0)
#elif defined(__linux__)
#include <sys/wait.h>
#endif

namespace pimsim {

struct UnitTestCaseConfig {
    std::string comments{};
    std::string config_file;
    std::string instruction_file;
    std::string report_file;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(UnitTestCaseConfig, comments, config_file, instruction_file,
                                                report_file)
};

struct UnitTestConfig {
    std::string name;
    std::vector<UnitTestCaseConfig> test_cases;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(UnitTestConfig, name, test_cases)
};

struct TestConfig {
    std::string root_dir;
    std::vector<UnitTestConfig> unit_test_list;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(TestConfig, root_dir, unit_test_list)
};

void unit_test(const std::string& root_dir, const UnitTestConfig& unit_test_config, bool& all_tests_passed) {
    std::cout << fmt::format("\tStart {}", unit_test_config.name) << std::endl;

    for (int i = 0; i < unit_test_config.test_cases.size(); i++) {
        const auto& test_case_config = unit_test_config.test_cases[i];
        std::cout << fmt::format("\t\tStart test case {}: {}\n\t\t\t", i, test_case_config.comments);

        auto config_file = fmt::format("{}/{}", root_dir, test_case_config.config_file);
        auto instruction_file = fmt::format("{}/{}", root_dir, test_case_config.instruction_file);
        auto report_file = fmt::format("{}/{}", root_dir, test_case_config.report_file);

        auto cmd = fmt::format("./{} {} {} {} >> ./log.txt 2>&1", unit_test_config.name, config_file, instruction_file,
                               report_file);
        int status = system(cmd.c_str());
        if (status == -1) {
            all_tests_passed = false;
            std::cout << "Fork Error!" << std::endl;
        } else if (!WIFEXITED(status)) {
            all_tests_passed = false;
            std::cout << "Abnormal Exit!" << std::endl;
        } else {
            if (int result = WEXITSTATUS(status); result == TEST_PASSED) {
                std::cout << "Passed" << std::endl;
            } else {
                all_tests_passed = false;
                if (result == TEST_FAILED) {
                    std::cout << "Failed" << std::endl;
                } else if (result == INVALID_CONFIG) {
                    std::cout << "Invalid Config" << std::endl;
                } else if (result == INVALID_USAGE) {
                    std::cout << "Invalid Usage" << std::endl;
                }
            }
        }
    }
}

void test_wrap(const char* test_config_file, bool& all_tests_passed) {
    std::ifstream ifs;
    ifs.open(test_config_file);
    auto j = nlohmann::json::parse(ifs);
    ifs.close();

    auto test_config = j.get<TestConfig>();
    // Unit Tests
    std::cout << "Start Unit Tests" << std::endl;
    for (const auto& unit_test_config : test_config.unit_test_list) {
        unit_test(test_config.root_dir, unit_test_config, all_tests_passed);
    }
    std::cout << "End Unit Tests" << std::endl;
}

}  // namespace pimsim

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: ./TestWrap [test_config_file]" << std::endl;
        return 1;
    }

    auto* test_config_file = argv[1];
    bool all_tests_passed = true;
    pimsim::test_wrap(test_config_file, all_tests_passed);
    if (all_tests_passed) {
        std::cout << "All Tests Passed!" << std::endl;
    } else {
        std::cout << "Some Tests Failed!!!!" << std::endl;
    }
    return 0;
}
