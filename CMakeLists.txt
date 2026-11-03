cmake_minimum_required(VERSION 3.20)
project(CPP_OpenGL_Tutorial)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.c)
add_executable(CPP_OpenGL_Tutorial ${SOURCES})

target_include_directories(CPP_OpenGL_Tutorial PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Linking/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_directories(CPP_OpenGL_Tutorial PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Linking/lib")

set(OpenGLLibs glfw3 opengl32 assimp-vc145-mtd)
target_link_libraries(CPP_OpenGL_Tutorial ${OpenGLLibs})

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets"
            "$<TARGET_FILE_DIR:CPP_OpenGL_Tutorial>/Assets"
    COMMENT "Copying assets"
)
add_dependencies(CPP_OpenGL_Tutorial copy_assets)

# Copy Assimp DLL post-build (only needed for dynamic builds)
add_custom_command(TARGET CPP_OpenGL_Tutorial POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/Linking/lib/assimp-vc145-mtd.dll"
    "$<TARGET_FILE_DIR:CPP_OpenGL_Tutorial>"
)

# MSVC Subsystem configuration for Windows GUI/Console apps
if(MSVC)
    set_target_properties(CPP_OpenGL_Tutorial PROPERTIES 
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()