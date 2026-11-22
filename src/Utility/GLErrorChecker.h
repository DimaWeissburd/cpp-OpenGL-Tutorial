#pragma once

#include <iostream>
#include <string>

// NOTE: include your OpenGL loader BEFORE this header
// (e.g. <glad/glad.h> or <GL/glew.h>), so GLenum / glGetError are available.

class GLErrorChecker {
public:
    // Drains the GL error queue and prints every error found.
    // Returns true if at least one error was reported.
    static bool check(const char* where, std::ostream& out = std::cerr) {
        bool hadError = false;
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            hadError = true;
            out << "GL error 0x" << std::hex << err << std::dec
                << " (" << errorToString(err) << ")"
                << " at " << where << std::endl;
        }
        return hadError;
    }

    // Same as above, with file/line info (used by the GL_CHECK macro).
    static bool check(const char* file, int line, const char* where,
        std::ostream& out = std::cerr) {
        std::string location = std::string(where) + " [" + file + ":" +
            std::to_string(line) + "]";
        return check(location.c_str(), out);
    }

    // Human-readable name for a GL error code.
    static const char* errorToString(GLenum err) {
        switch (err) {
        case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
        default:                               return "UNKNOWN_ERROR";
        }
    }

    GLErrorChecker() = delete; // static-only utility class
};

// Convenience macro: captures file and line automatically.
// Compiled out in release builds (NDEBUG).
#ifndef NDEBUG
#define GL_CHECK(where) GLErrorChecker::check(__FILE__, __LINE__, where)
#else
#define GL_CHECK(where) ((void)0)
#endif
