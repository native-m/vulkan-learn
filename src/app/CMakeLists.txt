cmake_minimum_required(VERSION 3.8)

file(GLOB_RECURSE APP_SRC_FILES
     "*.cpp"
     "*.cxx"
     "*.c")

file(GLOB_RECURSE APP_INC_FILES
     "*.hpp"
     "*.h")

add_executable(app ${APP_SRC_FILES} ${APP_INC_FILES})
target_link_libraries(app PRIVATE frm)

add_resource(app_resource)
target_resource_shader(app_resource "VertexShader.vs" vert)
target_resource_shader(app_resource "FragShader.fs" frag)
