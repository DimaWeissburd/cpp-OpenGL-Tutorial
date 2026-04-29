#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <../src/Shader.h>
#include <../src/io/Keyboard.h>
#include <../src/io/Mouse.h>
#include <../src/io/Joystick.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

float mixVal = 0.5f;

glm::mat4 transform = glm::mat4(1.0f);
Joystick mainJoystick(0);

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPEGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Tutorial", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD." << std::endl;
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, 800, 600);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetKeyCallback(window, Keyboard::keyCallback);
    glfwSetCursorPosCallback(window, Mouse::cursorPosCallback);
    glfwSetMouseButtonCallback(window, Mouse::mouseButtonCallback);
    glfwSetScrollCallback(window, Mouse::mouseScrollCallback);

    Shader shader("../assets/vertex_core.glsl", "../assets/fragment_core.glsl");

    float vertices[] = {
        // Positions            Colors                 Texture coordinates
        -0.5f,  -0.5f, 0.0f,    1.0f, 1.0f, 0.5f,      0.0f, 0.0f,
        -0.5f,   0.5f, 0.0f,    0.5f, 1.0f, 0.75f,     0.0f, 1.0f,
         0.5f,  -0.5f, 0.0f,    0.6f, 1.0f, 0.2f,      1.0f, 0.0f,
         0.5f,   0.5f, 0.0f,    1.0f, 0.2f, 1.0f,      1.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        3, 1, 2
    };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int texture1, texture2;

    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    int width, height, nChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("../assets/Rick.png", &width, &height, &nChannels, 0);

    if (data)  {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture." << std::endl;
    }

    stbi_image_free(data);

    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    data = stbi_load("../assets/Cthulhu.png", &width, &height, &nChannels, 0);

    if (data)  {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture." << std::endl;
    }

    stbi_image_free(data);

    shader.activate();
    shader.setInt("texture1", 0);
    shader.setInt("texture2", 1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::rotate(trans, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    shader.activate();
    shader.setMat4("transform", trans);

    mainJoystick.update();
    if (mainJoystick.isPresent()) {
        std::cout << "Joystick " << mainJoystick.getName() << " is present." << std::endl;
    } else {
        std::cout << "Joystick not present." << std::endl;
    }

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        glBindVertexArray(VAO);
        shader.activate();

        float timeValue = glfwGetTime();
        trans = glm::rotate(trans, glm::radians(timeValue / 100), glm::vec3(0.1f, 0.1f, 0.1f));
        shader.setMat4("transform", trans);

        shader.setFloat("mixVal", mixVal);
        shader.setMat4("transform", transform);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (Keyboard::key(GLFW_KEY_ESCAPE) || mainJoystick.buttonState(GLFW_JOYSTICK_BTN_DOWN)) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    float lx = mainJoystick.axesState(GLFW_JOYSTICK_AXES_LEFT_STICK_X);
    float ly = -mainJoystick.axesState(GLFW_JOYSTICK_AXES_LEFT_STICK_Y);
    float rx = mainJoystick.axesState(GLFW_JOYSTICK_AXES_RIGHT_STICK_X);
    float ry = -mainJoystick.axesState(GLFW_JOYSTICK_AXES_RIGHT_STICK_Y);
    float rt = mainJoystick.axesState(GLFW_JOYSTICK_AXES_RIGHT_TRIGGER) / 2 + 0.5f;
    float lt = -mainJoystick.axesState(GLFW_JOYSTICK_AXES_LEFT_TRIGGER) / 2 + 0.5f;

    // std::cout << "Left Stick: x-" << lx << ", y-" << ly << std::endl;
    // std::cout << "Right Stick: x-" << rx << ", y-" << ry << std::endl;
    // std::cout << "Triggers: Left-" << lt << ", Right-" << rt << std::endl;

    if (Keyboard::key(GLFW_KEY_W)) {
        transform = glm::translate(transform, glm::vec3(0.0f, 0.01f, 0.0f));
    }
    if (Keyboard::key(GLFW_KEY_S)) {
        transform = glm::translate(transform, glm::vec3(0.0f, -0.01f, 0.0f));
    }
    if (Keyboard::key(GLFW_KEY_A)) {
        transform = glm::translate(transform, glm::vec3(-0.01f, 0.0f, 0.0f));
    }
    if (Keyboard::key(GLFW_KEY_D)) {
        transform = glm::translate(transform, glm::vec3(0.01f, 0.0f, 0.0f));
    }

    if (std::abs(lx) > 0.05f) {
        transform = glm::translate(transform, glm::vec3(lx / 50.0f, 0.0f, 0.0f));
    }
    if (std::abs(ly) > 0.05f) {
        transform = glm::translate(transform, glm::vec3(0.0f, ly / 50.0f, 0.0f));
    }

    // if (rt > 0.05f) {
    //     transform = glm::scale(transform, glm::vec3(1 + rt / 10.0f, 1 + rt / 10.0f, 0.0f));
    // }
    // if (lt > 0.05f) {
    //     transform = glm::scale(transform, glm::vec3(1 - lt / 50.0f, 1 - lt / 10.0f, 0.0f));
    // }

    if (Keyboard::key(GLFW_KEY_UP)) {
        mixVal += 0.005f;
        if (mixVal > 1.0f) {
            mixVal = 1.0f;
        }
    }
    if (Keyboard::key(GLFW_KEY_DOWN)) {
        mixVal -= 0.005f;
        if (mixVal < 0.0f) {
            mixVal = 0.0f;
        }
    }

    // if (std::abs(ry) > 0.05f) {
    //     if (mixVal > 0.0f && mixVal < 1.0f) {
    //         mixVal += -ry / 100.0f;
    //     }
    // }

    mainJoystick.update();
}