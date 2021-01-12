cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE APP_SRC_FILES
     "*.cpp"
     "*.cxx"
     "*.c")

file(GLOB_RECURSE APP_INC_FILES
     "*.hpp"
     "*.h")

add_executable(01-HelloTriangle ${APP_SRC_FILES} ${APP_INC_FILES})
target_link_libraries(01-HelloTriangle PRIVATE frm)

add_resource(01-HelloTriangle-Res)
target_resource_shader(01-HelloTriangle-Res "VertexShader.vs" vert)
target_resource_shader(01-HelloTriangle-Res "FragShader.fs" frag)
