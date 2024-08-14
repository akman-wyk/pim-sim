//
// Created by wyk on 2024/8/13.
//

#pragma once
#include <string>

#include "config/config.h"
#include "core/core.h"

namespace pimsim {

class LayerSimulator {
public:
    LayerSimulator(std::string config_file, std::string instruction_file, std::string global_image_file);

    void run();

    void report(std::ostream& os, const std::string& report_json_file);

private:
    Core* core_{nullptr};

    Config config_;
    std::string config_file_;
    std::string instruction_file_;
    std::string global_image_file_;
};

}  // namespace pimsim
