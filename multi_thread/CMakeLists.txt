cmake_minimum_required(VERSION 3.14)
project(raytracer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Dawn package
find_package(Dawn REQUIRED)

# Add source files
add_executable(raytracer 
    src/main.cc
)

# Include directories
target_include_directories(raytracer PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link Dawn libraries
target_link_libraries(raytracer PRIVATE 
    Dawn::dawn
    Dawn::dawn_native
    Dawn::dawn_platform
)
