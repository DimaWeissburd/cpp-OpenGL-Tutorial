#include "RigidBody.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

RigidBody::RigidBody() {}

RigidBody::RigidBody(std::string modelId, glm::vec3 position, glm::vec3 rotation, glm::vec3 size, float mass)
	: modelId(modelId), position(position), rotation(rotation), size(size), mass(mass), velocity(0.0f), acceleration(0.0f), state(0),
	lastCollision(COLLISION_THRESHOLD), lastCollisionID("") {
	update(0.0f);
}

void RigidBody::update(float dt) {
	position += velocity * dt + 0.5f * acceleration * (dt * dt);
	velocity += acceleration * dt;

	glm::mat4 rotMat = glm::mat4(glm::quat(rotation));

	model = glm::translate(glm::mat4(1.0f), position);
	model = model * rotMat;
	model = glm::scale(model, size);

	normalModel = glm::transpose(glm::inverse(glm::mat3(model)));

	lastCollision += dt;
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

void RigidBody::handleCollision(RigidBody* inst, glm::vec3 norm) {
	if (lastCollision >= COLLISION_THRESHOLD || lastCollisionID != inst->instanceId) {
		this->velocity = glm::reflect(this->velocity, glm::normalize(norm));
		lastCollision = 0.0f;
	}

	lastCollisionID = inst->instanceId;
}