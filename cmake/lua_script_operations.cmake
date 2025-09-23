function(copy_lua_scripts_to_target target_name)
    file(GLOB LUA_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.lua")
    
    if(LUA_SCRIPTS)
        add_custom_target(copy_lua_scripts ALL
            COMMENT "Copying Lua scripts to build directory"
        )
        
        add_custom_command(TARGET copy_lua_scripts PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/scripts
        )

        foreach(SCRIPT ${LUA_SCRIPTS})
            get_filename_component(SCRIPT_NAME ${SCRIPT} NAME)
            add_custom_command(TARGET copy_lua_scripts PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                ${SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/scripts/${SCRIPT_NAME}
                COMMENT "Copying ${SCRIPT_NAME}"
            )
        endforeach()
        
        # Make the target depend on script copying
        add_dependencies(${target_name} copy_lua_scripts)
    endif()
endfunction()