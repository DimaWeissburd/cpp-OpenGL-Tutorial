#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include <glm/glm.hpp>

class RigidBody {
public:
	float mass;
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 acceleration;

	RigidBody(float mass = 1.0f, glm::vec3 position = glm::vec3(0.0f), glm::vec3 velocity = glm::vec3(0.0f), glm::vec3 acceleration = glm::vec3(0.0f));

	void update(float dt);
};

#endif