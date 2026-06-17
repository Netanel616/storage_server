cmake_minimum_required(VERSION 3.10)
project(tests)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(../server ../shared)

add_executable(network_test
    network_test.c
    ../server/network.cpp
    ../shared/file_utils.c
)

add_executable(server_logic_test
    server_logic_test.c
    ../server/server_logic.cpp
    ../server/network.cpp
    ../shared/file_utils.c
)

add_executable(protocol_test protocol_test.c)

add_executable(main_test
    main_test.c
    ../server/network.cpp
    ../server/server_logic.cpp
    ../shared/file_utils.c
)

add_executable(file_utils_test
    file_utils_test.c
    ../shared/file_utils.c
    ../server/network.cpp
)

add_test(NAME network_test COMMAND network_test)
add_test(NAME server_logic_test COMMAND server_logic_test)
add_test(NAME protocol_test COMMAND protocol_test)
add_test(NAME main_test COMMAND main_test)
add_test(NAME file_utils_test COMMAND file_utils_test)
