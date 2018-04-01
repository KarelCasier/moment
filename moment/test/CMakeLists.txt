set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/test)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

add_executable(moment_tests test_moment.cpp)

add_test(NAME moment_tests COMMAND $<TARGET_FILE:moment_tests>)
target_link_libraries(moment_tests moment_lib gtest gmock)
