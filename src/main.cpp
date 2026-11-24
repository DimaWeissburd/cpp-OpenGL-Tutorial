// Following C++ OpenGL Tutorial by Michael Grieco
// https://github.com/michaelgrieco/yt-tutorials/tree/master/CPP/OpenGL/OpenGLTutorial
// https://www.youtube.com/playlist?list=PLysLvOneEETPlOI_PI4mJnocqIpr2cSHS

#include <ft2build.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>
#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <vector>
#include <Scene.h>
#include <Algorithms/States.hpp>
#include <IO/Joystick.h>
#include <IO/Keyboard.h>
#include <IO/Mouse.h>
#include <Physics/Environment.h>
#include <Graphics/Camera.h>
#include <Graphics/CubeMap.h>
#include <Graphics/FrameMemory.hpp>
#include <Graphics/Light.h>
#include <Graphics/Mesh.h>
#include <Graphics/Model.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture.h>
#include <Graphics/UniformMemory.hpp>
#include <Graphics/Models/Box.hpp>
#include <Graphics/Models/Brickwall.hpp>
#include <Graphics/Models/Cube.hpp>
#include <Graphics/Models/HandHeld.hpp>
#include <Graphics/Models/Lamp.hpp>
#include <Graphics/Models/Plane.hpp>
#include <Graphics/Models/Sphere.hpp>
#include <Utility/IniFile.h>
#include "Utility/GLErrorChecker.h"

IniFile ini("Settings.ini");
Scene scene(ini);
Camera camera(ini);

void processInput(double dt);
void renderScene(Shader shader);

float dt = 0.0f;
float lastFrame = 0.0f;

Sphere sphere(10, glm::vec3(0.0f), glm::vec3(1.0f), 1.0f);
Cube cube(10, glm::vec3(0.0f), glm::vec3(1.0f), 1.0f);
Lamp lamp(4, glm::vec3(0.0f), glm::vec3(1.0f));
HandHeld magicStaff(1, camera.cameraPosition, glm::vec3(135.0f, 90.0f, 0.0f), glm::vec3(0.001f));
Model fox("fox", BoundTypes::AABB, 1, glm::vec3(20.0f), glm::radians(glm::vec3(-90.0f, -90.0f, 0.0f)), glm::vec3(20.0f), 1.0f, DYNAMIC);
Brickwall wall;

Joystick joystick(0);

unsigned int SCR_WIDTH = ini.getInt("window", "width", 1200);
unsigned int SCR_HEIGHT = ini.getInt("window", "height", 720);
std::string title = ini.getString("window", "title", "OpenGL Tutorial");

std::string Shader::defaultDirectory = "Assets/Shaders";

int main() {
    scene = Scene(ini, 3, 3, title.c_str(), SCR_WIDTH, SCR_HEIGHT);

    scene.cameras.push_back(&camera);
    scene.activeCamera = 0;

    if (!scene.init()) {
        std::cout << "Could not open window" << std::endl;
        glfwTerminate();
        return -1;
    }

    Shader::loadIntoDefault("defaultHead.gh");

    Shader shader(true, "Instanced/Instanced.vs", "Object.fs");
    Shader boxShader(false, "Instanced/Box.vs", "Instanced/Box.fs");
    Shader dirShadowShader(false, "Shadows/DirSpotShadow.vs", "Shadows/DirShadow.fs");
    Shader spotShadowShader(false, "Shadows/DirSpotShadow.vs", "Shadows/PointSpotShadow.fs");
    Shader pointShadowShader(false, "Shadows/PointShadow.vs", "Shadows/PointSpotShadow.fs", "Shadows/PointShadow.gs");
    Shader skyboxShader(false, "SkyBox/SkyBox.vs", "SkyBox/Sky.fs");
    Shader textShader(false, "Text.vs", "Text.fs");

    Shader::clearDefault();

    skyboxShader.activate();
    skyboxShader.set3Float("min", 0.047f, 0.016f, 0.239f);
    skyboxShader.set3Float("max", 0.945f, 1.000f, 0.682f);

    Cubemap skybox;
    skybox.init();
    //skybox.loadTextures("Assets/Textures/SkyBox");

    scene.registerModel(&lamp);
    scene.registerModel(&sphere);
    scene.registerModel(&cube);
    scene.registerModel(&wall);

    Box box;
    box.init();

    magicStaff.loadModel("Assets/Models/magic_staff/scene.gltf");
    scene.registerModel(&magicStaff);
    scene.generateInstance(magicStaff.id, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f), 1.0f);

    fox.loadModel("Assets/Models/fox_in_a_cape/scene.gltf");
    scene.registerModel(&fox);
    scene.generateInstance(fox.id, fox.position, fox.rotation, fox.size, fox.mass);

    scene.loadModels();

    DirectionalLight directionalLight(glm::vec3(-0.2f, -0.9f, -0.2f),
        glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),
        glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
        glm::vec4(0.75f, 0.75f, 0.75f, 1.0f),
        BoundingRegion(glm::vec3(-20.0f, -20.0f, 0.5f), glm::vec3(20.0f, 20.0f, 50.0f)));
    scene.directionalLight = &directionalLight;

    glm::vec3 pointLightPositions[] = {
        glm::vec3(1.0f,  1.0f,  0.0f),
        glm::vec3(0.0f,  15.0f,  0.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3(0.0f,  0.0f, -3.0f)
    };

    glm::vec4 ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
    glm::vec4 diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    glm::vec4 specular = glm::vec4(1.0f);
    float k0 = 1.0f;
    float k1 = 0.0014f;
    float k2 = 0.000007f;

    PointLight pointLights[4];

    for (unsigned int i = 0; i < 4; i++) {
        pointLights[i] = PointLight(
            pointLightPositions[i],
            k0, k1, k2,
            ambient, diffuse, specular,
            0.5f, 50.0f
        );
        scene.generateInstance(lamp.id, pointLightPositions[i], glm::vec3(0.0f), glm::vec3(0.25f), 0.25);
        scene.pointLights.push_back(&pointLights[i]);
        States::activateIndex(&scene.activePointLights, i);
    }

    SpotLight flashLight(
        scene.getActiveCamera()->cameraPosition, scene.getActiveCamera()->cameraFront, glm::vec3(1.0f, 0.0f, 0.0f),
        glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(20.0f)),
        1.0f, 0.0014f, 0.000007,
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f), glm::vec4(1.0f),
        0.1f, 100.0f
    );
    scene.spotLights.push_back(&flashLight);

    scene.activeSpotLights = 1;

    scene.generateInstance(cube.id, glm::vec3(0.0f, -0.3f, 0.0f), glm::vec3(0.0f), glm::vec3(20.0f, 0.1f, 20.0f), 101.0f);
    glm::vec3 cubePositions[] = {
        { 1.0f, 3.0f, -5.0f },
        { -7.25f, 2.1f, 1.5f },
        { -15.0f, 2.55f, 9.0f },
        { 4.0f, -3.5f, 5.0f },
        { 2.8f, 1.9f, -6.2f },
        { 3.5f, 6.3f, -1.0f },
        { -3.4f, 10.9f, -5.5f },
        { 0.0f, 11.0f, 0.0f },
        { 0.0f, 5.0f, 0.0f },
    };
    for (unsigned int i = 0; i < 9; i++) {
        scene.generateInstance(cube.id, cubePositions[i], glm::vec3(0.0f), glm::vec3(0.5f), 1.0f);
    }

    scene.generateInstance(wall.id, glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(0.0f), glm::vec3(10.0f), 1.0f);
    
    scene.initInstances();
    scene.prepare(box, { shader });

    joystick.update();
    if (joystick.isPresent()) {
        std::cout << "Joystick " << joystick.getName() << " is present." << std::endl;
    } else {
        std::cout << "Joystick not present." << std::endl;
    }

    scene.variableLog["Time"] = (double)0.0;

    scene.defaultFBO.bind();

    while (!scene.shouldClose()) {
        double currentTime = glfwGetTime();
        dt = currentTime - lastFrame;
        lastFrame = currentTime;

        scene.variableLog["Time"] += dt;
        scene.variableLog["FPS"] = 1 / dt;

        scene.update();

        processInput(dt);
        scene.updateLightUBO();

        fox.instances[0]->rotation.y += glm::radians(10.0f) * dt;

        for (int i = 0; i < sphere.currentNoInstances; i++) {
            if (glm::length(camera.cameraPosition - sphere.instances[i]->position) > 250.0f) {
                scene.markForDeletion(sphere.instances[i]->instanceId);
            }
        }

        // Render scene for directional light FBO
        directionalLight.shadowFBO.activate();
        scene.renderDirectionalLightShader(dirShadowShader);
        renderScene(dirShadowShader);

        // Render scene for point light FBO
        for (unsigned int i = 0, length = scene.pointLights.size(); i < length; i++) {
            if (States::isIndexActive(&scene.activePointLights, i)) {
                scene.pointLights[i]->shadowFBO.activate();
                scene.renderPointLightShader(pointShadowShader, i);
                renderScene(pointShadowShader);
            }
        }

        // Render scene for spotlight FBO
        for (unsigned int i = 0, length = scene.spotLights.size(); i < length; i++) {
            if (States::isIndexActive(&scene.activeSpotLights, i)) {
                scene.spotLights[i]->shadowFBO.activate();
                scene.renderSpotLightShader(spotShadowShader, i);
                renderScene(spotShadowShader);
            }
        }

        // Render scene normally
        scene.defaultFBO.activate();

        skyboxShader.activate();
        skyboxShader.setFloat("Time", scene.variableLog["Time"].val<float>());
        skybox.render(skyboxShader, &scene);

        scene.renderShader(shader);
        renderScene(shader);

        scene.renderText("Helvetica", textShader, "Time: " + scene.variableLog["Time"].dump(), SCR_WIDTH - 75.0f, SCR_HEIGHT - 10.0f, glm::vec2(0.3f), glm::vec3(0.5f, 0.6f, 1.0f));
        scene.renderText("Helvetica", textShader, "FPS: " + scene.variableLog["FPS"].dump(), SCR_WIDTH - 75.0f, SCR_HEIGHT - 20.0f, glm::vec2(0.3f), glm::vec3(0.5f, 0.6f, 1.0f));

        scene.newFrame(box);
        scene.clearDeadInstances();
    }

    fox.cleanup();
    magicStaff.cleanup();
    skybox.cleanup();
    scene.cleanup();
    return 0;
}

void renderScene(Shader shader) {
    if (sphere.currentNoInstances > 0) {
        scene.renderInstances(sphere.id, shader, dt);
    }
    scene.renderInstances(cube.id, shader, dt);
    scene.renderInstances(lamp.id, shader, dt);
    scene.renderInstances(wall.id, shader, dt);
    scene.renderInstances(fox.id, shader, dt);
    scene.renderInstances(magicStaff.id, shader, dt);
}

void launchItem(float dt) {
    RigidBody* rb = scene.generateInstance(sphere.id, camera.cameraPosition, glm::vec3(0.0f), glm::vec3(0.5f), 1.0f);
    if (rb) {
        rb->transferEnergy(50.0f, camera.cameraFront);
        rb->applyAcceleration(Environment::gravitationalAcceleration);
    }
}

void processInput(double dt) {
    scene.processInput(dt);

    if (Keyboard::key(GLFW_KEY_ESCAPE)) {
        scene.setShouldClose(true);
    }

    if (States::isIndexActive(&scene.activeSpotLights, 0)) {
        scene.spotLights[0]->position = scene.getActiveCamera()->cameraPosition;
        scene.spotLights[0]->direction = scene.getActiveCamera()->cameraFront;
        scene.spotLights[0]->up = scene.getActiveCamera()->cameraUp; 
        scene.spotLights[0]->updateMatrices();
    }

    if (Keyboard::keyPressed(GLFW_KEY_F)) {
        States::toggleIndex(&scene.activeSpotLights, 0);
    }

    if (Keyboard::keyPressed(GLFW_KEY_E)) {
        launchItem(dt);
    }

    for (int i = 0; i < 4; i++) {
        if (Keyboard::keyPressed(GLFW_KEY_1 + i)) {
            States::toggleIndex(&scene.activePointLights, i);
        }
    }
}