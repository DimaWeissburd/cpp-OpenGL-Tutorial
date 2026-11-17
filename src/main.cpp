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
#include <Rendering/Camera.h>
#include <Rendering/CubeMap.h>
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
#include <Utility/IniFile.h>

IniFile ini("Settings.ini");
Scene scene(ini);
Camera camera(ini);

void processInput(double dt);


float dt = 0.0f;
float lastFrame = 0.0f;

Sphere sphere(10);

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
    skyboxShader.activate();
    skyboxShader.set3Float("min", 0.047f, 0.016f, 0.239f);
    skyboxShader.set3Float("max", 0.945f, 1.000f, 0.682f);


    Cubemap skybox;
    skybox.init();
    skybox.loadTextures("Assets/SkyBox");

    Lamp lamp(4);
    scene.registerModel(&lamp);

    scene.registerModel(&sphere);

    Cube cube(1);
    scene.registerModel(&cube);

    Box box;
    box.init();

    scene.loadModels();

    //HandHeld magicStaff(1);
    //magicStaff.loadModel("Assets/Models/magic_staff/scene.gltf");
    //magicStaff.rotation.y = -90.0f;

    //Model fox("fox", BoundTypes::AABB, 1, CONST_INSTANCES);
    //fox.loadModel("Assets/Models/fox_in_a_cape/scene.gltf");
    //fox.rotation.x = -90.0f;
    //fox.rotation.z = -90.0f;

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
        scene.generateInstance(lamp.id, glm::vec3(0.25f), 0.25, pointLightPositions[i]);
        scene.pointLights.push_back(&pointLights[i]);
        States::activateIndex(&scene.activePointLights, i);
    }

    SpotLight spotLight = {
        scene.getActiveCamera()->cameraPosition, scene.getActiveCamera()->cameraFront,
        glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(20.0f)),
        1.0f, 0.0014f, 000007,
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
    };
    scene.spotLights.push_back(&spotLight);
    scene.activeSpotLights = 1;

    scene.generateInstance(cube.id, glm::vec3(20.0f, 0.1f, 20.0f), 101.0f, glm::vec3(0.0f, -0.3f, 0.0f));

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

        skyboxShader.activate();
        skyboxShader.setFloat("Time", scene.variableLog["Time"].val<float>());
        skybox.render(skyboxShader, &scene);

        scene.renderText("Helvetica", textShader, "Time: " + scene.variableLog["Time"].dump(), 50.0f, 550.0f, glm::vec2(1.0f), glm::vec3(0.5f, 0.6f, 1.0f));
        scene.renderText("Helvetica", textShader, "FPS: " + scene.variableLog["FPS"].dump(), 50.0f, 500.0f, glm::vec2(1.0f), glm::vec3(0.5f, 0.6f, 1.0f));

        for (int i = 0; i < sphere.currentNoInstances; i++) {
            if (glm::length(camera.cameraPosition - sphere.instances[i]->position) > 250.0f) {
                scene.markForDeletion(sphere.instances[i]->instanceId);
            }
        }

        scene.renderShader(shader);
        if (sphere.currentNoInstances > 0) {
            scene.renderInstances(sphere.id, shader, dt);
        }
        scene.renderInstances(cube.id, shader, dt);

        scene.renderShader(lampShader, false);
        scene.renderInstances(lamp.id, lampShader, dt);

        scene.renderShader(boxShader, false);
        box.render(boxShader);

        scene.newFrame(box);
        scene.clearDeadInstances();
    }

    skybox.cleanup();
    scene.cleanup();
    return 0;
}

void launchItem(float dt) {
    RigidBody* rb = scene.generateInstance(sphere.id, glm::vec3(0.1f), 1.0f, camera.cameraPosition);
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
}