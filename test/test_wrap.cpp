//
// Created by wyk on 2024/7/18.
//

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "fmt/format.h"
#include "nlohmann/json.hpp"

namespace pimsim {

struct UnitTestCaseConfig {
    std::string config_file;
    std::string instruction_file;
    std::string report_file;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(UnitTestCaseConfig, config_file, instruction_file, report_file)
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

void unit_test(const std::string& root_dir, const UnitTestConfig& unit_test_config) {
    std::cout << fmt::format("\tStart {}", unit_test_config.name) << std::endl;

    for (int i = 0; i < unit_test_config.test_cases.size(); i++) {
        const auto& test_case_config = unit_test_config.test_cases[i];
        std::cout << fmt::format("\t\tStart test case {}: ", i);

        auto config_file = fmt::format("{}/{}", root_dir, test_case_config.config_file);
        auto instruction_file = fmt::format("{}/{}", root_dir, test_case_config.instruction_file);
        auto report_file = fmt::format("{}/{}", root_dir, test_case_config.report_file);

        auto cmd = fmt::format("./{} {} {} {} >> ./log.txt 2>&1", unit_test_config.name, config_file, instruction_file,
                               report_file);
        if (int result = system(cmd.c_str()); result == 0) {
            std::cout << "Passed" << std::endl;
        } else {
            std::cout << "Failed" << std::endl;
        }
    }
}

void test_wrap(const char* test_config_file) {
    std::ifstream ifs;
    ifs.open(test_config_file);
    auto j = nlohmann::json::parse(ifs);
    ifs.close();

    auto test_config = j.get<TestConfig>();
    // Unit Tests
    std::cout << "Start Unit Tests" << std::endl;
    for (const auto& unit_test_config : test_config.unit_test_list) {
        unit_test(test_config.root_dir, unit_test_config);
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
    pimsim::test_wrap(test_config_file);
    return 0;
}
