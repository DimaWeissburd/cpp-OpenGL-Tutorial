#pragma once

#include <cfloat>
#include "../Model.h"
#include "../Camera.h"
#include "../../Scene.h"
#include "../../Algorithms/States.hpp"

class HandHeld : public Model {
public:
    HandHeld(unsigned int maxNoInstances, glm::vec3 position, glm::vec3 rotation, glm::vec3 size)
        : Model("HandHeld", maxNoInstances, position, rotation, size, 1.0f, DYNAMIC) {
        rigidBody.position = position;
        rigidBody.size = size;
        rigidBody.mass = 1.0f;
    }

    void render(Shader shader, float dt, Scene* scene) override {
        Camera* cam = scene->getActiveCamera();
        if (!cam || instances.empty()) return;

        glm::mat4 m(1.0f);   // start from identity every call

        glm::vec3 pos = cam->cameraPosition
            + cam->cameraFront * 1.5f
            - cam->cameraUp * 0.75f;
        m = glm::translate(m, pos);

        // Pitch alignment
        float cosPitch = glm::dot(cam->worldUp, cam->cameraFront)
            / (glm::length(cam->cameraUp) * glm::length(cam->cameraFront));
        float theta = acos(glm::clamp(cosPitch, -1.0f, 1.0f));
        m = glm::rotate(m, atanf(1) * 2 - theta, cam->cameraRight);

        // Yaw alignment
        glm::vec2 front2d(cam->cameraFront.x, cam->cameraFront.z);
        float len2d = glm::length(front2d);
        if (len2d > 1e-6f) {
            float cosYaw = glm::dot(glm::vec2(1.0f, 0.0f), front2d) / len2d;
            theta = acos(glm::clamp(cosYaw, -1.0f, 1.0f));
            m = glm::rotate(m, cam->cameraFront.z < 0 ? theta : -theta, cam->worldUp);
        }

        // Offset rotation + scale
        m = glm::rotate(m, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        m = glm::rotate(m, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        m = glm::rotate(m, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        m = glm::scale(m, size);

        // Store in the instance and upload
        RigidBody* inst = instances[0];
        inst->model = m;
        inst->normalModel = glm::transpose(glm::inverse(glm::mat3(m)));

        modelVBO.bind();
        modelVBO.updateData<glm::mat4>(0, 1, &inst->model);
        normalModelVBO.bind();
        normalModelVBO.updateData<glm::mat3>(0, 1, &inst->normalModel);

        // Draw (bypasses Model::render, which would overwrite the matrix)
        shader.setFloat("material.shininess", 0.05f);
        for (auto& mesh : meshes) {
            mesh.render(shader, currentNoInstances);
        }

        // Bounding box for the octree
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
            glm::vec3 world = glm::vec3(m * glm::vec4(corner, 1.0f));
            worldMin = glm::min(worldMin, world);
            worldMax = glm::max(worldMax, world);
        }

        glm::vec3 s = (worldMax - worldMin) / glm::max(localMax - localMin, glm::vec3(1e-6f));
        inst->size = s;
        inst->position = worldMin - localMin * s;
        States::activate(&inst->state, INSTANCE_MOVED);
    }
};