# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 3.8)

include("cmake/Extra.cmake")

if (WIN32)
    list(APPEND CMAKE_PREFIX_PATH "deps/SDL2")
endif ()

set(glm_DIR "deps/glm/cmake/glm")

project("vulkan-learn")

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)

add_subdirectory("src/framework")
add_subdirectory("src/app/01-HelloTriangle")
add_subdirectory("src/app/02-PushConstant")
add_subdirectory("src/app/03-VertexBuffer")
add_subdirectory("src/app/04-IndexBuffer")
