#ifndef MOUSE_H
#define MOUSE_H

#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Mouse {
public:
    static std::vector<void(*)(GLFWwindow* window, double _x, double _y)> cursorPositionCallbacks;
    static std::vector<void(*)(GLFWwindow* window, int button, int action, int mods)> mouseButtonCallbacks;
    static std::vector<void(*)(GLFWwindow* window, double dx, double dy)> mouseScrollCallbacks;

    static void cursorPositionCallback(GLFWwindow* window, double _x, double _y);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void mouseScrollCallback(GLFWwindow* window, double dx, double dy);
    static double getMouseX();
    static double getMouseY();
    static double getDX();
    static double getDY();
    static double getScrollDX();
    static double getScrollDY();
    static bool button(int button);
    static bool buttonChanged(int button);
    static bool buttonPressed(int button);
    static bool buttonReleased(int button);
    static void clearButtonsChanged();

private:
    static double x;
    static double y;
    static double lastX;
    static double lastY;
    static double dx;
    static double dy;
    static double scrollDX;
    static double scrollDY;
    static bool firstMouse;
    static bool buttons[];
    static bool buttonsChanged[];
};

#endif