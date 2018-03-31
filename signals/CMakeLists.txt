project(signals LANGUAGES CXX)

#set(LIB_HEADERS
    #include/signals/.hpp
    #)
#
#set(LIB_SOURCES
    #src/.cpp
    #)
#
#add_library(signals_lib STATIC ${LIB_SOURCES} ${LIB_HEADERS})
#
#target_include_directories(signals_lib
    #PUBLIC
    #include
    #PRIVATE
    #)

set (EXEC_SOURCES
    src/main.cpp
    )

add_executable(signals ${EXEC_SOURCES})

#target_link_libraries(signals
    #signals_lib
    #)
#
