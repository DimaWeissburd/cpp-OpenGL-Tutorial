#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "../Model.h"

class Sphere : public Model {
public:
	Sphere(unsigned int maxNoInstances)
		: Model("sphere", BoundTypes::SPHERE, maxNoInstances, NO_TEX | DYNAMIC) {}

	void init() {
		loadModel("Assets/Models/the_sphere/scene.gltf");
	}
};

#endif