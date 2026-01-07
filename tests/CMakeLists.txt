cmake_minimum_required(VERSION 3.13...3.21)

project(C++RogueLike_Tests)

# Set C++ standard to match main project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(GTest CONFIG REQUIRED)
find_package(unofficial-pdcurses CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(libtcod CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Include directories from main project
include_directories(
    ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/../src
)

# Collect all test source files
file(GLOB_RECURSE TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/**/*.cpp"
)

# Get source files from parent project (exclude main.cpp)
set(PARENT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../src")
file(GLOB_RECURSE PARENT_SOURCES
    "${PARENT_SOURCE_DIR}/**/*.cpp"
)
list(FILTER PARENT_SOURCES EXCLUDE REGEX ".*main_C\\+\\+RogueLike\\.cpp$")

# Create test executable with both test sources and parent sources
add_executable(run_tests ${TEST_SOURCES} ${PARENT_SOURCES})

# Link against Google Test and required libraries
target_link_libraries(run_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        unofficial::pdcurses::pdcurses
        SDL3::SDL3
        libtcod::libtcod
        nlohmann_json::nlohmann_json
)

# For MSVC, add compile options
if(MSVC)
    target_compile_options(run_tests PRIVATE /W3 /utf-8)
    set_target_properties(run_tests PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:run_tests>")
endif()

# Copy JSON files to test binary directory
set(JSON_FILES
    "${PARENT_SOURCE_DIR}/json/strength.json"
    "${PARENT_SOURCE_DIR}/json/dexterity.json"
    "${PARENT_SOURCE_DIR}/json/constitution.json"
    "${PARENT_SOURCE_DIR}/json/wisdom.json"
    "${PARENT_SOURCE_DIR}/json/intelligence.json"
    "${PARENT_SOURCE_DIR}/json/charisma.json"
)

foreach(JSON_FILE ${JSON_FILES})
    get_filename_component(JSON_FILENAME ${JSON_FILE} NAME)
    add_custom_command(TARGET run_tests POST_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different
                              ${JSON_FILE}
                              $<TARGET_FILE_DIR:run_tests>/${JSON_FILENAME})
endforeach()

# Discover and register tests
include(GoogleTest)
gtest_discover_tests(run_tests)

# Add custom target to run tests
add_custom_target(test_run
    COMMAND run_tests
    DEPENDS run_tests
    WORKING_DIRECTORY $<TARGET_FILE_DIR:run_tests>
    COMMENT "Running unit tests"
)
