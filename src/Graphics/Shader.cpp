#include "../Graphics/Shader.h"

Shader::Shader() {}
Shader::Shader(bool includeDefaultHeader, const char* vertexShaderPath, const char* fragmentShaderPath, const char* geometryShaderPath) {
    generate(includeDefaultHeader, vertexShaderPath, fragmentShaderPath, geometryShaderPath);
}

void Shader::generate(bool includeDefaultHeader, const char* vertexShaderPath, const char* fragmentShaderPath, const char* geometryShaderPath) {
    int success;
    char infoLog[512];

    GLuint vertexShader = compileShader(includeDefaultHeader, vertexShaderPath, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(includeDefaultHeader, fragmentShaderPath, GL_FRAGMENT_SHADER);

    GLuint geometryShader = 0;
    if (geometryShaderPath) {
        geometryShader = compileShader(includeDefaultHeader, geometryShaderPath, GL_GEOMETRY_SHADER);
    }

    id = glCreateProgram();
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    if (geometryShaderPath) {
        glAttachShader(id, geometryShader);
    }
    glLinkProgram(id);

    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 512, NULL, infoLog);
        std::cout << "Error linking program: " << std::endl << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (geometryShaderPath) {
        glDeleteShader(geometryShader);
    }
}

void Shader::activate() {
    glUseProgram(id);
}

GLuint Shader::compileShader(bool includeDefaultHeader, const char* filePath, GLenum type) {
    int success;
    char infoLog[512];

    GLuint shader = glCreateShader(type);
    std::string shaderSrc = loadShaderSrc(includeDefaultHeader, filePath);
    const GLchar* src = shaderSrc.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader compilation failed for " << filePath << ":" << std::endl << infoLog << std::endl;
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

void Shader::set4Float(const std::string& name, aiColor4D color) {
    glUniform4f(glGetUniformLocation(id, name.c_str()), color.r, color.g, color.b, color.a);
}

void Shader::set4Float(const std::string& name, glm::vec4 v) {
    glUniform4f(glGetUniformLocation(id, name.c_str()), v.x, v.y, v.z, v.w);
}

void Shader::setMat4(const std::string &name, glm::mat4 val) {
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, false, glm::value_ptr(val));
}

std::stringstream Shader::defaultHeaders;

void Shader::loadIntoDefault(const char* filepath) {
    std::string fileContents = Shader::loadShaderSrc(false, filepath);

    Shader::defaultHeaders << fileContents;
}

void Shader::clearDefault() {
    Shader::defaultHeaders.clear();
}

std::string Shader::loadShaderSrc(bool includeDefaultHeader, const char* filePath) {
    std::ifstream file;
    std::stringstream buf;

    std::string ret = "";

    if (includeDefaultHeader) {
        buf << Shader::defaultHeaders.str();
    }

    std::string fullPath = Shader::defaultDirectory + '/' + filePath;

    file.open(fullPath.c_str());

    if (file.is_open()) {
        buf << file.rdbuf();
        ret = buf.str();
    } else {
        std::cout << "Could not open " << filePath << std::endl;
    }

    file.close();

    return ret;
}