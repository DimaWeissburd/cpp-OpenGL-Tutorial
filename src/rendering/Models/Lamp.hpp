#ifndef LAMP_H
#define LAMP_H

#include "Cube.hpp"
#include "ModelArray.hpp"
#include "../Light.h"

class Lamp : public Cube {
public:
    glm::vec3 lightColor;

    PointLight pointLight;

    Lamp() {}

    Lamp(glm::vec3 lightColor,
         glm::vec4 ambient,
         glm::vec4 diffuse,
         glm::vec4 specular,
         float k0,
         float k1,
         float k2,
         glm::vec3 position,
         glm::vec3 size)
         : lightColor(lightColor),
           pointLight({ position, k0, k1, k2, ambient, diffuse, specular }),
           Cube(position, size) {}

    void render(Shader shader, float dt) {
        shader.set3Float("lightColor", lightColor);
        Cube::render(shader, dt);
    }
};

class LampArray : public ModelArray<Lamp> {
public:
    std::vector<PointLight> lightInstances;

    glm::vec4 ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
    glm::vec4 diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    glm::vec4 specular = glm::vec4(1.0f);
    float k0 = 1.0f;
    float k1 = 0.09f;
    float k2 = 0.032f;

    void init() {
        model = Lamp(glm::vec3(1.0f),
            ambient, diffuse, specular,
            k0, k1, k2,
            glm::vec3(0.0f), glm::vec3(0.25f));
        model.init();
    }

    void render(Shader shader, float dt) {
        for (PointLight& pl : lightInstances) {
            model.rb.position = pl.position;
            model.render(shader, dt);
        }
    }
};

#endif