#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    unsigned int id;
    Shader();
    Shader(bool includeDefaultHeader, const char* vertexShaderPath, const char* fragmentShaderPath, const char* geometryShaderPath = nullptr);

    void generate(bool includeDefaultHeader, const char* vertexShaderPath, const char* fragmentShaderPath, const char* geometryShaderPath = nullptr);

    void activate();

    GLuint compileShader(bool includeDefaultHeader, const char* filePath, GLenum type);

    void setBool(const std::string& name, bool value);
    void setInt(const std::string &name, int value);
    void setFloat(const std::string &name, float value);
    void set3Float(const std::string &name, glm::vec3 value);
    void set3Float(const std::string &name, float v0, float v1, float v2);
    void set4Float(const std::string &name, float v0, float v1, float v2, float v3);
    void set4Float(const std::string& name, aiColor4D color);
    void set4Float(const std::string& name, glm::vec4 v);
    void setMat4(const std::string &name, glm::mat4 val);

    static std::string defaultDirectory;

    static std::stringstream defaultHeaders;

    static void loadIntoDefault(const char* filePath);

    static void clearDefault();

    static std::string loadShaderSrc(bool includeDefaultHeader, const char* filePath);
};

#endif