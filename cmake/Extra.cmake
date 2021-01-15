function(add_resource _TARGET)
    add_custom_target(${_TARGET} ALL)
endfunction()

function(target_resource_shader _TARGET _SRC_FILE _STAGE)
    get_filename_component(_SRC_FILE_NAME ${_SRC_FILE} NAME)
    get_filename_component(_SRC_FILE_PATH ${_SRC_FILE} REALPATH)
    get_target_property(_BINDIR ${_TARGET} BINARY_DIR)
    add_custom_command(TARGET ${_TARGET}
                       COMMAND glslangValidator --target-env vulkan1.0 -S ${_STAGE} -o ${_BINDIR}/${_SRC_FILE_NAME}.spv ${_SRC_FILE_PATH}
                       BYPRODUCTS ${_BINDIR}/${_SRC_FILE_NAME}.spv)
endfunction()

function(target_resource_file _TARGET _SRC_FILES)
    get_target_property(_BINDIR ${_TARGET} BINARY_DIR)
    foreach(_FILE ${_SRC_FILES})
        get_filename_component(_FILE_NAME ${_FILE} NAME)
        get_filename_component(_FILE_PATH ${_FILE} REALPATH)
        add_custom_command(TARGET ${_TARGET}
                           COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_FILE_PATH} ${_BINDIR}/${_FILE_NAME}
                           BYPRODUCTS ${_BINDIR}/${_FILE_NAME})
    endforeach()
endfunction()
