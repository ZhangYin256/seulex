# CMakeLists.txt for SeuLex

# 要求CMake最低版本
# 项目名称、版本、使用的编程语言
cmake_minimum_required(VERSION 3.14)
project(SeuLex VERSION 1.0 LANGUAGES CXX)

# 设置C++标准为C++23、要求必须支持，并启用编译命令导出
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SEULEX_ENABLE_COVERAGE "Enable coverage instrumentation" OFF)
if (SEULEX_ENABLE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O0 -g --coverage)
        add_link_options(--coverage)
    else()
        message(FATAL_ERROR "Coverage is only supported with GNU/Clang compilers")
    endif()
endif()

# 核心库源码（不包含 main）
set(SEULEX_CORE_SOURCES
    src/automata/dfa_matcher.cpp
    src/automata/dfa_minimizer.cpp
    src/codegen/scanner_generator.cpp
    src/automata/nfa_builder.cpp
    src/automata/subset_construction.cpp
    src/automata/nfa_visualizer.cpp
    src/config/config.cpp
    src/parser/flex_regex_converter.cpp
    src/parser/lex_input_scanner.cpp
    src/parser/regex_parser.cpp
)

add_library(seulex_core ${SEULEX_CORE_SOURCES})
target_include_directories(seulex_core PUBLIC src)

# 主程序
add_executable(SeuLex src/main.cpp)
target_link_libraries(SeuLex PRIVATE seulex_core)

# 测试
option(SEULEX_BUILD_TESTS "Build SeuLex tests" ON)
if (SEULEX_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_executable(seulex_tests
            tests/flex_regex_converter_test.cpp
            tests/regex_parser_test.cpp
            tests/config_test.cpp
            tests/automata_pipeline_test.cpp
            tests/scanner_generator_test.cpp
    )
    target_link_libraries(seulex_tests PRIVATE seulex_core GTest::gtest_main)
    target_include_directories(seulex_tests PRIVATE src)

    gtest_discover_tests(seulex_tests PROPERTIES LABELS "unit")

    find_program(BISON_EXECUTABLE bison)
    find_program(CC_EXECUTABLE cc)
    if (BISON_EXECUTABLE AND CC_EXECUTABLE)
        add_test(
            NAME e2e_generate_simple_scanner
            COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/run_generate_simple.sh
                    $<TARGET_FILE:SeuLex>
                    ${CMAKE_SOURCE_DIR}
        )
        set_tests_properties(e2e_generate_simple_scanner PROPERTIES LABELS "e2e")

        add_test(
            NAME e2e_bison_parser_integration
            COMMAND ${CMAKE_SOURCE_DIR}/tests/e2e/run_bison_integration.sh
                    $<TARGET_FILE:SeuLex>
                    ${CMAKE_SOURCE_DIR}
                    ${BISON_EXECUTABLE}
                    ${CC_EXECUTABLE}
        )
        set_tests_properties(e2e_bison_parser_integration PROPERTIES LABELS "e2e")
    else()
        message(WARNING "Skipping e2e parser integration tests: bison/cc not found")
    endif()

    find_program(GCOVR_EXECUTABLE gcovr)
    if (GCOVR_EXECUTABLE)
        find_program(GCOV_EXECUTABLE gcov-14)
        if (NOT GCOV_EXECUTABLE)
            find_program(GCOV_EXECUTABLE gcov)
        endif()
        if (NOT GCOV_EXECUTABLE)
            message(FATAL_ERROR "Coverage requested but neither gcov-14 nor gcov was found")
        endif()

        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${GCOVR_EXECUTABLE}
                    -r ${CMAKE_SOURCE_DIR}
                    --filter ${CMAKE_SOURCE_DIR}/src
                    --exclude ${CMAKE_SOURCE_DIR}/tests
                    --object-directory ${CMAKE_BINARY_DIR}
                    --gcov-executable ${GCOV_EXECUTABLE}
                    --print-summary
                    --txt ${CMAKE_BINARY_DIR}/coverage/coverage-summary.txt
                    --html-details ${CMAKE_BINARY_DIR}/coverage/coverage.html
                    --xml ${CMAKE_BINARY_DIR}/coverage/coverage.xml
                --json ${CMAKE_BINARY_DIR}/coverage/coverage.json
            COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/scripts/coverage_by_module.py
                ${CMAKE_BINARY_DIR}/coverage/coverage.json
                ${CMAKE_SOURCE_DIR}
                ${CMAKE_BINARY_DIR}/coverage/module-coverage.md
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS seulex_tests SeuLex
            COMMENT "Running tests and generating coverage reports"
        )
    else()
        message(WARNING "gcovr not found, coverage target will not be available")
    endif()
endif()