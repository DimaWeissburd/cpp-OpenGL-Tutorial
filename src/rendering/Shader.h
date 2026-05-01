#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    unsigned int id;
    Shader();
    Shader(const char* vertexShaderPath, const char* fragmentShaderPath);

    void generate(const char* vertexShaderPath, const char* fragmentShaderPath);

    void activate();

    std::string loadShaderSrc(const char* filepath);
    GLuint compileShader(const char* filepath, GLenum type);

    void setBool(const std::string& name, bool value);
    void setInt(const std::string &name, int value);
    void setFloat(const std::string &name, float value);
    void set4Float(const std::string &name, float v0, float v1, float v2, float v3);
    void setMat4(const std::string &name, glm::mat4 val);
};

#endif