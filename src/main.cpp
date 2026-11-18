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
#include <Graphics/Models/Box.hpp>
#include <Graphics/Models/Cube.hpp>
#include <Graphics/Models/HandHeld.hpp>
#include <Graphics/Models/Lamp.hpp>
#include <Graphics/Models/Plane.hpp>
#include <Graphics/Models/Sphere.hpp>
#include <Utility/IniFile.h>

IniFile ini("Settings.ini");
Scene scene(ini);
Camera camera(ini);

void processInput(double dt);

float dt = 0.0f;
float lastFrame = 0.0f;

Sphere sphere(10, glm::vec3(0.0f), glm::vec3(1.0f), 1.0f);

Joystick joystick(0);

unsigned int SCR_WIDTH = ini.getInt("window", "width", 800);
unsigned int SCR_HEIGHT = ini.getInt("window", "height", 600);
std::string title = ini.getString("window", "title", "OpenGL Tutorial");

int main() {
    scene = Scene(ini, 3, 3, title.c_str(), SCR_WIDTH, SCR_HEIGHT);

    scene.cameras.push_back(&camera);
    scene.activeCamera = 0;

    if (!scene.init()) {
        std::cout << "Could not open window" << std::endl;
        glfwTerminate();
        return -1;
    }
    Shader shader("Assets/Instanced/Instanced.vs", "Assets/Object.fs");
    Shader lampShader("Assets/Instanced/Instanced.vs", "Assets/Lamp.fs");
    Shader boxShader("Assets/Instanced/Box.vs", "Assets/Instanced/Box.fs");
    Shader textShader("Assets/Text.vs", "Assets/Text.fs");
    Shader skyboxShader("Assets/SkyBox/SkyBox.vs", "Assets/SkyBox/Sky.fs");
    Shader outlineShader("Assets/Outline.vs", "Assets/Outline.fs");
    Shader bufferShader("Assets/Buffer.vs", "Assets/Buffer.fs");

    skyboxShader.activate();
    skyboxShader.set3Float("min", 0.047f, 0.016f, 0.239f);
    skyboxShader.set3Float("max", 0.945f, 1.000f, 0.682f);

    Cubemap skybox;
    skybox.init();
    skybox.loadTextures("Assets/SkyBox");

    Lamp lamp(4, glm::vec3(0.0f), glm::vec3(1.0f));
    scene.registerModel(&lamp);

    scene.registerModel(&sphere);

    Cube cube(10, glm::vec3(0.0f), glm::vec3(1.0f), 1.0f);
    scene.registerModel(&cube);

    Box box;
    box.init();

    const GLuint BUFFER_WIDTH = 800, BUFFER_HEIGHT = 600;
    FramebufferObject fbo(BUFFER_WIDTH, BUFFER_HEIGHT, GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    fbo.generate();
    fbo.bind();

    Texture bufferTex("bufferTex");
    bufferTex.bind();
    bufferTex.allocate(GL_RGBA, BUFFER_WIDTH, BUFFER_HEIGHT, GL_UNSIGNED_BYTE);
    Texture::setParams();

    fbo.attachTexture(GL_COLOR_ATTACHMENT0, bufferTex);
    fbo.allocateAndAttachRBO(GL_DEPTH_STENCIL_ATTACHMENT, GL_DEPTH24_STENCIL8);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Error with framebuffer" << std::endl;
    }

    scene.defaultFBO.bind();

    Plane map;
    map.init(bufferTex);
    scene.registerModel(&map);

    scene.loadModels();

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

    for (unsigned int i = 0; i < 4; i++) {
        pointLights[i] = {
            pointLightPositions[i],
            k0, k1, k2,
            ambient, diffuse, specular
        };
        scene.generateInstance(lamp.id, pointLightPositions[i], glm::vec3(0.25f), 0.25);
        scene.pointLights.push_back(&pointLights[i]);
        States::activateIndex(&scene.activePointLights, i);
    }

    SpotLight spotLight = {
        scene.getActiveCamera()->cameraPosition, scene.getActiveCamera()->cameraFront,
        glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(20.0f)),
        1.0f, 0.0014f, 0.000007,
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
    };
    scene.spotLights.push_back(&spotLight);
    scene.activeSpotLights = 1;

    scene.generateInstance(cube.id, glm::vec3(0.0f, -0.3f, 0.0f), glm::vec3(20.0f, 0.1f, 20.0f), 101.0f);
    glm::vec3 cubePositions[] = {
        { 1.0f, 3.0f, -5.0f },
        { -7.25f, 2.1f, 1.5f },
        { -15.0f, 2.55f, 9.0f },
        { 4.0f, -3.5f, 5.0f },
        { 2.8f, 1.9f, -6.2f },
        { 3.5f, 6.3f, -1.0f },
        { -3.4f, 10.9f, -5.5f },
        { 10.0f, -2.0f, 13.2f },
        { 2.1f, 7.9f, -8.3f },
    };
    for (unsigned int i = 0; i < 9; i++) {
        scene.generateInstance(cube.id, cubePositions[i], glm::vec3(0.5f), 1.0f);
    }

    scene.generateInstance(map.id, glm::vec3(0.0f), glm::vec3(2.0f, 2.0f, 0.0f), 0.0f);

    HandHeld magicStaff(1, camera.cameraPosition, glm::vec3(135.0f, 90.0f, 0.0f), glm::vec3(0.001f));
    magicStaff.loadModel("Assets/Models/magic_staff/scene.gltf");
    scene.registerModel(&magicStaff);
    scene.generateInstance(magicStaff.id, glm::vec3(0.0f), glm::vec3(1.0f), 1.0f);

    Model fox("fox", BoundTypes::AABB, 1, glm::vec3(20.0f), glm::vec3(-90.0f, -90.0f, 0.0f), glm::vec3(-20.0f, 20.0f, 20.0f), 1.0f, CONST_INSTANCES);
    fox.loadModel("Assets/Models/fox_in_a_cape/scene.gltf");
    scene.registerModel(&fox);
    scene.generateInstance(fox.id, fox.position, fox.size, fox.mass);

    scene.initInstances();
    scene.prepare(box);

    joystick.update();
    if (joystick.isPresent()) {
        std::cout << "Joystick " << joystick.getName() << " is present." << std::endl;
    } else {
        std::cout << "Joystick not present." << std::endl;
    }

    scene.variableLog["Time"] = (double)0.0;

    while (!scene.shouldClose()) {
        double currentTime = glfwGetTime();
        dt = currentTime - lastFrame;
        lastFrame = currentTime;

        scene.variableLog["Time"] += dt;
        scene.variableLog["FPS"] = 1 / dt;

        scene.update();
        
        processInput(dt);

        fbo.activate();

        skyboxShader.activate();
        skyboxShader.setFloat("Time", scene.variableLog["Time"].val<float>());
        skybox.render(skyboxShader, &scene);

        scene.renderText("Helvetica", textShader, "Time: " + scene.variableLog["Time"].dump(), 20.0f, 550.0f, glm::vec2(1.0f), glm::vec3(0.5f, 0.6f, 1.0f));
        scene.renderText("Helvetica", textShader, "FPS: " + scene.variableLog["FPS"].dump(), 20.0f, 500.0f, glm::vec2(1.0f), glm::vec3(0.5f, 0.6f, 1.0f));

        for (int i = 0; i < sphere.currentNoInstances; i++) {
            if (glm::length(camera.cameraPosition - sphere.instances[i]->position) > 250.0f) {
                scene.markForDeletion(sphere.instances[i]->instanceId);
            }
        }

        if (scene.variableLog["displayOutlines"].val<bool>()) {
            glStencilMask(0x00); // Disable writing to stencil buffer for spheres
        }

        // Render lamps
        scene.renderShader(lampShader, false);
        scene.renderInstances(lamp.id, lampShader, dt);
        
        scene.renderShader(shader);
        if (sphere.currentNoInstances > 0) {
            scene.renderInstances(sphere.id, shader, dt);
        }

        fox.rotation.y += 10.0f * dt;
        scene.renderInstances(fox.id, shader, dt);
        scene.renderInstances(magicStaff.id, shader, dt);

        if (scene.variableLog["displayOutlines"].val<bool>()) {
            glStencilFunc(GL_ALWAYS, 1, 0xFF); // Always write to stencil buffer for cubes
            glStencilMask(0xFF); // Always write to buffer
            scene.renderInstances(cube.id, shader, dt);

            glStencilFunc(GL_NOTEQUAL, 1, 0xFF); // Render fragments if different than what is stored
            glStencilMask(0x00); // Disable writing
            glDisable(GL_DEPTH_TEST); // Disable depth test so outlines are displayed behind

            // Draw outline of the cubes
            scene.renderShader(outlineShader, false);
            scene.renderInstances(cube.id, outlineShader, dt);

            glStencilFunc(GL_ALWAYS, 1, 0xFF); // Every fragment written to stencil buffer
            glStencilMask(0xFF); // Always write to buffer
            glEnable(GL_DEPTH_TEST); // Reenable depth testing
        } else { // Render cubes normally
            scene.renderInstances(cube.id, shader, dt);
        }

        scene.renderShader(boxShader, false);
        box.render(boxShader);

        scene.defaultFBO.activate();
        
        scene.renderInstances(map.id, bufferShader, dt);

        scene.newFrame(box);
        scene.clearDeadInstances();
    }

    fox.cleanup();
    magicStaff.cleanup();
    skybox.cleanup();
    fbo.cleanup();
    scene.cleanup();
    return 0;
}

void launchItem(float dt) {
    RigidBody* rb = scene.generateInstance(sphere.id, camera.cameraPosition, glm::vec3(0.1f), 1.0f);
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

    if (Keyboard::keyPressed(GLFW_KEY_O)) {
        scene.variableLog["displayOutlines"] = !scene.variableLog["displayOutlines"].val<bool>();
    }
}