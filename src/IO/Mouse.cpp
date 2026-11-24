#include "Mouse.h"

std::vector<void(*)(GLFWwindow* window, double _x, double _y)> Mouse::cursorPositionCallbacks;
std::vector<void(*)(GLFWwindow* window, int button, int action, int mods)> Mouse::mouseButtonCallbacks;
std::vector<void(*)(GLFWwindow* window, double dx, double dy)> Mouse::mouseScrollCallbacks;

double Mouse::x = 0;
double Mouse::y = 0;
double Mouse::lastX = 0;
double Mouse::lastY = 0;
double Mouse::dx = 0;
double Mouse::dy = 0;
double Mouse::scrollDX = 0;
double Mouse::scrollDY = 0;
bool Mouse::firstMouse = true;
bool Mouse::buttons[GLFW_MOUSE_BUTTON_LAST] = { 0 };
bool Mouse::buttonsChanged[GLFW_MOUSE_BUTTON_LAST] = { 0 };

void Mouse::cursorPositionCallback(GLFWwindow* window, double _x, double _y) {
    x = _x;
    y = _y;
    if (firstMouse) {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }
    dx = x - lastX;
    dy = lastY - y;
    lastX = x;
    lastY = y;

    for (void(*func)(GLFWwindow*, double, double) : Mouse::cursorPositionCallbacks)
    {
        func(window, _x, _y);
    }
}

void Mouse::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_RELEASE) {
        if (!buttons[button]) {
            buttons[button] = true;
        }
    } else {
        buttons[button] = false;
    }
    buttonsChanged[button] = action != GLFW_REPEAT;

    for (void(*func)(GLFWwindow*, int, int, int) : Mouse::mouseButtonCallbacks)
    {
        func(window, button, action, mods);
    }
}

void Mouse::mouseScrollCallback(GLFWwindow* window, double dx, double dy) {
    scrollDX = dx;
    scrollDY = dy;

    for (void(*func)(GLFWwindow*, double, double) : Mouse::mouseScrollCallbacks)
    {
        func(window, dx, dy);
    }
}

double Mouse::getMouseX() {
    return x;
}

double Mouse::getMouseY() {
    return y;
}

double Mouse::getDX() {
    double _dx = dx;
    dx = 0;
    return _dx;
}

double Mouse::getDY() {
    double _dy = dy;
    dy = 0;
    return _dy;
}

double Mouse::getScrollDX() {
    double _scrollDX = scrollDX;
    scrollDX = 0;
    return _scrollDX;
}

double Mouse::getScrollDY() {
    double _scrollDY = scrollDY;
    scrollDY = 0;
    return _scrollDY;
}

bool Mouse::button(int button) {
    return buttons[button];
}

bool Mouse::buttonChanged(int button) {
    bool ret = buttonsChanged[button];
    buttonsChanged[button] = false;
    return ret;
}

bool Mouse::buttonPressed(int button) {
    return buttons[button] && buttonChanged(button);
}

bool Mouse::buttonReleased(int button) {
    return !buttons[button] && buttonChanged(button);
}

void Mouse::clearButtonsChanged() {
    memset(buttonsChanged, 0, (size_t)GLFW_MOUSE_BUTTON_LAST);
}