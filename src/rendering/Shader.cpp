#include "../rendering/Shader.h"

Shader::Shader() {}
Shader::Shader(const char* vertexShaderPath, const char* fragmentShaderPath) {
    generate(vertexShaderPath, fragmentShaderPath);
}

void Shader::generate(const char* vertexShaderPath, const char* fragmentShaderPath) {
    int success;
    char infoLog[512];

    GLuint vertexShader = compileShader(vertexShaderPath, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderPath, GL_FRAGMENT_SHADER);

    id = glCreateProgram();
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    glLinkProgram(id);

    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 512, NULL, infoLog);
        std::cout << "Error linking program: " << std::endl << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::activate() {
    glUseProgram(id);
}

std::string Shader::loadShaderSrc(const char* filename) {
    std::ifstream file;
    std::stringstream buf;
    std::string ret = "";
    file.open(filename);
    if (file.is_open()) {
        buf << file.rdbuf();
        ret = buf.str();
    } else {
        std::cout << "Failed to open " << filename << std::endl;
    }
    file.close();
    return ret;
}

GLuint Shader::compileShader(const char* filepath, GLenum type) {
    int success;
    char infoLog[512];

    GLuint shader = glCreateShader(type);
    std::string shaderSrc = loadShaderSrc(filepath);
    const GLchar* src = shaderSrc.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader compilation failed: " << std::endl << infoLog << std::endl;
    }

    return shader;
}

void Shader::setBool(const std::string& name, bool value) {
    glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) {
    glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) {
    glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}

void Shader::set3Float(const std::string &name, glm::vec3 value) {
    set3Float(name, value.x, value.y, value.z);
    glUniform3f(glGetUniformLocation(id, name.c_str()), value[0], value[1], value[2]);
}

void Shader::set3Float(const std::string &name, float v0, float v1, float v2) {
    glUniform3f(glGetUniformLocation(id, name.c_str()), v0, v1, v2);
}

void Shader::set4Float(const std::string& name, float v1, float v2, float v3, float v4) {
    glUniform4f(glGetUniformLocation(id, name.c_str()), v1, v2, v3, v4);
}

void Shader::setMat4(const std::string &name, glm::mat4 val) {
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, false, glm::value_ptr(val));
}
