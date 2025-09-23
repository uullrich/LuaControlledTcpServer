FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.6
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(lua)

file(GLOB LUA_SOURCES CONFIGURE_DEPENDS "${lua_SOURCE_DIR}/*.c")
file(GLOB LUA_HEADERS CONFIGURE_DEPENDS "${lua_SOURCE_DIR}/*.h")

list(REMOVE_ITEM LUA_SOURCES 
    "${lua_SOURCE_DIR}/lua.c"
    "${lua_SOURCE_DIR}/luac.c"
)

add_library(lua_static STATIC ${LUA_SOURCES} ${LUA_HEADERS})

target_include_directories(lua_static INTERFACE
    ${lua_SOURCE_DIR}
)

target_compile_features(lua_static PRIVATE c_std_99)