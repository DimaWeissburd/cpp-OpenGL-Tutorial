#include "Bounds.h"
#include "Octree.h"

bool BoundingRegion::operator==(const BoundingRegion& boundingRegion) const {
	if (type != boundingRegion.type) {
		return false;
	}
	if (type == BoundTypes::AABB) {
		return min == boundingRegion.min && max == boundingRegion.max;
	}
	else {
		return center == boundingRegion.center && radius == boundingRegion.radius;
	}
}

BoundingRegion::BoundingRegion(BoundTypes type)
	: type(type) {}

BoundingRegion::BoundingRegion(glm::vec3 center, float radius)
	:type(BoundTypes::SPHERE), center(center), ogCenter(center), radius(radius), ogRadius(radius){}

BoundingRegion::BoundingRegion(glm::vec3 min, glm::vec3 max)
	:type(BoundTypes::AABB), min(min), ogMin(min), max(max), ogMax(max) {}

void BoundingRegion::transform() {
	if (instance) {
		if (type == BoundTypes::AABB) {
			min = ogMin * instance->size + instance->position;
			max = ogMax * instance->size + instance->position;
		}
		else {
			center = ogCenter * instance->size + instance->position;
			float maxDim = instance->size[0];
			for (int i = 1; i < 3; i++) {
				if (instance->size[i] > maxDim) {
					maxDim = instance->size[i];
				}
			}
			radius = ogRadius * maxDim;
		}
	}
}

glm::vec3 BoundingRegion::calculateCenter() {
	return (type == BoundTypes::AABB) ? (min + max) / 2.0f : center;
}

glm::vec3 BoundingRegion::calculateDimensions() {
	return (type == BoundTypes::AABB) ? (max - min) : glm::vec3(2.0f * radius);
}

bool BoundingRegion::containsPoint(glm::vec3 point) {
	if (type == BoundTypes::AABB) {
		return (point.x >= min.x) && (point.x <= max.x)
			&& (point.y >= min.y) && (point.y <= max.y)
			&& (point.z >= min.z) && (point.z <= max.z);
	} else if (type == BoundTypes::SPHERE) {
		float distSquared = 0.0f;
		for (int i = 0; i < 3; i++) {
			distSquared += (center[i] - point[i]) * (center[i] - point[i]);
		}
		return distSquared <= (radius * radius);
	}
}

bool BoundingRegion::containsRegion(BoundingRegion boundingRegion) {
	if (boundingRegion.type == BoundTypes::AABB) {
		return containsPoint(boundingRegion.min) && containsPoint(boundingRegion.max);
	} else if (type == BoundTypes::SPHERE && boundingRegion.type == BoundTypes::SPHERE) {
		return glm::length(center - boundingRegion.center) + boundingRegion.radius < radius;
	} else {
		if (!containsPoint(boundingRegion.center)) {
			return false;
		}
		for (int i = 0; i < 3; i++) {
			if (abs(max[i] - boundingRegion.center[i]) < boundingRegion.radius ||
				abs(min[i] - boundingRegion.center[i]) < boundingRegion.radius) {
				return false;
			}
		}
		return true;
	}
}

bool BoundingRegion::intersectsWith(BoundingRegion boundingRegion) {
	if (type == BoundTypes::AABB && boundingRegion.type == BoundTypes::AABB) {
		glm::vec3 radius = calculateDimensions() / 2.0f;
		glm::vec3 boundingRegionRadius = boundingRegion.calculateDimensions() / 2.0f;
		glm::vec3 center = calculateCenter();
		glm::vec3 boundingRegionCenter = boundingRegion.calculateCenter();
		glm::vec3 dist = abs(center - boundingRegionCenter);
		for (int i = 0; i < 3; i++) {
			if (dist[i] > radius[i] + boundingRegionRadius[i]) {
				return false;
			}
		}
		return true;
	} else if (type == BoundTypes::SPHERE && boundingRegion.type == BoundTypes::SPHERE) {
		return glm::length(center - boundingRegion.center) < (radius + boundingRegion.radius);
	} else if (type == BoundTypes::SPHERE) {
		float distSquared = 0.0f;
		for (int i = 0; i < 3; i++) {
			float closestPoint = std::max(boundingRegion.min[i], std::min(center[i], boundingRegion.max[i]));
			distSquared += (closestPoint - center[i]) * (closestPoint - center[i]);
		}
		return distSquared < (radius * radius);
	} else {
		return boundingRegion.intersectsWith(*this);
	}
}