cmake_minimum_required(VERSION 4.0)
project(CPP_OpenGL_Tutorial)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(GLEW_STATIC)

link_directories(lib)
set(OpenGLLibs glfw3 opengl32)

include_directories(include)

add_executable(CPP_OpenGL_Tutorial
        src/main.cpp
        src/glad.c
        src/stb.cpp
        src/Shader.cpp
        src/Shader.h)

target_link_libraries(CPP_OpenGL_Tutorial ${OpenGLLibs})