#ifndef BOUNDS_H
#define BOUNDS_H

#include <glm/glm.hpp>

enum class BoundTypes {
	AABB = 0x00,
	SPHERE = 0x01
};

class BoundingRegion {
public:
	BoundTypes type;

	glm::vec3 center;
	float radius;

	glm::vec3 min;
	glm::vec3 max;

	BoundingRegion(BoundTypes type);
	BoundingRegion(glm::vec3 center, float radius);
	BoundingRegion(glm::vec3 min, glm::vec3 max);

	glm::vec3 calculateCenter();
	glm::vec3 calculateDimentions();

	bool containsPoint(glm::vec3 point);
	bool containsRegion(BoundingRegion br);
	bool intersectsWith(BoundingRegion br);
};

#endif