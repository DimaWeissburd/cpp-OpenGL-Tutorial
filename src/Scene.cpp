#include "Scene.h"
#include "Utility/IniFile.h"

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
: activeCamera(-1), activePointLights(0), activeSpotLights(0), directionalLight(nullptr), dirLightActive(false),
  currentId("aaaaaaaa"), useGammaCorrection(ini.getBool("graphics", "use_gamma_correction", true)) {}

Scene::Scene(const IniFile& ini, int glfwVersionMajor, int glfwVersionMinor, const char* title, unsigned int screenWidth, unsigned int screenHeight)
: glfwVersionMajor(glfwVersionMajor), glfwVersionMinor(glfwVersionMinor), title(title), activeCamera(-1),
  activePointLights(0), activeSpotLights(0), directionalLight(nullptr), dirLightActive(false),
  currentId("aaaaaaaa"), useGammaCorrection(ini.getBool("graphics", "use_gamma_correction", true)) {
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
	glfwSetCursorPosCallback(window, Mouse::cursorPosCallback);
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


	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	octree = new Octree::node(BoundingRegion(glm::vec3(-16.0f), glm::vec3(16.0f)));

	if (FT_Init_FreeType(&ft)) {
		std::cout << "Could not init FreeType library" << std::endl;
		return false;
	}

	fonts.insert("Helvetica", TextRenderer(32));
	if (!fonts["Helvetica"].loadFont(ft, "Assets/Fonts/Helvetica.ttf")) {
		std::cout << "Could not load font" << std::endl;
		return false;
	}

	FT_Done_FreeType(ft);

	variableLog["useGammaCorrection"] = useGammaCorrection;

	return true;
}

void Scene::prepare(Box& box) {
	octree->update(box);
}

void Scene::processInput(float dt) {
	if (activeCamera != -1 && activeCamera < cameras.size()) {
		double dx = Mouse::getDX();
		double dy = Mouse::getDY();
		if (dx != 0 || dy != 0) {
			cameras[activeCamera]->updateCameraDirection(dx, dy);
		}

		double scrollDy = Mouse::getScrollDY();
		if (scrollDy != 0) {
			cameras[activeCamera]->updateCameraZoom(scrollDy);
		}

		if (Keyboard::key(GLFW_KEY_W)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::FORWARD, dt);
		}
		if (Keyboard::key(GLFW_KEY_S)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::BACKWARD, dt);
		}
		if (Keyboard::key(GLFW_KEY_D)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::RIGHT, dt);
		}
		if (Keyboard::key(GLFW_KEY_A)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::LEFT, dt);
		}
		if (Keyboard::key(GLFW_KEY_SPACE)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::UP, dt);
		}
		if (Keyboard::key(GLFW_KEY_LEFT_SHIFT)) {
			cameras[activeCamera]->updateCameraPosition(CameraDirection::DOWN, dt);
		}

		view = cameras[activeCamera]->getViewMatrix();
		projection = glm::perspective(
			glm::radians(cameras[activeCamera]->getZoom()),
			(float)screenWidth / (float)screenHeight,
			0.1f, 100.0f
		);
		textProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);

		cameraPosition = cameras[activeCamera]->cameraPosition;
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
	glfwPollEvents();
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
				pointLights[i]->render(shader, nActiveLights);
				nActiveLights++;
			}
		}
		shader.setInt("nPointLights", nActiveLights);

		nLights = spotLights.size();
		nActiveLights = 0;
		for (unsigned int i = 0; i < nLights; i++) {
			if (States::isIndexActive(&activeSpotLights, i)) {
				spotLights[i]->render(shader, nActiveLights, texturesIdx--);
				nActiveLights++;
			}
		}
		shader.setInt("nSpotLights", nActiveLights);

		shader.setBool("useGammaCorrection", variableLog["useGammaCorrection"].val<bool>());
	}
}

void Scene::renderDirectionalLightShader(Shader shader) {
	shader.activate();
	shader.setMat4("lightSpaceMatrix", directionalLight->lightSpaceMatrix);
}

void Scene::renderSpotLightShader(Shader shader, unsigned int index) {
	shader.activate();
	shader.setMat4("lightSpaceMatrix", spotLights[index]->lightSpaceMatrix);
}

void Scene::renderInstances(std::string modelId, Shader shader, float dt) {
	shader.activate();
	models[modelId]->render(shader, dt, this);
}

void Scene::renderText(std::string font, Shader shader, std::string text, float x, float y, glm::vec2 scale, glm::vec3 color) {
	shader.activate();
	shader.setMat4("projection", textProjection);
	fonts[font].render(shader, text, x, y, scale, color);
}

void Scene::cleanup() {
	models.traverse([](Model* model) -> void {
		model->cleanup();
	});

	models.cleanup();
	instances.cleanup();

	fonts.traverse([](TextRenderer tr) -> void {
		tr.cleanup();
	});

	fonts.cleanup();

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

void Scene::registerModel(Model* model) {
	models.insert(model->id, model);
}

RigidBody* Scene::generateInstance(std::string modelId, glm::vec3 position, glm::vec3 size, float mass) {
	RigidBody* rigidBody = models[modelId]->generateInstance(position, size, mass);
	if (rigidBody) {
		std::string id = generateId();
		rigidBody->instanceId = id;
		instances.insert(id, rigidBody);
		octree->addToPending(rigidBody, models);
		return rigidBody;
	}
	return nullptr;
}

void Scene::initInstances() {
	models.traverse([](Model* model) -> void {
		model->initInstances();
	});
}

void Scene::loadModels() {
	models.traverse([](Model* model) -> void {
		model->init();
	});
}

void Scene::removeInstance(std::string instanceId) {
	RigidBody* rb = instances[instanceId];
	if (!rb) return;
	models[rb->modelId]->removeInstance(instanceId);
	instances.erase(instanceId);
}

void Scene::markForDeletion(std::string instanceId) {
	RigidBody* instance = instances[instanceId];
	if (!instance || States::isActive(&instance->state, INSTANCE_DEAD)) return;
	States::activate(&instance->state, INSTANCE_DEAD);
	instancesToDelete.push_back(instance);
}

void Scene::clearDeadInstances() {
	for (RigidBody* rb : instancesToDelete) {
		removeInstance(rb->instanceId);
	}
	instancesToDelete.clear();
}