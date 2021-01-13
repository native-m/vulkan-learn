cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE APP_SRC_FILES
     "*.cpp"
     "*.cxx"
     "*.c")

file(GLOB_RECURSE APP_INC_FILES
     "*.hpp"
     "*.h")

add_executable(05-Transform ${APP_SRC_FILES} ${APP_INC_FILES})
target_link_libraries(05-Transform PRIVATE frm)

add_resource(05-Transform-Res)
target_resource_shader(05-Transform-Res "VertexShader.vs" vert)
target_resource_shader(05-Transform-Res "FragShader.fs" frag)
