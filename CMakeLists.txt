cmake_minimum_required(VERSION 3.29)

# Project name
project(OpenGL)

# C++ standard
set(CMAKE_CXX_STANDARD 20)

# Executable
add_executable(OpenGL main.cpp
        "main.cpp")

# Find and link OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(OpenGL PRIVATE OpenGL::GL)
target_link_libraries(OpenGL PRIVATE GL GLU glut)

# Find and link assimp
find_package(assimp REQUIRED)
target_link_libraries(OpenGL PRIVATE assimp::assimp)

# Optional: Link GLFW or other libraries (e.g., GLEW or GLAD)
# Uncomment and modify as needed
find_package(glfw3 REQUIRED)
target_link_libraries(OpenGL PRIVATE glfw)
