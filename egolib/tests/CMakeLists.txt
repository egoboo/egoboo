cmake_minimum_required (VERSION 2.8.7)

project(EgoLibTests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -Wall")
#-fsanitize=address -fsanitize=leak -fsanitize=undefined

# Add gtest library include
include_directories("${PROJECT_SOURCE_DIR}/external/googletest/googletest/include")

# Build a list of all tests
file(GLOB_RECURSE TEST_FILES ${PROJECT_SOURCE_DIR}/egolib/Tests/*.cpp)

#Setup include directories
include_directories(${CMAKE_SOURCE_DIR}/idlib/src)
include_directories(${PROJECT_SOURCE_DIR})

# For each test, create an executable
foreach(test_file ${TEST_FILES})
    get_filename_component(UNIT_TEST ${test_file} NAME_WE)
    get_filename_component(testWorkingDir ${test_file} DIRECTORY)
    add_executable(${UNIT_TEST} ${test_file})
    target_link_libraries(${UNIT_TEST} gtest gtest_main egolib)
    add_test(NAME ${UNIT_TEST} COMMAND ${UNIT_TEST} WORKING_DIRECTORY ${testWorkingDir})

    #Execute test after building
    add_custom_command(
            TARGET ${UNIT_TEST}
            COMMENT "Running test: ${UNIT_TEST}"
            POST_BUILD
            COMMAND ${UNIT_TEST}
    )
endforeach()
