#include "Scene.h"
#include "Utility/IniFile.h"

#define MAX_POINT_LIGHTS 10
#define MAX_SPOT_LIGHTS 4

unsigned int Scene::screenWidth = 0;
unsigned int Scene::screenHeight = 0;

std::string Scene::generateId() {
	for (int i = currentId.length() - 1; i >= 0; i--) {
		if ((int)currentId[i] != (int)'z') {
			currentId[i] = (char)(((int)currentId[i]) + 1);
			break;
		} else {
			currentId[i] = 'a';
		}
	}
	return currentId;
}

void Scene::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	Scene::screenWidth = width;
	Scene::screenHeight = height;
}

Scene::Scene(const IniFile& ini)
: activeCamera(-1), activePointLights(0), activeSpotLights(0), directionalLight(nullptr), directionalLightActive(false),
  currentId("aaaaaaaa"), lightUBO(0) {}

Scene::Scene(const IniFile& ini, int glfwVersionMajor, int glfwVersionMinor, const char* title, unsigned int screenWidth, unsigned int screenHeight)
: glfwVersionMajor(glfwVersionMajor), glfwVersionMinor(glfwVersionMinor), title(title), activeCamera(-1),
  activePointLights(0), activeSpotLights(0), directionalLight(nullptr), directionalLightActive(false),
  currentId("aaaaaaaa"), lightUBO(0) {
	Scene::screenWidth = screenWidth;
	Scene::screenHeight = screenHeight;
	defaultFBO = FramebufferObject(screenWidth, screenHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	setWindowColor(0.1f, 0.15f, 0.15f, 1.0f);
}

bool Scene::init() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glfwVersionMajor);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glfwVersionMinor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	window = glfwCreateWindow(screenWidth, screenHeight, title, NULL, NULL);
	if (window == NULL) {
		return false;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return false;
	}

	glViewport(0, 0, screenWidth, screenHeight);

	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	glfwSetKeyCallback(window, Keyboard::keyCallback);
	glfwSetCursorPosCallback(window, Mouse::cursorPositionCallback);
	glfwSetMouseButtonCallback(window, Mouse::mouseButtonCallback);
	glfwSetScrollCallback(window, Mouse::mouseScrollCallback);

	// Depth testing
	glEnable(GL_DEPTH_TEST);

	// Blending for text textures
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Stencil testing
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // Replace fragments only if both stencil and depth pass
	
	glfwSwapInterval(1);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	models = avl_createEmptyRoot(strkeycmp);
	instances = Trie::Trie<RigidBody*>(Trie::ascii_lowercase);

	octree = new Octree::node(BoundingRegion(glm::vec3(-16.0f), glm::vec3(16.0f)));

	if (FT_Init_FreeType(&ft)) {
		std::cout << "Could not init FreeType library" << std::endl;
		return false;
	}

	fonts = avl_createEmptyRoot(strkeycmp);

	return true;
}

bool Scene::registerFont(TextRenderer* tr, std::string name, std::string path) {
	if (tr->loadFont(ft, path)) {
		fonts = avl_insert(fonts, (void*)name.c_str(), tr);
		return true;
	} else {
		return false;
	}
}

void Scene::prepare(Box& box, std::vector<Shader> shaders) {
	octree->update(box);
	lightUBO = UBO::UBO(0, {
		UBO::newStruct({ // Directional light
			UBO::Type::VEC3,
			UBO::Type::VEC4,
			UBO::Type::VEC4,
			UBO::Type::VEC4,
			UBO::Type::SCALAR,
			UBO::newColMat(4, 4)
		}),

		UBO::Type::SCALAR, // Point lights
		UBO::newArray(MAX_POINT_LIGHTS, UBO::newStruct({
			UBO::Type::VEC3,
			UBO::Type::VEC4,
			UBO::Type::VEC4,
			UBO::Type::VEC4,
			UBO::Type::SCALAR,
			UBO::Type::SCALAR,
			UBO::Type::SCALAR,
			UBO::Type::SCALAR
		})),

		UBO::Type::SCALAR, // Spot lights
		UBO::newArray(MAX_SPOT_LIGHTS, UBO::newStruct({
			UBO::Type::VEC3,
			UBO::Type::VEC3,
			UBO::Type::SCALAR,
			UBO::Type::SCALAR,
			UBO::Type::VEC4,
			UBO::Type::VEC4,
			UBO::Type::VEC4,
			UBO::Type::SCALAR,
			UBO::Type::SCALAR,
			UBO::Type::SCALAR,
			UBO::Type::SCALAR,
			UBO::Type::SCALAR,
			UBO::newColMat(4, 4)
		}))
	});
	for (Shader s : shaders) {
		lightUBO.attachToShader(s, "Lights");
	}
	lightUBO.generate();
	lightUBO.bind();
	lightUBO.initNullData(GL_STATIC_DRAW);
	lightUBO.bindRange();
	lightUBO.clear();

	updateLightUBO();
}

void Scene::updateLightUBO() {
	lightUBO.bind();
	lightUBO.startWrite();

	// Directional light
	lightUBO.writeElement<glm::vec3>(&directionalLight->direction);
	lightUBO.writeElement<glm::vec4>(&directionalLight->ambient);
	lightUBO.writeElement<glm::vec4>(&directionalLight->diffuse);
	lightUBO.writeElement<glm::vec4>(&directionalLight->specular);
	lightUBO.writeElement<float>(&directionalLight->br.max.z);
	lightUBO.writeArrayContainer<glm::mat4, glm::vec4>(&directionalLight->lightSpaceMatrix, 4);

	// Point lights
	nPointLights = 0;
	for (unsigned int k = 0; k < pointLights.size() && nPointLights < MAX_POINT_LIGHTS; k++) {
		if (States::isIndexActive(&activePointLights, k)) nPointLights++;
	}
	lightUBO.writeElement<unsigned int>(&nPointLights);

	unsigned int written = 0;
	for (unsigned int k = 0; k < pointLights.size() && written < nPointLights; k++) {
		if (!States::isIndexActive(&activePointLights, k)) continue;
		lightUBO.writeElement<glm::vec3>(&pointLights[k]->position);
		lightUBO.writeElement<glm::vec4>(&pointLights[k]->ambient);
		lightUBO.writeElement<glm::vec4>(&pointLights[k]->diffuse);
		lightUBO.writeElement<glm::vec4>(&pointLights[k]->specular);
		lightUBO.writeElement<float>(&pointLights[k]->k0);
		lightUBO.writeElement<float>(&pointLights[k]->k1);
		lightUBO.writeElement<float>(&pointLights[k]->k2);
		lightUBO.writeElement<float>(&pointLights[k]->farPlane);
		written++;
	}
	lightUBO.advanceArray(MAX_POINT_LIGHTS - written);

	// Spot lights
	nSpotLights = 0;
	for (unsigned int k = 0; k < spotLights.size() && nSpotLights < MAX_SPOT_LIGHTS; k++) {
		if (States::isIndexActive(&activeSpotLights, k)) nSpotLights++;
	}
	lightUBO.writeElement<unsigned int>(&nSpotLights);

	written = 0;
	for (unsigned int k = 0; k < spotLights.size() && written < nSpotLights; k++) {
		if (!States::isIndexActive(&activeSpotLights, k)) continue;
		lightUBO.writeElement<glm::vec3>(&spotLights[k]->position);
		lightUBO.writeElement<glm::vec3>(&spotLights[k]->direction);
		lightUBO.writeElement<float>(&spotLights[k]->cutOff);
		lightUBO.writeElement<float>(&spotLights[k]->outerCutOff);
		lightUBO.writeElement<glm::vec4>(&spotLights[k]->ambient);
		lightUBO.writeElement<glm::vec4>(&spotLights[k]->diffuse);
		lightUBO.writeElement<glm::vec4>(&spotLights[k]->specular);
		lightUBO.writeElement<float>(&spotLights[k]->k0);
		lightUBO.writeElement<float>(&spotLights[k]->k1);
		lightUBO.writeElement<float>(&spotLights[k]->k2);
		lightUBO.writeElement<float>(&spotLights[k]->nearPlane);
		lightUBO.writeElement<float>(&spotLights[k]->farPlane);
		lightUBO.writeArrayContainer<glm::mat4, glm::vec4>(&spotLights[k]->lightSpaceMatrix, 4);
		written++;
	}
}

void Scene::processInput(float dt) {
	if (activeCamera != -1 && activeCamera < cameras.size()) {
		// active camera exists

		// set camera pos/matrices, continuously poll
		if (Keyboard::key(GLFW_KEY_W)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::FORWARD, dt);
			cameraPosition = cameras[activeCamera]->cameraPosition;
			updateCameraMatrices(activeCamera);
		}
		if (Keyboard::key(GLFW_KEY_S)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::BACKWARD, dt);
			cameraPosition = cameras[activeCamera]->cameraPosition;
			updateCameraMatrices(activeCamera);
		}
		if (Keyboard::key(GLFW_KEY_D)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::RIGHT, dt);
			cameraPosition = cameras[activeCamera]->cameraPosition;
			updateCameraMatrices(activeCamera);
		}
		if (Keyboard::key(GLFW_KEY_A)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::LEFT, dt);
			cameraPosition = cameras[activeCamera]->cameraPosition;
			updateCameraMatrices(activeCamera);
		}
		if (Keyboard::key(GLFW_KEY_SPACE)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::UP, dt);
			cameraPosition = cameras[activeCamera]->cameraPosition;
			updateCameraMatrices(activeCamera);
		}
		if (Keyboard::key(GLFW_KEY_LEFT_SHIFT)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::DOWN, dt);
			cameraPosition = cameras[activeCamera]->cameraPosition;
			updateCameraMatrices(activeCamera);
		}
	}

	Keyboard::clearKeysChanged();
	Mouse::clearButtonsChanged();
}

void Scene::keyChanged(GLFWwindow* window, int key, int scancode, int action, int mods)
{

}

void Scene::cursorChanged(GLFWwindow* window, double _x, double _y)
{
	if (activeCamera != -1 && activeCamera < cameras.size()) {
		double dx = Mouse::getDX(), dy = Mouse::getDY();
		if (dx != 0 || dy != 0) {
			cameras[activeCamera]->updateCameraDirection(dx, dy);
		}

		updateCameraMatrices(activeCamera);
	}
}

void Scene::mouseButtonChanged(GLFWwindow* window, int button, int action, int mods)
{

}

void Scene::scrollChanged(GLFWwindow* window, double dx, double dy)
{
	if (activeCamera != -1 && activeCamera < cameras.size()) {
		double scrollDy = Mouse::getScrollDY();
		if (scrollDy != 0) {
			cameras[activeCamera]->updateCameraZoom(scrollDy);
		}
		updateCameraMatrices(activeCamera);
	}
}

void Scene::update() {
	glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);
	defaultFBO.clear();
}

void Scene::newFrame(Box& box) {
	box.positions.clear();
	box.sizes.clear();

	octree->processPending();
	octree->update(box);

	glfwSwapBuffers(window);
	glfwWaitEventsTimeout(0.001);
}

void Scene::renderShader(Shader shader, bool applyLighting) {
	shader.activate();

	shader.setMat4("view", view);
	shader.setMat4("projection", projection);
	shader.set3Float("viewPosition", cameraPosition);

	if (applyLighting) {
		unsigned int texturesIdx = 31;

		directionalLight->render(shader, texturesIdx--);

		unsigned int nLights = pointLights.size();
		unsigned int nActiveLights = 0;
		for (unsigned int i = 0; i < nLights; i++) {
			if (States::isIndexActive(&activePointLights, i)) {
				pointLights[i]->render(shader, nActiveLights, texturesIdx--);
				nActiveLights++;
			}
		}

		unsigned int spareCubeUnit = texturesIdx--;
		for (unsigned int i = nActiveLights; i < MAX_POINT_LIGHTS; i++) {
			shader.setInt("pointLightBuffers[" + std::to_string(i) + "]", spareCubeUnit);
		}

		nLights = spotLights.size();
		nActiveLights = 0;
		for (unsigned int i = 0; i < nLights; i++) {
			if (States::isIndexActive(&activeSpotLights, i)) {
				spotLights[i]->render(shader, nActiveLights, texturesIdx--);
				nActiveLights++;
			}
		}

		unsigned int spare2DUnit = texturesIdx--;
		for (unsigned int i = nActiveLights; i < MAX_SPOT_LIGHTS; i++) {
			shader.setInt("spotLightBuffers[" + std::to_string(i) + "]", spare2DUnit);
		}
	}
}

void Scene::renderDirectionalLightShader(Shader shader) {
	shader.activate();
	shader.setMat4("lightSpaceMatrix", directionalLight->lightSpaceMatrix);
}

void Scene::renderPointLightShader(Shader shader, unsigned int index) {
	shader.activate();
	for (unsigned int i = 0; i < 6; i++) {
		shader.setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", pointLights[index]->lightSpaceMatrices[i]);
	}
	shader.set3Float("lightPosition", pointLights[index]->position);
	shader.setFloat("farPlane", pointLights[index]->farPlane);
}

void Scene::renderSpotLightShader(Shader shader, unsigned int index) {
	shader.activate();
	shader.setMat4("lightSpaceMatrix", spotLights[index]->lightSpaceMatrix);
	shader.set3Float("lightPosition", spotLights[index]->position);
	shader.setFloat("farPlane", spotLights[index]->farPlane);
}

void Scene::renderInstances(std::string modelId, Shader shader, float dt) {
	void* val = avl_get(models, (void*)modelId.c_str());
	if (val) {
		shader.activate();
		((Model*)val)->render(shader, dt, this);
	}
}

void Scene::renderText(std::string font, Shader shader, std::string text, float x, float y, glm::vec2 scale, glm::vec3 color) {
	void* val = avl_get(fonts, (void*)font.c_str());
	if (val) {
		shader.activate();
		shader.setMat4("projection", textProjection);
		((TextRenderer*)val)->render(shader, text, x, y, scale, color);
	}
}

void Scene::cleanup() {
	instances.cleanup();

	avl_postorderTraverse(models, [](avl* node) -> void {
		((Model*)node->val)->cleanup();
	});

	avl_free(models);

	avl_postorderTraverse(fonts, [](avl* node) -> void {
		((TextRenderer*)node->val)->cleanup();
	});

	avl_free(fonts);

	octree->destroy();

	glfwTerminate();
}

bool Scene::shouldClose() {
	return glfwWindowShouldClose(window);
}

Camera* Scene::getActiveCamera() {
	return (activeCamera >= 0 && activeCamera < cameras.size()) ? cameras[activeCamera] : nullptr;
}

void Scene::setShouldClose(bool shouldClose) {
	glfwSetWindowShouldClose(window, shouldClose);
}

void Scene::setWindowColor(float r, float g, float b, float a) {
	backgroundColor[0] = r;
	backgroundColor[1] = g;
	backgroundColor[2] = b;
	backgroundColor[3] = a;
}

void Scene::updateCameraMatrices(int idx)
{
	// set matrices
	view = cameras[activeCamera]->getViewMatrix();
	projection = glm::perspective(
		glm::radians(cameras[activeCamera]->getZoom()),
		(float)screenWidth / (float)screenHeight,
		0.1f, 100.0f
	);
	textProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
}

void Scene::registerModel(Model* model) {
	models = avl_insert(models, (void*)model->id.c_str(), model);
}

RigidBody* Scene::generateInstance(std::string modelId, glm::vec3 position, glm::vec3 rotation, glm::vec3 size, float mass) {
	void* val = avl_get(models, (void*)modelId.c_str());
	if (val) {
		Model* model = (Model*)val;
		RigidBody* rigidBody = model->generateInstance(position, rotation, size, mass);
		if (rigidBody) {
			std::string id = generateId();
			rigidBody->instanceId = id;
			instances.insert(rigidBody->instanceId, rigidBody);
			octree->addToPending(rigidBody, model);
			return rigidBody;
		}
	}
	return nullptr;
}

void Scene::initInstances() {
	avl_inorderTraverse(models, [](avl* node) -> void {
		((Model*)node->val)->initInstances();
	});
}

void Scene::loadModels() {
	avl_inorderTraverse(models, [](avl* node) -> void {
		((Model*)node->val)->init();
	});
}

void Scene::removeInstance(std::string instanceId) {
	RigidBody* instance = instances[instanceId];
	std::string targetModel = instance->modelId;
	Model* model = (Model*)avl_get(models, (void*)targetModel.c_str());
	model->removeInstance(instanceId);
	instances[instanceId] = NULL;
	instances.erase(instanceId);
	free(instance);
}

void Scene::markForDeletion(std::string instanceId) {
	RigidBody* instance = instances[instanceId];
	States::activate(&instance->state, INSTANCE_DEAD);
	instancesToDelete.push_back(instance);
}

void Scene::clearDeadInstances() {
	for (RigidBody* rb : instancesToDelete) {
		removeInstance(rb->instanceId);
	}
	instancesToDelete.clear();
}