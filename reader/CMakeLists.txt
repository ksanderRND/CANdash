add_executable(can_reader
    main.cpp)

target_link_libraries(can_reader PRIVATE candash_common)
