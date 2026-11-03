#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <Rendering/Camera.h>
#include <Rendering/Model.h>
#include <Rendering/Light.h>
#include <Rendering/Shader.h>
#include <Rendering/Models/Box.hpp>
#include <IO/Keyboard.h>
#include <IO/Mouse.h>
#include <Algorithms/Octree.h>
#include <Algorithms/States.hpp>
#include <Algorithms/Trie.hpp>

namespace Octree {
	class node;
}
class Model;

class Scene {
public:
	Trie::Trie<Model*> models;
	Trie::Trie<RigidBody*> instances;

	std::vector<RigidBody*> instancesToDelete;

	Octree::node* octree;

	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

	Scene();
	Scene(int glfwVersionMajor, int glfwVersionMinor, const char* title, unsigned int screenWidth, unsigned int screenHeight);

	bool init();

	void prepare(Box& box);

	void processInput(float dt);
	void update();
	void newFrame(Box& box);
	void renderShader(Shader shader, bool applyLighting = true);
	void renderInstances(std::string modelId, Shader shader, float dt);

	void cleanup();

	bool shouldClose();
	Camera* getActiveCamera();

	void setShouldClose(bool shouldClose);
	void setWindowColor(float r, float g, float b, float a);

	void registerModel(Model* model);
	RigidBody* generateInstance(std::string modelId, glm::vec3 size, float mass, glm::vec3 position);
	void initInstances();
	void loadModels();
	void removeInstance(std::string instanceId);
	void markForDeletion(std::string instanceId);
	void clearDeadInstances();

	std::string currentId;
	std::string generateId();

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