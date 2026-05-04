#include "Light.h"

void DirectionalLight::render(Shader shader) {
    std::string name = "directionalLight";
    shader.set3Float(name + ".direction", direction);
    shader.set3Float(name + ".ambient", ambient);
    shader.set3Float(name + ".diffuse", diffuse);
    shader.set3Float(name + ".specular", specular);
}

void PointLight::render(Shader shader, int index) {
    std::string name = "pointLights[" + std::to_string(index) + "]";
    shader.set3Float(name + ".position", position);
    shader.setFloat(name + ".k0", k0);
    shader.setFloat(name + ".k1", k1);
    shader.setFloat(name + ".k2", k2);
    shader.set3Float(name + ".ambient", ambient);
    shader.set3Float(name + ".diffuse", diffuse);
    shader.set3Float(name + ".specular", specular);
}

void SpotLight::render(Shader shader, int index) {
    std::string name = "spotLights[" + std::to_string(index) + "]";
    shader.set3Float(name + ".position", position);
    shader.set3Float(name + ".direction", direction);
    shader.setFloat(name + ".cutOff", cutOff);
    shader.setFloat(name + ".outerCutOff", outerCutOff);
    shader.setFloat(name + ".k0", k0);
    shader.setFloat(name + ".k1", k1);
    shader.setFloat(name + ".k2", k2);
    shader.set3Float(name + ".ambient", ambient);
    shader.set3Float(name + ".diffuse", diffuse);
    shader.set3Float(name + ".specular", specular);
}