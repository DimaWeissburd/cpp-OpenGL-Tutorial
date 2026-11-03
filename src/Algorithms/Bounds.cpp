#include "Bounds.h"

bool BoundingRegion::operator==(const BoundingRegion& br) const {
	if (type != br.type) {
		return false;
	}
	if (type == BoundTypes::AABB) {
		return min == br.min && max == br.max;
	}
	else {
		return center == br.center && radius == br.radius;
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
		return (point.x >= min.x) && (point.x < max.x)
			&& (point.y >= min.y) && (point.y < max.y)
			&& (point.z >= min.z) && (point.z < max.z);
	} else {
		float distSquared = 0.0f;
		for (int i = 0; i < 3; i++) {
			distSquared = (center[i] - point[i]) * (center[i] - point[i]);
		}
		return distSquared <= (radius * radius);
	}
}

bool BoundingRegion::containsRegion(BoundingRegion br) {
	if (br.type == BoundTypes::AABB) {
		return containsPoint(br.min) && containsPoint(br.max);
	} else if (type == BoundTypes::SPHERE && br.type == BoundTypes::SPHERE) {
		return glm::length(center - br.center) + br.radius < radius;
	} else {
		if (!containsPoint(br.center)) {
			return false;
		}
		for (int i = 0; i < 3; i++) {
			if (abs(max[i] - br.center[i]) < br.radius ||
				abs(min[i] - br.center[i]) < br.radius) {
				return false;
			}
		}
		return true;
	}
}

bool BoundingRegion::intersectsWith(BoundingRegion br) {
	if (type == BoundTypes::AABB && br.type == BoundTypes::AABB) {
		glm::vec3 rad = calculateDimensions() / 2.0f;
		glm::vec3 radBr = br.calculateDimensions() / 2.0f;
		glm::vec3 center = calculateCenter();
		glm::vec3 centerBr = br.calculateCenter();
		glm::vec3 dist = abs(center - centerBr);
		for (int i = 0; i < 3; i++) {
			if (dist[i] > rad[i] + radBr[i]) {
				return false;
			}
		}
		return true;
	} else if (type == BoundTypes::SPHERE && br.type == BoundTypes::SPHERE) {
		return glm::length(center - br.center) < (radius + br.radius);
	} else if (type == BoundTypes::SPHERE) {
		float distSquared = 0.0f;
		for (int i = 0; i < 3; i++) {
			float closestPoint = std::max(br.min[i], std::min(center[i], br.max[i]));
			distSquared += (closestPoint - center[i]) * (closestPoint - center[i]);
		}
		return distSquared < (radius * radius);
	} else {
		return br.intersectsWith(*this);
	}
}