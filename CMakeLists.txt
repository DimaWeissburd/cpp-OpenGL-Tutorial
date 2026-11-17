
cmake_minimum_required(VERSION 3.20)
project(CPP_OpenGL_Tutorial)
 
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.c)
add_executable(CPP_OpenGL_Tutorial ${SOURCES})
 
target_compile_features(CPP_OpenGL_Tutorial PRIVATE cxx_std_20)
 
target_include_directories(CPP_OpenGL_Tutorial PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Linking/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
 
target_link_directories(CPP_OpenGL_Tutorial PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Linking/lib"
)
 
# Assimp: debug and release builds are different libraries/DLLs.
# Adjust the release names if yours differ.
set(ASSIMP_LIB_DEBUG   assimp-vc145-mtd)
set(ASSIMP_LIB_RELEASE assimp-vc145-mt)
 
target_link_libraries(CPP_OpenGL_Tutorial PRIVATE
    glfw3
    opengl32
    freetype
    $<IF:$<CONFIG:Debug>,${ASSIMP_LIB_DEBUG},${ASSIMP_LIB_RELEASE}>
)
 
# Warnings
if(MSVC)
    target_compile_options(CPP_OpenGL_Tutorial PRIVATE /W4)
else()
    target_compile_options(CPP_OpenGL_Tutorial PRIVATE -Wall -Wextra)
endif()
 
# Copy runtime files next to the executable after every build:
# assets folder, settings file, and the DLLs (only needed for dynamic builds).
add_custom_command(TARGET CPP_OpenGL_Tutorial POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
            "$<TARGET_FILE_DIR:CPP_OpenGL_Tutorial>/Assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Settings.ini"
            "$<IF:$<CONFIG:Debug>,${CMAKE_CURRENT_SOURCE_DIR}/Linking/lib/${ASSIMP_LIB_DEBUG}.dll,${CMAKE_CURRENT_SOURCE_DIR}/Linking/lib/${ASSIMP_LIB_RELEASE}.dll>"
            "${CMAKE_CURRENT_SOURCE_DIR}/Linking/lib/freetype.dll"
            "$<TARGET_FILE_DIR:CPP_OpenGL_Tutorial>"
    COMMENT "Copying assets, settings and DLLs"
)
 
if(MSVC)
    # Debug keeps the console window; other configs run as a GUI app.
    target_link_options(CPP_OpenGL_Tutorial PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS;/ENTRY:mainCRTStartup>
    )
 
    # Make Visual Studio run from the exe folder so Settings.ini and Assets are found.
    set_target_properties(CPP_OpenGL_Tutorial PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:CPP_OpenGL_Tutorial>"
    )
endif()
 