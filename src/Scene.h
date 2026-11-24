#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <jsoncpp/json.hpp>
#include <Graphics/Camera.h>
#include <Graphics/FrameMemory.hpp>
#include <Graphics/Light.h>
#include <Graphics/Model.h>
#include <Graphics/Shader.h>
#include <Graphics/Text.h>
#include <Graphics/UniformMemory.hpp>
#include <Graphics/Models/Box.hpp>
#include <IO/Keyboard.h>
#include <IO/Mouse.h>
#include <Algorithms/Avl.h>
#include <Algorithms/Octree.h>
#include <Algorithms/States.hpp>
#include <Algorithms/Trie.hpp>

namespace Octree {
	class node;
}
class Model;

class IniFile;

class Scene {
public:
	avl* models;
	Trie::Trie<RigidBody*> instances;

	std::vector<RigidBody*> instancesToDelete;

	Octree::node* octree;

	jsoncpp::json variableLog;

	FT_Library ft;
	avl* fonts;

	FramebufferObject defaultFBO;

	UBO::UBO lightUBO;

	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

	Scene(const IniFile& ini);
	Scene(const IniFile& ini, int glfwVersionMajor, int glfwVersionMinor, const char* title, unsigned int screenWidth, unsigned int screenHeight);

	bool init();

	bool registerFont(TextRenderer* tr, std::string name, std::string path);

	void prepare(Box& box, std::vector<Shader> shaders);

	void updateLightUBO();

	void processInput(float dt);
	void keyChanged(GLFWwindow* window, int key, int scancode, int action, int mods);
	void cursorChanged(GLFWwindow* window, double _x, double _y);
	void mouseButtonChanged(GLFWwindow* window, int button, int action, int mods);
	void scrollChanged(GLFWwindow* window, double dx, double dy);

	void update();
	void newFrame(Box& box);
	void renderShader(Shader shader, bool applyLighting = true);
	void renderDirectionalLightShader(Shader shader);
	void renderPointLightShader(Shader shader, unsigned int index);
	void renderSpotLightShader(Shader shader, unsigned int index);
	void renderInstances(std::string modelId, Shader shader, float dt);
	void renderText(std::string font, Shader shader, std::string text, float x, float y, glm::vec2 scale, glm::vec3 color);

	void cleanup();

	bool shouldClose();
	Camera* getActiveCamera();

	void setShouldClose(bool shouldClose);
	void setWindowColor(float r, float g, float b, float a);

	void updateCameraMatrices(int index);

	void registerModel(Model* model);
	RigidBody* generateInstance(std::string modelId, glm::vec3 position = glm::vec3(0.0f), glm::vec3 rotation = glm::vec3(0.0f), glm::vec3 size = glm::vec3(1.0f), float mass = 1.0f);
	void initInstances();
	void loadModels();
	void removeInstance(std::string instanceId);
	void markForDeletion(std::string instanceId);
	void clearDeadInstances();

	std::string currentId;
	std::string generateId();

	std::vector<PointLight*> pointLights;
	unsigned int activePointLights;
	unsigned int nPointLights;
	std::vector<SpotLight*> spotLights;
	unsigned int activeSpotLights;
	unsigned int nSpotLights;
	DirectionalLight* directionalLight;
	bool directionalLightActive;

	std::vector<Camera*> cameras;
	int activeCamera;
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 textProjection;
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