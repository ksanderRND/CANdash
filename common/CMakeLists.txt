add_library(candash_common STATIC
    DbcParser.cpp)

target_include_directories(candash_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
