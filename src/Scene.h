#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include <Rendering/Light.h>
#include <Rendering/Shader.h>
#include <Rendering/Camera.h>
#include <IO/Keyboard.h>
#include <IO/Mouse.h>
#include <Algorithms/States.hpp>

class Scene {
public:
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

	Scene();
	Scene(int glfwVersionMajor, int glfwVersionMinor, const char* title, unsigned int screenWidth, unsigned int screenHeight);

	bool init();

	void processInput(float dt);
	void update();
	void newFrame();
	void render(Shader shader, bool applyLighting = true);

	void cleanup();

	bool shouldClose();
	Camera* getActiveCamera();

	void setShouldClose(bool shouldClose);
	void setWindowColor(float r, float g, float b, float a);

	std::vector<PointLight*> pointLights;
	unsigned int activePointLights;
	std::vector<SpotLight*> spotLights;
	unsigned int activeSpotLights;
	DirectionalLight* directionalLight;
	bool dirLightActive;

	std::vector<Camera*> cameras;
	int activeCamera;
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec3 cameraPosition;
protected:
	GLFWwindow* window;
	const char* title;
	static unsigned int screenWidth;
	static unsigned int screenHeight;
	float backgroundColor[4];
	int glfwVersionMajor;
	int glfwVersionMinor;
};

#endif