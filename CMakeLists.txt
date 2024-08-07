
# project
cmake_minimum_required(VERSION 3.20)
#cmake_policy(SET CMP0091 NEW)
project(pim-sim LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Use C++ 17")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# dependency
add_subdirectory(packages/fmt)
add_subdirectory(packages/systemc)
add_subdirectory(packages/json)
add_subdirectory(packages/zlib)
add_subdirectory(packages/filesystem)

add_library(pim-simulator STATIC ""
        src/config/config.cpp
        src/config/config.h
        src/config/config_enum.cpp
        src/config/config_enum.h
        src/config/config_enum.cpp
        src/config/config_enum.h
        src/util/macro_scope.h
        src/core/local_memory_unit/local_memory.cpp
        src/core/local_memory_unit/local_memory.h
        src/core/payload/payload.h
        src/core/payload/payload_enum.h
        src/base_component/base_module.cpp
        src/base_component/base_module.h
        src/base_component/clock.cpp
        src/base_component/clock.h
        src/base_component/energy_counter.cpp
        src/base_component/energy_counter.h
        src/util/reporter.cpp
        src/util/reporter.h
        src/core/local_memory_unit/ram.cpp
        src/core/local_memory_unit/ram.h
        src/util/util.cpp
        src/util/util.h
        src/core/local_memory_unit/reg_buffer.cpp
        src/core/local_memory_unit/reg_buffer.h
        src/core/local_memory_unit/local_memory_unit.cpp
        src/core/local_memory_unit/local_memory_unit.h
        src/core/simd_unit/simd_unit.cpp
        src/core/simd_unit/simd_unit.h
        src/base_component/fsm.h
        src/base_component/submodule_socket.h
        src/base_component/memory_socket.cpp
        src/base_component/memory_socket.h
        src/core/payload/payload.cpp
        src/util/log.h
        src/core/transfer_unit/transfer_unit.cpp
        src/core/transfer_unit/transfer_unit.h
        src/isa/isa.h
        src/core/payload/payload_enum.cpp
        src/core/scalar_unit/scalar_unit.cpp
        src/core/scalar_unit/scalar_unit.h
        src/core/reg_unit/reg_unit.cpp
        src/core/reg_unit/reg_unit.h
        src/base_component/register.h
        src/base_component/reg_unit_socket.cpp
        src/base_component/reg_unit_socket.h
        src/core/pim_unit/macro.cpp
        src/core/pim_unit/macro.h
        src/core/pim_unit/pim_payload.h
        src/core/pim_unit/macro_group.cpp
        src/core/pim_unit/macro_group.h
        src/core/pim_unit/macro_group_controller.cpp
        src/core/pim_unit/macro_group_controller.h
        src/core/pim_unit/pim_compute_unit.cpp
        src/core/pim_unit/pim_compute_unit.h
        src/core/pim_unit/pim_output_unit.cpp
        src/core/pim_unit/pim_output_unit.h
        src/core/pim_unit/pim_transfer_unit.cpp
        src/core/pim_unit/pim_transfer_unit.h
        src/core/pim_unit/pim_set_unit.cpp
        src/core/pim_unit/pim_set_unit.h
        src/core/pim_unit/pim_load_unit.cpp
        src/core/pim_unit/pim_load_unit.h
)
set_target_properties(pim-simulator PROPERTIES OUTPUT_NAME "pim-simulator")
target_include_directories(pim-simulator PRIVATE
        src
)

# header-only dependency
target_include_directories(pim-simulator PUBLIC packages/header-only)
target_include_directories(pim-simulator PUBLIC packages/header-only/zstr/src)

add_dependencies(pim-simulator systemc zlibstatic nlohmann_json fmt ghc_filesystem)

target_link_libraries(pim-simulator PUBLIC
        systemc
        zlibstatic
        nlohmann_json
        fmt
        ghc_filesystem
)

add_executable(ChipTest "" src/main.cpp)
add_dependencies(ChipTest pim-simulator)
target_link_libraries(ChipTest PRIVATE pim-simulator)
target_include_directories(ChipTest PRIVATE src)

add_executable(ConfigTest test/other_test/config_test.cpp
        src/config/config.h
        src/config/config.cpp
        src/config/config_enum.h
        src/config/config_enum.cpp
        src/util/macro_scope.h)
add_dependencies(ConfigTest nlohmann_json fmt)
target_link_libraries(ConfigTest PUBLIC nlohmann_json fmt)
target_include_directories(ConfigTest PRIVATE src)
target_include_directories(ConfigTest PUBLIC packages/header-only)

add_executable(LocalMemoryUnitTest "" test/other_test/local_memory_unit_test.cpp)
add_dependencies(LocalMemoryUnitTest pim-simulator)
target_link_libraries(LocalMemoryUnitTest PRIVATE pim-simulator)
target_include_directories(LocalMemoryUnitTest PRIVATE src)

add_executable(SIMDUnitTest "" test/execute_unit_test/simd_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(SIMDUnitTest pim-simulator)
target_link_libraries(SIMDUnitTest PRIVATE pim-simulator)
target_include_directories(SIMDUnitTest PRIVATE src)

add_executable(TransferUnitTest "" test/execute_unit_test/transfer_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(TransferUnitTest pim-simulator)
target_link_libraries(TransferUnitTest PRIVATE pim-simulator)
target_include_directories(TransferUnitTest PRIVATE src)

add_executable(MacroTest "" test/other_test/macro_test.cpp)
add_dependencies(MacroTest pim-simulator)
target_link_libraries(MacroTest PRIVATE pim-simulator)
target_include_directories(MacroTest PRIVATE src)

add_executable(MacroGroupTest "" test/other_test/macro_group_test.cpp)
add_dependencies(MacroGroupTest pim-simulator)
target_link_libraries(MacroGroupTest PRIVATE pim-simulator)
target_include_directories(MacroGroupTest PRIVATE src)

add_executable(PimComputeUnitTest "" test/execute_unit_test/pim_compute_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(PimComputeUnitTest pim-simulator)
target_link_libraries(PimComputeUnitTest PRIVATE pim-simulator)
target_include_directories(PimComputeUnitTest PRIVATE src)

add_executable(PimOutputUnitTest "" test/execute_unit_test/pim_output_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(PimOutputUnitTest pim-simulator)
target_link_libraries(PimOutputUnitTest PRIVATE pim-simulator)
target_include_directories(PimOutputUnitTest PRIVATE src)

add_executable(PimSetUnitTest "" test/execute_unit_test/pim_set_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h
)
add_dependencies(PimSetUnitTest pim-simulator)
target_link_libraries(PimSetUnitTest PRIVATE pim-simulator)
target_include_directories(PimSetUnitTest PRIVATE src)

add_executable(PimTransferUnitTest "" test/execute_unit_test/pim_transfer_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(PimTransferUnitTest pim-simulator)
target_link_libraries(PimTransferUnitTest PRIVATE pim-simulator)
target_include_directories(PimTransferUnitTest PRIVATE src)

add_executable(PimLoadUnitTest "" test/execute_unit_test/pim_load_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(PimLoadUnitTest pim-simulator)
target_link_libraries(PimLoadUnitTest PRIVATE pim-simulator)
target_include_directories(PimLoadUnitTest PRIVATE src)

add_executable(TestWrap "" test/test_wrap.cpp)
add_dependencies(TestWrap nlohmann_json fmt)
target_link_libraries(TestWrap PUBLIC nlohmann_json fmt)
target_include_directories(TestWrap PRIVATE src)
target_include_directories(TestWrap PUBLIC packages/header-only)
