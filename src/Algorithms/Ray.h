#ifndef RAY_H
#define RAY_H

#include <glm/glm.hpp>

#include "Bounds.h"
#include "../Physics/CollisionMesh.h"
#include "../Physics/RigidBody.h"

class Ray {
public:
	glm::vec3 origin;
	glm::vec3 direction;
	glm::vec3 inverseDirection;

	Ray(glm::vec3 origin, glm::vec3 direction);

	bool intersectsBoundingRegion(BoundingRegion boundingRegion, float& tmin, float& tmax);
	bool intersectsMesh(CollisionMesh* mesh, RigidBody* rigidBody, float& t);
};

#endif