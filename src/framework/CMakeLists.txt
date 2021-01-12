cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE FRM_SRC_FILES
     "*.cpp"
     "*.cxx"
     "*.c")

file(GLOB_RECURSE FRM_INC_FILES
     "*.hpp"
     "*.h")

add_library(frm ${FRM_SRC_FILES} ${FRM_INC_FILES})
target_include_directories(frm PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include/")
target_link_libraries(frm
                      PUBLIC Vulkan::Vulkan
                      PUBLIC SDL2::SDL2-static
                      PUBLIC glm::glm)
target_precompile_headers(frm
                          PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/../include/framework/Common.h>")
