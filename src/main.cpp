#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <fstream>
#include <string>
#include <vector>
#include <stack>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Scene.h>
#include <Algorithms/States.hpp>
#include <IO/Joystick.h>
#include <IO/Keyboard.h>
#include <IO/Mouse.h>
#include <Physics/Environment.h>
#include <Rendering/Camera.h>
#include <Rendering/Light.h>
#include <Rendering/Mesh.h>
#include <Rendering/Model.h>
#include <Rendering/Shader.h>
#include <Rendering/Texture.h>
#include <Rendering/Models/Box.hpp>
#include <Rendering/Models/Cube.hpp>
#include <Rendering/Models/HandHeld.hpp>
#include <Rendering/Models/Lamp.hpp>
#include <Rendering/Models/Sphere.hpp>

Scene scene;
Camera camera;

void processInput(double dt);


float dt = 0.0f;
float lastFrame = 0.0f;

Joystick joystick(0);

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

SphereArray launchObjects;

int main() {
    scene = Scene(3, 3, "OpenGL Tutorial", 800, 600);

    scene.cameras.push_back(&camera);
    scene.activeCamera = 0;

    if (!scene.init()) {
        std::cout << "Could not open window" << std::endl;
        glfwTerminate();
        return -1;
    }

    Shader shader("Assets/Object.vs", "Assets/Object.fs");
    Shader lampShader("Assets/Instanced/Instanced.vs", "Assets/Lamp.fs");
    Shader launchShader("Assets/Instanced/Instanced.vs", "Assets/Object.fs");
    Shader boxShader("Assets/Instanced/Box.vs", "Assets/Instanced/Box.fs");

    HandHeld magicStaff;
    magicStaff.loadModel("Assets/Models/magic_staff/scene.gltf");
    magicStaff.rotation.y = -90.0f;

    Model fox;
    fox.loadModel("Assets/Models/fox_in_a_cape/scene.gltf");
    fox.rotation.x = -90.0f;
    fox.rotation.z = -90.0f;

    launchObjects.init();

    Box box;
    box.init();

    DirectionalLight directionalLight = { glm::vec3(-0.2f, -1.0f, -0.3f),
        glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), glm::vec4(0.4f, 0.4f, 0.4f, 1.0f), glm::vec4(0.75f, 0.75f, 0.75f, 1.0f) };

    scene.directionalLight = &directionalLight;
    scene.activeSpotLights = 0;

    glm::vec3 pointLightPositions[] = {
        glm::vec3(0.7f,  0.2f,  2.0f),
        glm::vec3(2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3(0.0f,  0.0f, -3.0f)
    };

    glm::vec4 ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
    glm::vec4 diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    glm::vec4 specular = glm::vec4(1.0f);
    float k0 = 1.0f;
    float k1 = 0.09f;
    float k2 = 0.032f;

    PointLight pointLights[4];
    LampArray lamps;
    lamps.init();
    for (unsigned int i = 0; i < 4; i++) {
        pointLights[i] = {
            pointLightPositions[i],
            k0, k1, k2,
            ambient, diffuse, specular
        };
        lamps.lightInstances.push_back(pointLights[i]);
        scene.pointLights.push_back(&pointLights[i]);
        States::activate(&scene.activePointLights, i);
    }

    SpotLight spotLight = {
        scene.getActiveCamera()->cameraPosition, scene.getActiveCamera()->cameraFront,
        glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(20.0f)),
        1.0f, 0.07f, 0.032f,
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
    };
    scene.spotLights.push_back(&spotLight);
    States::activate(&scene.activeSpotLights, 0);

    joystick.update();
    if (joystick.isPresent()) {
        std::cout << "Joystick " << joystick.getName() << " is present." << std::endl;
    } else {
        std::cout << "Joystick not present." << std::endl;
    }

    while (!scene.shouldClose()) {
        box.positions.clear();
        box.sizes.clear();

        double currentTime = glfwGetTime();
        dt = currentTime - lastFrame;
        lastFrame = currentTime;

        processInput(dt);
        scene.update();

        spotLight.position = scene.getActiveCamera()->cameraPosition;
        spotLight.direction = scene.getActiveCamera()->cameraFront;

        scene.render(shader);
        fox.render(shader, dt, &box);
        magicStaff.render(shader, dt, &box, &scene);

        for (int i = (int)launchObjects.instances.size() - 1; i >= 0; i--) {
            if (glm::length(scene.getActiveCamera()->cameraPosition - launchObjects.instances[i].position) > 50.0f) {
                launchObjects.instances.erase(launchObjects.instances.begin() + i);
            }
        }
        if (!launchObjects.instances.empty()) {
            scene.render(launchShader);
            launchObjects.render(launchShader, dt, &box);
        }

        scene.render(lampShader, false);
        lamps.render(lampShader, dt, &box);

        if (box.positions.size() > 0) {
            scene.render(boxShader, false);
            box.render(boxShader);
        }

        scene.newFrame();
    }

    fox.cleanup();
    magicStaff.cleanup();
    lamps.cleanup();
    box.cleanup();
    launchObjects.cleanup();

    scene.cleanup();
    return 0;
}

void launchItem(float dt) {
    RigidBody rb(1.0f, scene.getActiveCamera()->cameraPosition);
    rb.transferEnergy(100.0f, scene.getActiveCamera()->cameraFront);
    rb.applyAcceleration(Environment::gravitationalAcceleration);
    launchObjects.instances.push_back(rb);
}

void processInput(double dt) {
    scene.processInput(dt);

    if (Keyboard::key(GLFW_KEY_ESCAPE)) {
        scene.setShouldClose(true);
    }

    if (Keyboard::keyPressed(GLFW_KEY_F)) {
        States::toggle(&scene.activeSpotLights, 0);
    }

    if (Keyboard::keyPressed(GLFW_KEY_E)) {
        launchItem(dt);
    }

    for (int i = 0; i < 4; i++) {
        if (Keyboard::keyPressed(GLFW_KEY_1 + i)) {
            States::toggle(&scene.activePointLights, i);
        }
    }
}