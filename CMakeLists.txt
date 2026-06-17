cmake_minimum_required(VERSION 3.10)
project(mini_backup_system)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(server shared)

enable_testing()

find_package(Threads REQUIRED)

add_executable(backup_server
    server/main.cpp
    server/network.cpp
    server/server_logic.cpp
    shared/file_utils.c
)
target_link_libraries(backup_server PRIVATE Threads::Threads)

add_executable(backup_client
    client_desktop/main.cpp
    shared/file_utils.c
)

add_executable(server_test
    tests/server_test.c
    shared/file_utils.c
)

add_subdirectory(tests)
