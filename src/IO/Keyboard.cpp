#include "Keyboard.h"

std::vector<void(*)(GLFWwindow* window, int key, int scancode, int action, int mods)> Keyboard::keyCallbacks;

bool Keyboard::keys[GLFW_KEY_LAST] = { 0 };
bool Keyboard::keysChanged[GLFW_KEY_LAST] = { 0 };

void Keyboard::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_RELEASE) {
        if (!keys[key]) {
            keys[key] = true;
        }
    } else {
        keys[key] = false;
    }

    keysChanged[key] = action != GLFW_REPEAT;

    for (void(*func)(GLFWwindow*, int, int, int, int) : Keyboard::keyCallbacks)
    {
        func(window, key, scancode, action, mods);
    }
}
bool Keyboard::key(int key) {
    return keys[key];
}

bool Keyboard::keyChanged(int key) {
    bool ret = keysChanged[key];
    keysChanged[key] = false;
    return ret;
}

bool Keyboard::keyPressed(int key) {
    return keys[key] && keyChanged(key);
}

bool Keyboard::keyReleased(int key) {
    return !keys[key] && keyChanged(key);
}

void Keyboard::clearKeysChanged() {
    memset(keysChanged, 0, (size_t)GLFW_KEY_LAST);
}