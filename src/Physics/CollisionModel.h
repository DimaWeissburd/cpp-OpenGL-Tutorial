#ifndef COLLISIONMODEL_H
#define COLLISIONMODEL_H

#include "CollisionMesh.h"

#include <vector>

class Model;

class CollisionModel {
public:
	Model* model;

	std::vector<CollisionMesh> meshes;

	CollisionModel(Model* model);
};

#endif