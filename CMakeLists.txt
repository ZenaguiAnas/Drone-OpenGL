cmake_minimum_required(VERSION 3.29)

# Project name
project(OpenGL)

# C++ standard
set(CMAKE_CXX_STANDARD 20)

# Executable
add_executable(OpenGL main.cpp)

# Include directories for custom headers
target_include_directories(OpenGL PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Find and link OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(OpenGL PRIVATE OpenGL::GL)

# Find and link Assimp
find_package(assimp REQUIRED)
target_link_libraries(OpenGL PRIVATE assimp::assimp)

# Find and link GLFW
find_package(glfw3 REQUIRED)
target_link_libraries(OpenGL PRIVATE glfw)

# Find and link GLUT
find_package(GLUT REQUIRED)
target_link_libraries(OpenGL PRIVATE GLUT::GLUT)

# Link AntTweakBar
target_link_libraries(OpenGL PRIVATE AntTweakBar)

# Link GLU
find_library(GLU_LIBRARY GLU REQUIRED)
target_link_libraries(OpenGL PRIVATE ${GLU_LIBRARY})
