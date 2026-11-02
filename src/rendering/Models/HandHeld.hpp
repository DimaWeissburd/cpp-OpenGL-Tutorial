#include "../Model.h"
#include "../Camera.h"
#include "../../Scene.h"

class HandHeld : public Model {
public:
	HandHeld(unsigned int maxNoInstances) : Model("HandHeld", BoundTypes::AABB, maxNoInstances, CONST_INSTANCES | NO_TEX) {}

    void render(Shader shader, float dt, Scene* scene, bool setModel = false) {
        glm::mat4 model = glm::mat4(1.0f);

        // Translate to camera hand position
        rb.position = scene->getActiveCamera()->cameraPosition + glm::vec3(scene->getActiveCamera()->cameraFront * 1.5f) - glm::vec3(scene->getActiveCamera()->cameraUp * 0.75f);
        model = glm::translate(model, rb.position);

        // Align with camera (Pitch and Yaw)
        glm::vec2 front2d = glm::vec2(scene->getActiveCamera()->cameraFront.x, scene->getActiveCamera()->cameraFront.z);
        float theta;

        // Pitch alignment
        theta = acos(glm::dot(scene->getActiveCamera()->worldUp, scene->getActiveCamera()->cameraFront) / (glm::length(scene->getActiveCamera()->cameraUp) * glm::length(scene->getActiveCamera()->cameraFront)));
        model = glm::rotate(model, atanf(1) * 2 - theta, scene->getActiveCamera()->cameraRight);

        // Yaw alignment
        theta = acos(glm::dot(glm::vec2(1.0f, 0.0f), front2d) / glm::length(front2d));
        model = glm::rotate(model, scene->getActiveCamera()->cameraFront.z < 0 ? theta : -theta, scene->getActiveCamera()->worldUp);

        // Apply offset rotation
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

        // Scale
        model = glm::scale(model, size);

        shader.setMat4("model", model);

        Model::render(shader, dt, scene, false);

        // Adjust bounding box to rotation
        glm::vec3 localMin(FLT_MAX), localMax(-FLT_MAX);
        for (auto& mesh : meshes) {
            localMin = glm::min(localMin, mesh.br.min);
            localMax = glm::max(localMax, mesh.br.max);
        }

        glm::vec3 worldMin(FLT_MAX), worldMax(-FLT_MAX);
        for (int i = 0; i < 8; i++) {
            glm::vec3 corner(
                (i & 1) ? localMax.x : localMin.x,
                (i & 2) ? localMax.y : localMin.y,
                (i & 4) ? localMax.z : localMin.z
            );
            glm::vec3 world = glm::vec3(model * glm::vec4(corner, 1.0f));
            worldMin = glm::min(worldMin, world);
            worldMax = glm::max(worldMax, world);
        }

        glm::vec3 center = (worldMin + worldMax) * 0.5f;
        glm::vec3 dims = worldMax - worldMin;
    }
};