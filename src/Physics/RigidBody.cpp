#include "RigidBody.h"

RigidBody::RigidBody() {}

RigidBody::RigidBody(std::string* modelId, glm::vec3 size, float mass, glm::vec3 position)
	: modelId(modelId), size(size), mass(mass), position(position), velocity(0.0f), acceleration(0.0f) {}

void RigidBody::update(float dt) {
	position += velocity * dt + 0.5f * acceleration * (dt * dt);
	velocity += acceleration * dt;
}

void RigidBody::applyForce(glm::vec3 force) {
	acceleration += force / mass;
}

void RigidBody::applyForce(glm::vec3 direction, float magnitude) {
	applyForce(direction * magnitude);
}

void RigidBody::applyAcceleration(glm::vec3 a) {
	acceleration += a;
}

void RigidBody::applyAcceleration(glm::vec3 direction, float magnitude) {
	applyAcceleration(direction * magnitude);
}

void RigidBody::applyImpulse(glm::vec3 force, float dt) {
	velocity += (force / mass) * dt;
}

void RigidBody::applyImpulse(glm::vec3 direction, float magnitude, float dt) {
	applyForce(direction * magnitude, dt);
}

void RigidBody::transferEnergy(float joules, glm::vec3 direction) {
	if (joules == 0) {
		return;
	}

	glm::vec3 dV = sqrt(2 * abs(joules) / mass) * direction;

	velocity += joules > 0 ? dV : -dV;
}