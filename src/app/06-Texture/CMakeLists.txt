cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE APP_SRC_FILES
     "*.cpp"
     "*.cxx"
     "*.c")

file(GLOB_RECURSE APP_INC_FILES
     "*.hpp"
     "*.h")

add_executable(06-Texture ${APP_SRC_FILES} ${APP_INC_FILES})
target_link_libraries(06-Texture PRIVATE frm)

add_resource(06-Texture-Res)
target_resource_shader(06-Texture-Res "VertexShader.vs" vert)
target_resource_shader(06-Texture-Res "FragShader.fs" frag)
target_resource_file(06-Texture-Res "shaderboi_fish.png")
