
# project
cmake_minimum_required(VERSION 3.20)
#cmake_policy(SET CMP0091 NEW)
project(pim-sim LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Use C++ 17")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

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
        src/util/macro_scope.h)
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

add_executable(ConfigTest test/config_test.cpp
        src/config/config.h
        src/config/config.cpp
        src/config/config_enum.h
        src/config/config_enum.cpp
        src/util/macro_scope.h)
add_dependencies(ConfigTest nlohmann_json fmt)
target_link_libraries(ConfigTest PUBLIC nlohmann_json fmt)
target_include_directories(ConfigTest PRIVATE src)
target_include_directories(ConfigTest PUBLIC packages/header-only)
