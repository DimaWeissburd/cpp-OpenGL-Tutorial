#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <io/Joystick.h>
#include <io/Keyboard.h>
#include <io/Mouse.h>
#include <rendering/Camera.h>
#include <rendering/Light.h>
#include <rendering/Mesh.h>
#include <rendering/Model.h>
#include <rendering/Screen.h>
#include <rendering/Shader.h>
#include <rendering/Texture.h>
#include <rendering/Models/Cube.hpp>
#include <rendering/Models/Lamp.hpp>
#include <rendering/Models/HandHeld.hpp>

void processInput(double dt);

Camera Camera::cameras[2] = {
    Camera(glm::vec3(0.0f, 0.0f, 0.0f)),
    Camera(glm::vec3(10.0f, 10.0f, 10.0f))
};
int activeCamera = 0;
int controlledCamera = 0;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool flashLightOn = true;

Joystick joystick(0);

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

Screen screen;

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

    Shader shader("assets/object.vs", "assets/object.fs");
    Shader lampShader("assets/object.vs", "assets/lamp.fs");

    HandHeld magicStaff;
    magicStaff.loadModel("assets/models/magic_staff/scene.gltf");
    magicStaff.rotation.y = -90.0f;

    Model fox;
    fox.loadModel("assets/models/fox_in_a_cape/scene.gltf");
    fox.rotation.x = -90.0f;
    fox.rotation.z = -90.0f;

    DirectionalLight directionalLight = { glm::vec3(-0.2f, -1.0f, -0.3f),
        glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), glm::vec4(0.4f, 0.4f, 0.4f, 1.0f), glm::vec4(0.75f, 0.75f, 0.75f, 1.0f) };

    glm::vec3 pointLightPositions[] = {
        glm::vec3(0.7f,  0.2f,  2.0f),
        glm::vec3(2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3(0.0f,  0.0f, -3.0f)
    };

    Lamp* lamps[4];
    for (unsigned int i = 0; i < 4; i++) {
        lamps[i] = new Lamp(glm::vec3(1.0f),
            glm::vec4(0.05f, 0.05f, 0.05f, 1.0f), glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            1.0f, 0.07f, 0.032f,
            pointLightPositions[i], glm::vec3(0.25f));

        lamps[i]->init();
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
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        processInput(deltaTime);

        screen.update();

        shader.activate();
        shader.set3Float("viewPosition", Camera::cameras[activeCamera].cameraPosition);

        float rotationSpeed = 15.0f * (float)deltaTime;
        directionalLight.direction = glm::vec3(
            glm::rotate(glm::mat4(1.0f), glm::radians(rotationSpeed), glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::vec4(directionalLight.direction, 1.0f));
        directionalLight.render(shader);

        for (int i = 0; i < 4; i++) {
            lamps[i]->pointLight.render(shader, i);
        }
        shader.setInt("noPointLights", 4);

        if (flashLightOn) {
            spotLight.position = Camera::cameras[activeCamera].cameraPosition;
            spotLight.direction = Camera::cameras[activeCamera].cameraFront;
            spotLight.render(shader, 0);
            shader.setInt("noSpotLights", 1);
        } else {
            shader.setInt("noSpotLights", 0);
        }

        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);

        view = Camera::cameras[activeCamera].getViewMatrix();

        projection = glm::perspective(glm::radians(Camera::cameras[activeCamera].getZoom()), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        fox.rotation.z += 0.1f;

        magicStaff.render(shader);
        fox.render(shader);

        lampShader.activate();
        lampShader.setMat4("view", view);
        lampShader.setMat4("projection", projection);
        for (int i = 0; i < 4; i++) {
            lamps[i]->render(lampShader);
        }

        screen.newFrame();
    }

    magicStaff.cleanup();
    fox.cleanup();
    for (int i = 0; i < 4; i++) {
        lamps[i]->cleanup();
        delete lamps[i];
    }

    glfwTerminate();
    return 0;
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
}