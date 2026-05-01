#include "Screen.h"

#include <../src/io/Keyboard.h>
#include <../src/io/Mouse.h>

unsigned int Screen::SCR_WIDTH = 800;
unsigned int Screen::SCR_HEIGHT = 600;

void Screen::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

Screen::Screen() : window(nullptr) {}

bool Screen::init() {
    window = glfwCreateWindow(Screen::SCR_WIDTH, Screen::SCR_HEIGHT, "OpenGL Tutorial", NULL, NULL);
    if (!window) {
        return false;
    }
    glfwMakeContextCurrent(window);
    return true;
}

void Screen::setParameters() {
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetFramebufferSizeCallback(window, Screen::framebufferSizeCallback);
    glfwSetKeyCallback(window, Keyboard::keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, Mouse::cursorPosCallback);
    glfwSetMouseButtonCallback(window, Mouse::mouseButtonCallback);
    glfwSetScrollCallback(window, Mouse::mouseScrollCallback);
    glEnable(GL_DEPTH_TEST);
}

void Screen::update() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Screen::newFrame() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

bool Screen::shouldClose() {
    return glfwWindowShouldClose(window);
}

void Screen::setShouldClose(bool shouldClose) {
    glfwSetWindowShouldClose(window, shouldClose);
}