cmake_minimum_required(VERSION 4.0)
project(CPP_OpenGL_Tutorial)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

add_compile_definitions(GLEW_STATIC)

link_directories(lib)
set(OpenGLLibs glfw3 opengl32)

include_directories(include)

add_executable(CPP_OpenGL_Tutorial
        src/main.cpp
        src/glad.c
        src/stb.cpp
        src/io/Joystick.cpp
        src/io/Joystick.h
        src/io/mouse.cpp
        src/io/mouse.h
        src/io/keyboard.cpp
        src/io/keyboard.h
        src/rendering/Shader.cpp
        src/rendering/Shader.h
        src/rendering/Camera.cpp
        src/rendering/Camera.h)

target_link_libraries(CPP_OpenGL_Tutorial ${OpenGLLibs})