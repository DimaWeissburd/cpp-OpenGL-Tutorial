#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <../src/io/Joystick.h>
#include <../src/io/Keyboard.h>
#include <../src/io/Mouse.h>
#include <../src/rendering/Camera.h>
#include <../src/rendering/Light.h>
#include <../src/rendering/Mesh.h>
#include <../src/rendering/Model.h>
#include <../src/rendering/Screen.h>
#include <../src/rendering/Shader.h>
#include <../src/rendering/Texture.h>
#include <../src/rendering/Models/Cube.hpp>
#include <../src/rendering/Models/Lamp.hpp>

void processInput(double dt);

Camera cameras[2] = {
    Camera(glm::vec3(0.0f, 0.0f, 3.0f)),
    Camera(glm::vec3(10.0f, 10.0f, 10.0f))
};
int activeCamera = 0;

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

    Shader shader("../assets/object.vs", "../assets/object.fs");
    Shader lampShader("../assets/object.vs", "../assets/lamp.fs");

    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f,  2.0f, -2.5f),
        glm::vec3(1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    Cube cubes[10];
    for (unsigned int i = 0; i < 10; i++) {
        cubes[i] = Cube(Material::gold, cubePositions[i], glm::vec3(1.0f));
        cubes[i].init();
    }

    glm::vec3 pointLightPositions[] = {
        glm::vec3(0.7f,  0.2f,  2.0f),
        glm::vec3(2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3(0.0f,  0.0f, -3.0f)
    };
    Lamp lamps[4];
    for (unsigned int i = 0; i < 4; i++) {
        lamps[i] = Lamp(glm::vec3(1.0f),
            glm::vec3(0.05f), glm::vec3(0.8f), glm::vec3(1.0f),
            1.0f, 0.07f, 0.032f,
            pointLightPositions[i], glm::vec3(0.25f));
        lamps[i].init();
    }

    DirectionalLight directionalLight = {glm::vec3(-0.2f, -1.0f, -0.3f),
        glm::vec3(0.1f), glm::vec3(0.4f), glm::vec3(0.75f)};

    SpotLight spotLight = {
        cameras[activeCamera].cameraPosition, cameras[activeCamera].cameraFront,
        glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(20.0f)),
        1.0f, 0.07f, 0.032f,
        glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(1.0f)
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
        shader.set3Float("viewPosition", cameras[activeCamera].cameraPosition);

        directionalLight.direction = glm::vec3(
            glm::rotate(glm::mat4(1.0f), glm::radians(0.5f), glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::vec4(directionalLight.direction, 1.0f));
        directionalLight.render(shader);

        for (int i = 0; i < 4; i++) {
            lamps[i].pointLight.render(shader, i);
        }
        shader.setInt("noPointLights", 4);

        if (flashLightOn) {
            spotLight.position = cameras[activeCamera].cameraPosition;
            spotLight.direction = cameras[activeCamera].cameraFront;
            spotLight.render(shader, 0);
            shader.setInt("noSpotLights", 1);
        } else {
            shader.setInt("noSpotLights", 0);
        }

        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);

        view = cameras[activeCamera].getViewMatrix();
        projection = glm::perspective(glm::radians(cameras[activeCamera].getZoom()), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        for (int i = 0; i < 10; i++) {
            cubes[i].render(shader);
        }

        lampShader.activate();
        lampShader.setMat4("view", view);
        lampShader.setMat4("projection", projection);
        for (int i = 0; i < 4; i++) {
            lamps[i].render(lampShader);
        }

        screen.newFrame();
    }

    for (int i = 0; i < 10; i++) {
        cubes[i].cleanup();
    }
    for (int i = 0; i < 4; i++) {
        lamps[i].cleanup();
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

    if (Keyboard::keyPressed(GLFW_KEY_F)) {
        flashLightOn = !flashLightOn;
    }

    if (Keyboard::key(GLFW_KEY_W)) {
        cameras[activeCamera].updateCameraPosition(CameraDirection::FORWARD, dt);
    }
    if (Keyboard::key(GLFW_KEY_S)) {
        cameras[activeCamera].updateCameraPosition(CameraDirection::BACKWARD, dt);
    }
    if (Keyboard::key(GLFW_KEY_D)) {
        cameras[activeCamera].updateCameraPosition(CameraDirection::RIGHT, dt);
    }
    if (Keyboard::key(GLFW_KEY_A)) {
        cameras[activeCamera].updateCameraPosition(CameraDirection::LEFT, dt);
    }
    if (Keyboard::key(GLFW_KEY_SPACE)) {
        cameras[activeCamera].updateCameraPosition(CameraDirection::UP, dt);
    }
    if (Keyboard::key(GLFW_KEY_LEFT_SHIFT)) {
        cameras[activeCamera].updateCameraPosition(CameraDirection::DOWN, dt);
    }

    double dx = Mouse::getDX(), dy = Mouse::getDY();
    if (dx != 0 || dy != 0) {
        cameras[activeCamera].updateCameraDirection(dx, dy);
    }

    double scrollDy = Mouse::getScrollDY();
    if (scrollDy != 0) {
        cameras[activeCamera].updateCameraZoom(scrollDy);
    }
}