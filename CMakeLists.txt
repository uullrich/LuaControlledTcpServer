cmake_minimum_required(VERSION 3.12)
project(LuaControlledTcpServer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(deps/asio.cmake)
include(deps/sol2.cmake)
include(cmake/lua_script_operations.cmake)

add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/can/CanMessage.cpp
    src/tcp/TcpServer.cpp
    src/lua/LuaBinding.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE sol2_interface asio_interface)

copy_lua_scripts_to_target(${PROJECT_NAME})