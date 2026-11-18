#pragma once

#include <cfloat>
#include "../Model.h"
#include "../Camera.h"
#include "../../Scene.h"
#include "../../Algorithms/States.hpp"

class HandHeld : public Model {
public:
    HandHeld(unsigned int maxNoInstances, glm::vec3 position, glm::vec3 rotation, glm::vec3 size)
        : Model("HandHeld", BoundTypes::AABB, maxNoInstances, position, rotation, size, 1.0f, CONST_INSTANCES) {
        rb.position = position;
        rb.size = size;
        rb.mass = 1.0f;
    }

    void render(Shader shader, float dt, Scene* scene, bool setModel = false) override {
        Camera* cam = scene->getActiveCamera();
        if (!cam) return;

        glm::mat4 model = glm::mat4(1.0f);

        // Translate to camera hand position
        rb.position = cam->cameraPosition
            + cam->cameraFront * 1.5f
            - cam->cameraUp * 0.75f;
        model = glm::translate(model, rb.position);

        float theta;

        // Pitch alignment (clamped: acos is only defined on [-1, 1])
        float cosPitch = glm::dot(cam->worldUp, cam->cameraFront)
            / (glm::length(cam->cameraUp) * glm::length(cam->cameraFront));
        theta = acos(glm::clamp(cosPitch, -1.0f, 1.0f));
        model = glm::rotate(model, atanf(1) * 2 - theta, cam->cameraRight);

        // Yaw alignment (guarded against looking straight up/down, clamped like above)
        glm::vec2 front2d = glm::vec2(cam->cameraFront.x, cam->cameraFront.z);
        float len2d = glm::length(front2d);
        if (len2d > 1e-6f) {
            float cosYaw = glm::dot(glm::vec2(1.0f, 0.0f), front2d) / len2d;
            theta = acos(glm::clamp(cosYaw, -1.0f, 1.0f));
            model = glm::rotate(model, cam->cameraFront.z < 0 ? theta : -theta, cam->worldUp);
        }

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
            localMin = glm::min(localMin, mesh.boundingRegion.min);
            localMax = glm::max(localMax, mesh.boundingRegion.max);
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

        // Write the staff's real placement into the instance so the octree/box follow it.
        // Safe for rendering: CONST_INSTANCES models never re-upload aOffset/aSize.
        if (!instances.empty()) {
            RigidBody* inst = instances[0];

            glm::vec3 localExtent = localMax - localMin;
            glm::vec3 worldExtent = worldMax - worldMin;

            glm::vec3 s = worldExtent / glm::max(localExtent, glm::vec3(1e-6f));
            inst->size = s;
            inst->position = worldMin - localMin * s;

            States::activate(&inst->state, INSTANCE_MOVED);
        }
    }
};