#include "RigidBody.h"

RigidBody::RigidBody(float mass, glm::vec3 position, glm::vec3 velocity, glm::vec3 acceleration)
	: mass(mass), position(position), velocity(velocity), acceleration(acceleration) {}

void RigidBody::update(float dt) {
	position = velocity * dt + 0.5f * acceleration * (dt * dt);
	velocity += acceleration * dt;
}