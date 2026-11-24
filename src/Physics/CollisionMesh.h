#ifndef COLLISIONMESH_H
#define COLLISIONMESH_H

#include <vector>

#include "../Algorithms/Bounds.h"

class CollisionModel;
class CollisionMesh;
class RigidBody;

typedef struct Face {
	CollisionMesh* mesh;
	unsigned int i1, i2, i3;

	glm::vec3 baseNormal;
	glm::vec3 normal;

	bool collidesWithFace(RigidBody* thisRB, struct Face& face, RigidBody* faceRB, glm::vec3& retNorm);
	bool collidesWithSphere(RigidBody* thisRB, BoundingRegion& boundingRegion, glm::vec3& retNorm);
} Face;

class CollisionMesh {
public:
	CollisionModel* model;
	BoundingRegion boundingRegion;

	std::vector<glm::vec3> points;
	std::vector<Face> faces;

	CollisionMesh(unsigned int noPoints, float* coordinates, unsigned int noFaces, unsigned int* indices);
};

#endif