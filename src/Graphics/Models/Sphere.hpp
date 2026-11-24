#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "../Model.h"

class Sphere : public Model {
public:
	Sphere(unsigned int maxNoInstances, glm::vec3 rotation, glm::vec3 size, float mass)
		: Model("sphere", maxNoInstances, glm::vec3(0.0f), rotation, size, mass, NO_TEX | DYNAMIC) {}

	void init() {
		loadModel("Assets/Models/the_sphere/scene.gltf");
	}
};

#endif