#include "Light.h"

DirectionalLight::DirectionalLight(glm::vec3 direction, glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular, BoundingRegion br)
    : direction(direction), ambient(ambient), diffuse(diffuse), specular(specular), shadowFBO(1024, 1024, GL_DEPTH_BUFFER_BIT), br(br) {
    shadowFBO.generate();
    shadowFBO.bind();
    shadowFBO.disableColorBuffer();
    shadowFBO.allocateAndAttachTexture(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, GL_FLOAT);
    updateMatrices();
}

void DirectionalLight::render(Shader shader, unsigned int textureIdx) {
    std::string name = "directionalLight";
    shader.set3Float(name + ".direction", direction);
    shader.set4Float(name + ".ambient", ambient);
    shader.set4Float(name + ".diffuse", diffuse);
    shader.set4Float(name + ".specular", specular);

    glActiveTexture(GL_TEXTURE0 + textureIdx);
    shadowFBO.textures[0].bind();
    shader.setInt("directionalLight.depthBuffer", textureIdx);

    shader.setMat4("directionalLight.lightSpaceMatrix", lightSpaceMatrix);
}

void DirectionalLight::updateMatrices() {
    glm::mat4 projection = glm::ortho(br.min.x, br.max.x, br.min.y, br.max.y, br.min.z, br.max.z);
    glm::vec3 position = -2.0f * direction;
    glm::mat4 lightView = glm::lookAt(position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    lightSpaceMatrix = projection * lightView;
}

void PointLight::render(Shader shader, int index) {
    std::string name = "pointLights[" + std::to_string(index) + "]";
    shader.set3Float(name + ".position", position);
    shader.setFloat(name + ".k0", k0);
    shader.setFloat(name + ".k1", k1);
    shader.setFloat(name + ".k2", k2);
    shader.set4Float(name + ".ambient", ambient);
    shader.set4Float(name + ".diffuse", diffuse);
    shader.set4Float(name + ".specular", specular);
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
    shader.set4Float(name + ".ambient", ambient);
    shader.set4Float(name + ".diffuse", diffuse);
    shader.set4Float(name + ".specular", specular);
}