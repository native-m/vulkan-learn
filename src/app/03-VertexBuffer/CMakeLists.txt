cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE APP_SRC_FILES
     "*.cpp"
     "*.cxx"
     "*.c")

file(GLOB_RECURSE APP_INC_FILES
     "*.hpp"
     "*.h")

add_executable(03-VertexBuffer ${APP_SRC_FILES} ${APP_INC_FILES})
target_link_libraries(03-VertexBuffer PRIVATE frm)

add_resource(03-VertexBuffer-Res)
target_resource_shader(03-VertexBuffer-Res "VertexShader.vs" vert)
target_resource_shader(03-VertexBuffer-Res "FragShader.fs" frag)
