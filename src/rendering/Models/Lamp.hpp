#ifndef LAMP_H
#define LAMP_H

#include "cube.hpp"

class Lamp : public Cube {
public:
    glm::vec3 lightColor;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    Lamp(glm::vec3 lightColor, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, glm::vec3 position, glm::vec3 size)
        : lightColor(lightColor), ambient(ambient), diffuse(diffuse), specular(specular), Cube(Material::white_plastic, position, size) {}

    void render(Shader shader) {
        shader.set3Float("lightColor", lightColor);
        Cube::render(shader);
    }
};

#endif