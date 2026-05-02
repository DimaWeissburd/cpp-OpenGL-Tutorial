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

    Cube cube(Material::mix(Material::gold, Material::emerald), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.75f));
    cube.init();

    Lamp lamp(glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(-1.0f, -0.5f, -0.5f), glm::vec3(0.25f));
    lamp.init();

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
        shader.set3Float("light.position", lamp.position);
        shader.set3Float("viewPosition", cameras[activeCamera].cameraPosition);

        shader.set3Float("light.ambient", lamp.ambient);
        shader.set3Float("light.diffuse", lamp.diffuse);
        shader.set3Float("light.specular", lamp.specular);

        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);

        view = cameras[activeCamera].getViewMatrix();
        projection = glm::perspective(glm::radians(cameras[activeCamera].getZoom()), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        cube.render(shader);

        lampShader.activate();
        lampShader.setMat4("view", view);
        lampShader.setMat4("projection", projection);
        lamp.render(lampShader);

        screen.newFrame();
    }

    cube.cleanup();
    lamp.cleanup();

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