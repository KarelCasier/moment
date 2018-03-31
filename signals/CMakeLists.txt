project(signals LANGUAGES CXX)

add_library(signals_lib INTERFACE)

target_include_directories(signals_lib
    INTERFACE
    include
    )

set (EXEC_SOURCES
    src/main.cpp
    )

add_executable(signals ${EXEC_SOURCES})

target_link_libraries(signals
    signals_lib
    )

