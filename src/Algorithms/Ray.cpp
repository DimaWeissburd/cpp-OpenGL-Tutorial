#include "Ray.h"

#include "LinAlg.h"
#include <limits>

Ray::Ray(glm::vec3 origin, glm::vec3 direction)
	: origin(origin), direction(direction), inverseDirection(1.0f) {
	for (int i = 0; i < 3; i++) {
		inverseDirection[i] /= direction[i];
	}
}

bool Ray::intersectsBoundingRegion(BoundingRegion boundingRegion, float& tmin, float& tmax) {
	if (boundingRegion.type == BoundTypes::AABB) {
		tmin = std::numeric_limits<float>::lowest();
		tmax = std::numeric_limits<float>::max();

		for (int i = 0; i < 3; i++) {
			float t1 = (boundingRegion.min[i] - origin[i]) * inverseDirection[i];
			float t2 = (boundingRegion.max[i] - origin[i]) * inverseDirection[i];

			tmin = std::fmaxf(tmin, std::fminf(t1, t2));
			tmax = std::fminf(tmax, std::fmaxf(t1, t2));
		}

		return (tmax >= tmin) && tmax >= 0.0f;
	} else {
		glm::vec3 cp = origin - boundingRegion.center;

		float a = magsq(direction);
		float b = 2.0f * glm::dot(direction, cp);
		float c = magsq(cp) - boundingRegion.radius * boundingRegion.radius;

		float D = b * b - 4.0f * a * c;
		if (D < 0) {
			return false;
		}
		D = sqrtf(D);

		tmax = (-b + D) / (2.0f * a);
		tmin = (-b - D) / (2.0f * a);

		return tmax >= 0.0f;
	}
}

bool Ray::intersectsMesh(CollisionMesh* mesh, RigidBody* rigidBody, float& t) {
	bool intersects = false;

	for (Face& f : mesh->faces) {
		float tmp = -1.0f;
		glm::vec3 P1 = mat4vec3mult(rigidBody->model, mesh->points[f.i1]);
		glm::vec3 P2 = mat4vec3mult(rigidBody->model, mesh->points[f.i2]) - P1;
		glm::vec3 P3 = mat4vec3mult(rigidBody->model, mesh->points[f.i3]) - P1;
		glm::vec3 normal = rigidBody->normalModel * f.normal;

		glm::vec3 U1 = origin - P1;

		LinePlaneIntCase intCase = linePlaneIntersection(glm::vec3(0.0f), normal, U1, direction, tmp);

		if ((char)intCase > 1) {
			if (tmp < 0.0f || t < tmp) {
				continue;
			}

			glm::vec3 intersection = U1 + tmp * direction;

			if (faceContainsPoint(P2, P3, normal, intersection)) {
				intersects = true;
				t = fminf(t, tmp);
			}
		}
	}

	return intersects;
}