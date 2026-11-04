add_executable(can_simulator
    main.cpp)

target_link_libraries(can_simulator PRIVATE candash_common)
