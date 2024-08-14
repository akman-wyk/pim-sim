//
// Created by wyk on 2024/8/13.
//

#include "network_simulator.h"

namespace pimsim {

Reporter test_network(const std::string& data_root_dir, const std::string& report_root_dir, const std::string& network,
                      const TestCaseConfig& test_case_config, const std::vector<LayerConfig>& layer_config) {
    auto data_dir = fmt::format("{}/{}/{}", data_root_dir, network, test_case_config.test_case_name);
    auto report_dir = fmt::format("{}/{}", report_root_dir, TEMP_REPORT_DIR_NAME);

    std::size_t execute_times = layer_config.size();
    Reporter total_reporter;
    for (std::size_t i = 0; i < execute_times; i++) {
        std::cout << fmt::format("    execute file{}: {}\n", i, layer_config[i].sub_dir_name);

        auto code_file = fmt::format("{}/{}/{}", data_dir, layer_config[i].sub_dir_name, CODE_FILE_NAME);
        auto global_image_file =
            fmt::format("{}/{}/{}", data_dir, layer_config[i].sub_dir_name, GLOBAL_IMAGE_FILE_NAME);
        auto report_json_file = fmt::format("{}/report{}.json", report_dir, i);

        // execute and generate json file
        auto cmd = fmt::format("./LayerSimulator {} {} {} -j {} >> ./log.txt 2>&1", test_case_config.config_file_path,
                               code_file, global_image_file, report_json_file);
        system(cmd.c_str());

        // read json file, get reporter and add reporter
        std::ifstream ifs(report_json_file);
        auto report_json = nlohmann::json::parse(ifs);
        ifs.close();

        auto reporter = report_json.get<Reporter>();
        total_reporter += reporter;

        // delete json file
        remove(report_json_file.c_str());
    }

    auto total_report_file_path = fmt::format("{}/{}/{}", report_root_dir, network, test_case_config.report_file_name);
    std::ofstream ofs(total_report_file_path);
    total_reporter.report(ofs);
    ofs.close();

    return std::move(total_reporter);
}

void test_wrap(const std::string& test_config_file) {
    std::ifstream ifs(test_config_file);
    auto test_config_json = nlohmann::json::parse(ifs);
    ifs.close();

    auto test_config = test_config_json.get<TestConfig>();
    std::map<std::string, Reporter> reporters;

    if (test_config.generate_report) {
        for (int i = 0; i < test_config.test_case_config.size(); i++) {
            if (const auto& test_case = test_config.test_case_config[i]; test_case.test) {
                std::cout << fmt::format("Testing case {}: {}", i, test_case.test_case_name) << std::endl;
                auto reporter = test_network(test_config.data_root_dir, test_config.report_root_dir,
                                             test_config.network, test_case, test_config.layer_config);
                reporters.emplace(test_case.test_case_name, std::move(reporter));
                std::cout << fmt::format("Finish test case {}\n", i) << std::endl;
            }
        }
    }

    if (test_config.compare) {
        std::cout << "Generating compare report" << std::endl;
        for (auto& [compare, test_case_1, test_case_2] : test_config.compare_config) {
            if (!compare) {
                continue;
            }

            if (reporters.find(test_case_1) == reporters.end() || reporters.find(test_case_2) == reporters.end()) {
                std::cout << fmt::format("    No input data for comparing {} with {}", test_case_1, test_case_2)
                          << std::endl;
                continue;
            }

            std::cout << fmt::format("    Comparing {} with {}", test_case_1, test_case_2) << std::endl;
            auto& r1 = reporters[test_case_1];
            auto& r2 = reporters[test_case_2];
            auto compare_r = r1.compare(r2);

            auto comp_file = fmt::format("{}/{}/{}_cmp_{}.txt", test_config.report_root_dir, test_config.network,
                                         test_case_1, test_case_2);
            std::ofstream comp_ofs(comp_file);
            compare_r.report(comp_ofs);
            comp_ofs.close();
        }
    }
}

}  // namespace pimsim

int sc_main(int argc, char** argv) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    if (argc != 2) {
        std::cerr << "Usage: ./NetworkSimulator <test_config_file>" << std::endl;
        return EXIT_FAILURE;
    }
    pimsim::test_wrap(argv[1]);
    return 0;
}
