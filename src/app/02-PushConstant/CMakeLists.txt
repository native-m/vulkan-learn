cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE APP_SRC_FILES
     "*.cpp"
     "*.cxx"
     "*.c")

file(GLOB_RECURSE APP_INC_FILES
     "*.hpp"
     "*.h")

add_executable(02-PushConstant ${APP_SRC_FILES} ${APP_INC_FILES})
target_link_libraries(02-PushConstant PRIVATE frm)

add_resource(02-PushConstant-Res)
target_resource_shader(02-PushConstant-Res "VertexShader.vs" vert)
target_resource_shader(02-PushConstant-Res "FragShader.fs" frag)
