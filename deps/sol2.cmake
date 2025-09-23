include(FetchContent)
include(deps/lua.cmake)

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2
    GIT_TAG 9190880c593dfb018ccf5cc9729ab87739709862
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(sol2)

add_library(sol2_interface INTERFACE)

target_include_directories(sol2_interface INTERFACE
    ${sol2_SOURCE_DIR}/include
)
target_link_libraries(sol2_interface INTERFACE lua_static)