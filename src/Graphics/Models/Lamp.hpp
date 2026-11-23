#ifndef LAMP_H
#define LAMP_H

#include "Cube.hpp"
#include "../Light.h"

class Lamp : public Cube {
public:
    glm::vec3 lightColor;

    Lamp(unsigned int maxNoInstances, glm::vec3 rotation, glm::vec3 size, glm::vec3 lightColor = glm::vec3(1.0f))
    : Cube(maxNoInstances, rotation, size, 1.0f, Material::white_rubber) {
        id = "lamp";
        this->lightColor = lightColor;
    }
};

#endif