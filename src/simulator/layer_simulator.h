//
// Created by wyk on 2024/8/13.
//

#pragma once
#include <string>

#include "config/config.h"
#include "core/core.h"

#define TEST_PASSED           0
#define TEST_FAILED           1
#define INVALID_USAGE         2
#define INVALID_CONFIG        3
#define CHECK_INS_STAT_FAILED 4
#define CHECK_REG_FAILED      5

namespace pimsim {

class LayerSimulator {
public:
    LayerSimulator(std::string config_file, std::string instruction_file, std::string global_image_file,
                   std::string expected_ins_stat_file, std::string expected_reg_file, std::string actual_reg_file);

    void run();

    void report(std::ostream& os, const std::string& report_json_file);

    bool checkInsStat() const;
    bool checkReg() const;

private:
    Core* core_{nullptr};

    Config config_;
    std::string config_file_;
    std::string instruction_file_;
    std::string global_image_file_;
    std::string expected_ins_stat_file_;
    std::string expected_reg_file_;
    std::string actual_reg_file_;
};

}  // namespace pimsim
