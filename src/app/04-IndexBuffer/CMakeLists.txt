cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE APP_SRC_FILES
     "*.cpp"
     "*.cxx"
     "*.c")

file(GLOB_RECURSE APP_INC_FILES
     "*.hpp"
     "*.h")

add_executable(04-IndexBuffer ${APP_SRC_FILES} ${APP_INC_FILES})
target_link_libraries(04-IndexBuffer PRIVATE frm)

add_resource(04-IndexBuffer-Res)
target_resource_shader(04-IndexBuffer-Res "VertexShader.vs" vert)
target_resource_shader(04-IndexBuffer-Res "FragShader.fs" frag)
