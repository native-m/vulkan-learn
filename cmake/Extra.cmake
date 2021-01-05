function(add_resource _TARGET)
    add_custom_target(${_TARGET} ALL)
endfunction()

function(target_resource_shader _TARGET _SRC_FILE _STAGE)
    get_filename_component(_SRC_FILE_NAME ${_SRC_FILE} NAME)
    get_filename_component(_SRC_FILE_PATH ${_SRC_FILE} REALPATH)
    get_target_property(_BINDIR ${_TARGET} BINARY_DIR)
    add_custom_command(TARGET ${_TARGET}
                       COMMAND glslc --target-env=vulkan1.0 -fshader-stage=${_STAGE} -o ${_BINDIR}/${_SRC_FILE_NAME}.spv ${_SRC_FILE_PATH}
                       BYPRODUCTS ${_BINDIR}/${_SRC_FILE_NAME}.spv)
endfunction()

function(target_resource_file _TARGET _FILE)
endfunction()
