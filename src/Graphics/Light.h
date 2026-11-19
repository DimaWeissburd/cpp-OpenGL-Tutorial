#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include "Shader.h"
#include "../Algorithms/Bounds.h"
#include "FrameMemory.hpp"

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    BoundingRegion br;
    glm::mat4 lightSpaceMatrix;
    FramebufferObject shadowFBO;

    DirectionalLight(glm::vec3 direction, glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular, BoundingRegion br);

    void render(Shader shader, unsigned int textureIdx);

    void updateMatrices();
};

struct PointLight {
    glm::vec3 position;
    float k0;
    float k1;
    float k2;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    void render(Shader shader, int index);
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;
    float k0;
    float k1;
    float k2;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    void render(Shader shader, int index);
};

#endif