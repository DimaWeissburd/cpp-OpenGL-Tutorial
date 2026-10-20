cmake_minimum_required(VERSION 4.0)
project(CPP_OpenGL_Tutorial)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

add_compile_definitions(GLEW_STATIC)

add_executable(
    CPP_OpenGL_Tutorial
    src/main.cpp
    src/Scene.cpp
    src/Scene.h
    src/glad.c
    src/stb.cpp
    src/Algorithms/Bounds.cpp
    src/Algorithms/Bounds.h
    src/Algorithms/States.hpp
    src/IO/Joystick.cpp
    src/IO/Joystick.h
    src/IO/Keyboard.cpp
    src/io/Keyboard.h
    src/IO/Mouse.cpp
    src/IO/Mouse.h
    src/Physics/Environment.cpp
    src/Physics/Environment.h
    src/Physics/RigidBody.cpp
    src/Physics/RigidBody.h
    src/Rendering/Models/Box.hpp
    src/Rendering/Models/Cube.hpp
    src/Rendering/Models/HandHeld.hpp
    src/Rendering/Models/Lamp.hpp
    src/Rendering/Models/ModelArray.hpp
    src/Rendering/Models/Sphere.hpp
    src/Rendering/Camera.cpp
    src/Rendering/Camera.h
    src/Rendering/GLMemory.hpp
    src/Rendering/Light.cpp
    src/Rendering/Light.h
    src/Rendering/Material.h
    src/Rendering/Material.cpp
    src/Rendering/Mesh.h
    src/Rendering/Mesh.cpp
    src/Rendering/Model.h
    src/Rendering/Model.cpp
    src/Rendering/Shader.cpp
    src/Rendering/Shader.h
    src/Rendering/Texture.h
    src/Rendering/Texture.cpp
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

add_custom_command(TARGET CPP_OpenGL_Tutorial POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/Linking/lib/assimp-vc145-mtd.dll"
    "$<TARGET_FILE_DIR:CPP_OpenGL_Tutorial>"
)

if(MSVC)
    set_target_properties(CPP_OpenGL_Tutorial PROPERTIES 
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()