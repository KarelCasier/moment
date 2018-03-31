project(moment LANGUAGES CXX)

add_library(moment_lib INTERFACE)

target_include_directories(moment_lib
    INTERFACE
    include
    )

set (EXEC_SOURCES
    src/main.cpp
    )

add_executable(moment ${EXEC_SOURCES})

target_link_libraries(moment
    moment_lib
    )

