cmake_minimum_required(VERSION 4.0)
project(CPP_OpenGL_Tutorial)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

add_compile_definitions(GLEW_STATIC)

add_executable(
    CPP_OpenGL_Tutorial
    src/main.cpp
    src/glad.c
    src/stb.cpp
    src/io/Joystick.cpp
    src/io/Joystick.h
    src/io/Mouse.cpp
    src/io/Mouse.h
    src/io/Keyboard.cpp
    src/io/Keyboard.h
    src/rendering/Shader.cpp
    src/rendering/Shader.h
    src/rendering/Camera.cpp
    src/rendering/Camera.h
    src/rendering/Screen.cpp
    src/rendering/Screen.h
    src/rendering/Texture.h
    src/rendering/Texture.cpp
    src/rendering/Mesh.h
    src/rendering/Mesh.cpp
    src/rendering/Model.h
    src/rendering/Model.cpp
    src/rendering/Models/Cube.hpp
    src/rendering/Material.h
    src/rendering/Material.cpp
    src/rendering/Models/Lamp.hpp
    src/rendering/Light.cpp
    src/rendering/Light.h
)

target_include_directories(CPP_OpenGL_Tutorial PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Linking/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_directories(CPP_OpenGL_Tutorial PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Linking/lib")

set(OpenGLLibs glfw3 opengl32 assimp-vc145-mtd)

target_link_libraries(CPP_OpenGL_Tutorial ${OpenGLLibs})

add_custom_command(TARGET CPP_OpenGL_Tutorial POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:CPP_OpenGL_Tutorial>/assets"
)

if(MSVC)
    set_target_properties(CPP_OpenGL_Tutorial PROPERTIES 
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()