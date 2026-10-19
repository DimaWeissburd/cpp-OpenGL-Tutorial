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
#include <IO/Joystick.h>
#include <IO/Keyboard.h>
#include <IO/Mouse.h>
#include <Physics/Environment.h>
#include <Rendering/Camera.h>
#include <Rendering/Light.h>
#include <Rendering/Mesh.h>
#include <Rendering/Model.h>
#include <Rendering/Screen.h>
#include <Rendering/Shader.h>
#include <Rendering/Texture.h>
#include <Rendering/Models/Box.hpp>
#include <Rendering/Models/Cube.hpp>
#include <Rendering/Models/HandHeld.hpp>
#include <Rendering/Models/Lamp.hpp>
#include <Rendering/Models/Sphere.hpp>

void processInput(double dt);

Camera Camera::cameras[2] = {
    Camera(glm::vec3(0.0f, 0.0f, 0.0f)),
    Camera(glm::vec3(10.0f, 10.0f, 10.0f))
};
int activeCamera = 0;
int controlledCamera = 0;

float dt = 0.0f;
float lastFrame = 0.0f;

Box box;

bool flashLightOn = true;

Joystick joystick(0);

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

Screen screen;

SphereArray launchObjects;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPEGL_FORWARD_COMPAT, GL_TRUE);
#endif

    if (!screen.init()) {
        std::cout << "Failed to create window." << std::endl;
        glfwTerminate();
        return -1;
    }

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD." << std::endl;
        glfwTerminate();
        return -1;
    }

    screen.setParameters();

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
    box.init();

    DirectionalLight directionalLight = { glm::vec3(-0.2f, -1.0f, -0.3f),
        glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), glm::vec4(0.4f, 0.4f, 0.4f, 1.0f), glm::vec4(0.75f, 0.75f, 0.75f, 1.0f) };

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

    LampArray lamps;
    lamps.init();
    for (unsigned int i = 0; i < 4; i++) {
        lamps.lightInstances.push_back({
            pointLightPositions[i],
            k0, k1, k2,
            ambient, diffuse, specular
        });
    }

    SpotLight spotLight = {
        Camera::cameras[activeCamera].cameraPosition, Camera::cameras[activeCamera].cameraFront,
        glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(20.0f)),
        1.0f, 0.07f, 0.032f,
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
    };

    joystick.update();
    if (joystick.isPresent()) {
        std::cout << "Joystick " << joystick.getName() << " is present." << std::endl;
    } else {
        std::cout << "Joystick not present." << std::endl;
    }

    while (!screen.shouldClose()) {
        double currentTime = glfwGetTime();
        dt = currentTime - lastFrame;
        lastFrame = currentTime;

        processInput(dt);
        screen.update();

        glm::mat4 view = Camera::cameras[activeCamera].getViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(Camera::cameras[activeCamera].getZoom()),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 100.0f
        );
        glm::vec3 viewPos = Camera::cameras[activeCamera].cameraPosition;

        float rotationSpeed = 15.0f * (float)dt;
        directionalLight.direction = glm::vec3(
            glm::rotate(glm::mat4(1.0f), glm::radians(rotationSpeed), glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::vec4(directionalLight.direction, 1.0f)
        );

        if (flashLightOn) {
            spotLight.position = viewPos;
            spotLight.direction = Camera::cameras[activeCamera].cameraFront;
        }

        for (int i = (int)launchObjects.instances.size() - 1; i >= 0; i--) {
            if (glm::length(Camera::cameras[0].cameraPosition - launchObjects.instances[i].position) > 50.0f) {
                launchObjects.instances.erase(launchObjects.instances.begin() + i);
            }
        }

        lampShader.activate();
        lampShader.setMat4("view", view);
        lampShader.setMat4("projection", projection);
        lamps.render(lampShader, dt);

        shader.activate();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.set3Float("viewPosition", viewPos);
        directionalLight.render(shader);
        for (unsigned int i = 0; i < 4; i++) {
            lamps.lightInstances[i].render(shader, i);
        }
        shader.setInt("noPointLights", 4);
        if (flashLightOn) {
            spotLight.render(shader, 0);
            shader.setInt("noSpotLights", 1);
        }
        else {
            shader.setInt("noSpotLights", 0);
        }
        fox.render(shader, dt);

        if (!launchObjects.instances.empty()) {
            launchShader.activate();
            launchShader.setMat4("view", view);
            launchShader.setMat4("projection", projection);
            launchShader.set3Float("viewPosition", viewPos);

            directionalLight.render(launchShader);
            for (unsigned int i = 0; i < 4; i++) {
                lamps.lightInstances[i].render(launchShader, i);
            }
            launchShader.setInt("noPointLights", 4);

            if (flashLightOn) {
                spotLight.render(launchShader, 0);
                launchShader.setInt("noSpotLights", 1);
            }
            else {
                launchShader.setInt("noSpotLights", 0);
            }

            launchObjects.render(launchShader, dt);
        }

        if (box.offsets.size() > 0) {
            boxShader.activate();
            boxShader.setMat4("view", view);
            boxShader.setMat4("projection", projection);
            box.render(boxShader);
        }

        screen.newFrame();
    }

    fox.cleanup();
    lamps.cleanup();
    box.cleanup();
    launchObjects.cleanup();

    glfwTerminate();
    return 0;
}

void launchItem(float dt) {
    RigidBody rb(1.0f, Camera::cameras[0].cameraPosition);
    rb.transferEnergy(100.0f, Camera::cameras[0].cameraFront);
    rb.applyAcceleration(Environment::gravitationalAcceleration);
    launchObjects.instances.push_back(rb);
}

void processInput(double dt) {
    if (Keyboard::key(GLFW_KEY_ESCAPE)) {
        screen.setShouldClose(true);
    }

    if (Keyboard::keyPressed(GLFW_KEY_TAB)) {
        activeCamera = activeCamera == 0 ? 1 : 0;
    }

    if (Keyboard::keyPressed(GLFW_KEY_GRAVE_ACCENT)) {
        controlledCamera = controlledCamera == 0 ? 1 : 0;
    }

    if (Keyboard::keyPressed(GLFW_KEY_F)) {
        flashLightOn = !flashLightOn;
    }

    if (Keyboard::key(GLFW_KEY_W)) {
        Camera::cameras[controlledCamera].updateCameraPosition(CameraDirection::FORWARD, dt);
    }
    if (Keyboard::key(GLFW_KEY_S)) {
        Camera::cameras[controlledCamera].updateCameraPosition(CameraDirection::BACKWARD, dt);
    }
    if (Keyboard::key(GLFW_KEY_D)) {
        Camera::cameras[controlledCamera].updateCameraPosition(CameraDirection::RIGHT, dt);
    }
    if (Keyboard::key(GLFW_KEY_A)) {
        Camera::cameras[controlledCamera].updateCameraPosition(CameraDirection::LEFT, dt);
    }
    if (Keyboard::key(GLFW_KEY_SPACE)) {
        Camera::cameras[controlledCamera].updateCameraPosition(CameraDirection::UP, dt);
    }
    if (Keyboard::key(GLFW_KEY_LEFT_SHIFT)) {
        Camera::cameras[controlledCamera].updateCameraPosition(CameraDirection::DOWN, dt);
    }

    double dx = Mouse::getDX(), dy = Mouse::getDY();
    if (dx != 0 || dy != 0) {
        Camera::cameras[controlledCamera].updateCameraDirection(dx, dy);
    }

    double scrollDy = Mouse::getScrollDY();
    if (scrollDy != 0) {
        Camera::cameras[controlledCamera].updateCameraZoom(scrollDy);
    }

    if (Keyboard::keyPressed(GLFW_KEY_E)) {
        launchItem(dt);
    }

    if (Keyboard::keyPressed(GLFW_KEY_Q)) {
        box.offsets.push_back(glm::vec3(box.offsets.size() * 1.0f));
        box.sizes.push_back(glm::vec3(box.sizes.size() * 0.5f));
    }
}