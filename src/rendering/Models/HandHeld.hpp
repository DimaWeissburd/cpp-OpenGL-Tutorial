#include "../Model.h"
#include "../Camera.h"

class HandHeld : public Model {
public:
	HandHeld() : Model(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.001f), false) {}

    void render(Shader shader, bool setModel = false) {
        glm::mat4 model = glm::mat4(1.0f);

        // Translate to camera hand position
        position = Camera::cameras[0].cameraPosition + glm::vec3(Camera::cameras[0].cameraFront * 1.5f) - glm::vec3(Camera::cameras[0].cameraUp * 0.75f);
        model = glm::translate(model, position);

        // Align with camera (Pitch and Yaw)
        glm::vec2 front2d = glm::vec2(Camera::cameras[0].cameraFront.x, Camera::cameras[0].cameraFront.z);
        float theta;

        // Pitch alignment
        theta = acos(glm::dot(Camera::cameras[0].worldUp, Camera::cameras[0].cameraFront) / (glm::length(Camera::cameras[0].cameraUp) * glm::length(Camera::cameras[0].cameraFront)));
        model = glm::rotate(model, atanf(1) * 2 - theta, Camera::cameras[0].cameraRight);

        // Yaw alignment
        theta = acos(glm::dot(glm::vec2(1.0f, 0.0f), front2d) / glm::length(front2d));
        model = glm::rotate(model, Camera::cameras[0].cameraFront.z < 0 ? theta : -theta, Camera::cameras[0].worldUp);

        // Apply offset rotation
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

        // Scale
        model = glm::scale(model, size);

        shader.setMat4("model", model);

        Model::render(shader, false);
    }
};