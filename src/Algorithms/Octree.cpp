#include "Octree.h"
#include "../Graphics/Models/Box.hpp"

void Octree::calculateBounds(BoundingRegion& out, Octant octant, BoundingRegion parentRegion) {
    glm::vec3 center = parentRegion.calculateCenter();
    if (octant == Octant::O1) {
        out = BoundingRegion(center, parentRegion.max);
    }
    else if (octant == Octant::O2) {
        out = BoundingRegion(glm::vec3(parentRegion.min.x, center.y, center.z), glm::vec3(center.x, parentRegion.max.y, parentRegion.max.z));
    }
    else if (octant == Octant::O3) {
        out = BoundingRegion(glm::vec3(parentRegion.min.x, parentRegion.min.y, center.z), glm::vec3(center.x, center.y, parentRegion.max.z));
    }
    else if (octant == Octant::O4) {
        out = BoundingRegion(glm::vec3(center.x, parentRegion.min.y, center.z), glm::vec3(parentRegion.max.x, center.y, parentRegion.max.z));
    }
    else if (octant == Octant::O5) {
        out = BoundingRegion(glm::vec3(center.x, center.y, parentRegion.min.z), glm::vec3(parentRegion.max.x, parentRegion.max.y, center.z));
    }
    else if (octant == Octant::O6) {
        out = BoundingRegion(glm::vec3(parentRegion.min.x, center.y, parentRegion.min.z), glm::vec3(center.x, parentRegion.max.y, center.z));
    }
    else if (octant == Octant::O7) {
        out = BoundingRegion(parentRegion.min, center);
    }
    else if (octant == Octant::O8) {
        out = BoundingRegion(glm::vec3(center.x, parentRegion.min.y, parentRegion.min.z), glm::vec3(parentRegion.max.x, center.y, center.z));
    }
}

Octree::node::node()
    : region(BoundTypes::AABB) {}

Octree::node::node(BoundingRegion bounds)
    : region(bounds) {}

Octree::node::node(BoundingRegion bounds, std::vector<BoundingRegion> objectList)
    : region(bounds) {
    objects.insert(objects.end(), objectList.begin(), objectList.end());
}

void Octree::node::addToPending(RigidBody* instance, Trie::Trie<Model*> models) {
    for (BoundingRegion boundingRegion : models[instance->modelId]->boundingRegions) {
        boundingRegion.instance = instance;
        boundingRegion.transform();
        queue.push(boundingRegion);
    }
}

void Octree::node::build() {
    BoundingRegion octants[NO_CHILDREN];
    glm::vec3 dimensions = region.calculateDimensions();
    std::vector<BoundingRegion> octLists[NO_CHILDREN];

    if (objects.size() <= 1) {
        goto setVars;
    }

    for (int i = 0; i < 3; i++) {
        if (dimensions[i] < MIN_BOUNDS) {
            goto setVars;
        }
    }

    for (int i = 0; i < NO_CHILDREN; i++) {
        calculateBounds(octants[i], (Octant)(1 << i), region);
    }

    for (int i = 0, length = objects.size(); i < length; i++) {
        BoundingRegion br = objects[i];
        for (int j = 0; j < NO_CHILDREN; j++) {
            if (octants[j].containsRegion(br)) {
                octLists[j].push_back(br);
                objects.erase(objects.begin() + i);
                i--;
                length--;
                break;
            }
        }
    }

    for (int i = 0; i < NO_CHILDREN; i++) {
        if (octLists[i].size() != 0) {
            children[i] = new node(octants[i], octLists[i]);
            States::activateIndex(&activeOctants, i);
            children[i]->parent = this;
            children[i]->build();
        }
    }

setVars:
    treeBuilt = true;
    treeReady = true;

    for (int i = 0; i < objects.size(); i++) {
        objects[i].cell = this;
    }
}

void Octree::node::update(Box& box) {
    box.positions.push_back(region.calculateCenter());
    box.sizes.push_back(region.calculateDimensions());

    if (treeBuilt && treeReady) {
        if (objects.size() == 0) {
            if (!activeOctants) {
                if (currentLifespan == -1) {
                    currentLifespan = maxLifespan;
                }
                else if (currentLifespan > 0) {
                    currentLifespan--;
                }
            }
        } else {
            if (currentLifespan != -1) {
                if (maxLifespan <= 64) {
                    maxLifespan <<= 2;
                }
            }
        }

        for (int i = 0, listSize = objects.size(); i < listSize; i++) {
            if (States::isActive(&objects[i].instance->state, INSTANCE_DEAD)) {
                objects.erase(objects.begin() + i);
                i--;
                listSize--;
            }
        }

        std::stack<int> movedObjects;
        for (int i = 0, listSize = objects.size(); i < listSize; i++) {
            if (States::isActive(&objects[i].instance->state, INSTANCE_MOVED)) {
                objects[i].transform();
                movedObjects.push(i);
            }
            box.positions.push_back(objects[i].calculateCenter());
            box.sizes.push_back(objects[i].calculateDimensions());
        }

        unsigned char flags = activeOctants;
        for (int i = 0; flags > 0; flags >>= 1, i++) {
            if (States::isIndexActive(&flags, 0) && children[i] != nullptr && children[i]->currentLifespan == 0) {
                if (children[i]->objects.size() > 0) {
                    children[i]->currentLifespan = -1;
                } else {
                    children[i]->destroy();
                    delete children[i];
                    children[i] = nullptr;
                    States::deactivateIndex(&activeOctants, i);
                }
            }
        }

        if (children != nullptr) {
            for (unsigned char flags = activeOctants, i = 0; flags > 0; flags >>= 1, i++) {
                if (States::isIndexActive(&flags, 0)) {
                    if (children[i] != nullptr) {
                        children[i]->update(box);
                    }
                }
            }
        }

        BoundingRegion movedObject;
        while (movedObjects.size() != 0) {
            movedObject = objects[movedObjects.top()];
            node* current = this;

            while (!current->region.containsRegion(movedObject)) {
                if (current->parent != nullptr) {
                    current = current->parent;
                } else {
                    break;
                }
            }

            objects.erase(objects.begin() + movedObjects.top());
            movedObjects.pop();
            current->queue.push(movedObject);

            current = movedObject.cell;
            current->checkCollisionsSelf(movedObject);
            current->checkCollisionsChildren(movedObject);
            while (current->parent) {
                current = current->parent;
                current->checkCollisionsSelf(movedObject);
            }
        }
    }

    processPending();
}

void Octree::node::processPending() {
    if (!treeBuilt) {
        while (queue.size() != 0) {
            objects.push_back(queue.front());
            queue.pop();
        }
        build();
    } else {
        for (int i = 0, length = queue.size(); i < length; i++) {
            BoundingRegion boundingRegion = queue.front();
            if (region.containsRegion(boundingRegion)) {
                insert(boundingRegion);
            } else {
                boundingRegion.transform();
                queue.push(boundingRegion);
            }
            queue.pop();
        }
    }
}

bool Octree::node::insert(BoundingRegion object) {
    glm::vec3 dimensions = region.calculateDimensions();
    if (objects.size() == 0 || dimensions.x < MIN_BOUNDS || dimensions.y < MIN_BOUNDS || dimensions.z < MIN_BOUNDS) {
        objects.push_back(object);
        return true;
    }

    if (!region.containsRegion(object)) {
        if (parent != nullptr) return parent->insert(object);
        object.cell = this;
        objects.push_back(object);
        return true;
    }

    BoundingRegion octants[NO_CHILDREN];
    for (int i = 0; i < NO_CHILDREN; i++) {
        if (children[i] != nullptr) {
            octants[i] = children[i]->region;
        } else {
            calculateBounds(octants[i], (Octant)(1 << i), region);
        }
    }

    objects.push_back(object);

    std::vector<BoundingRegion> octLists[NO_CHILDREN];
    for (int i = 0, length = objects.size(); i < length; i++) {
        objects[i].cell = this;
        for (int j = 0; j < NO_CHILDREN; j++) {
            if (octants[j].containsRegion(objects[i])) {
                octLists[j].push_back(objects[i]);
                objects.erase(objects.begin() + i);
                i--;
                length--;
                break;
            }
        }
    }

    for (int i = 0; i < NO_CHILDREN; i++) {
        if (octLists[i].size() != 0) {
            if (children[i]) {
                for (BoundingRegion boundingRegion : octLists[i]) {
                    children[i]->insert(boundingRegion);
                }
            } else {
                children[i] = new node(octants[i], octLists[i]);
                children[i]->parent = this;
                States::activateIndex(&activeOctants, i);
                children[i]->build();
            }
        }
    }

    return true;
}

void Octree::node::checkCollisionsSelf(BoundingRegion object) {
    for (BoundingRegion boundingRegion : objects) {
        if (boundingRegion.instance->instanceId == object.instance->instanceId) {
            continue;
        }

        if (boundingRegion.intersectsWith(object)) {

            unsigned int noFacesBr = boundingRegion.collisionMesh ? boundingRegion.collisionMesh->faces.size() : 0;
            unsigned int noFacesObj = object.collisionMesh ? object.collisionMesh->faces.size() : 0;

            glm::vec3 normal;

            if (noFacesBr) {
                unsigned int noFacesBr = boundingRegion.collisionMesh->faces.size();

                if (noFacesObj) {
                    unsigned int noFacesObj = object.collisionMesh->faces.size();

                    for (unsigned int i = 0; i < noFacesBr; i++) {
                        for (unsigned int j = 0; j < noFacesObj; j++) {
                            if (boundingRegion.collisionMesh->faces[i].collidesWithFace(
                                boundingRegion.instance,
                                object.collisionMesh->faces[j],
                                object.instance,
                                normal
                            )) {
                                object.instance->handleCollision(boundingRegion.instance, normal);
                                break;
                            }
                        }
                    }
                } else {
                    for (unsigned int i = 0; i < noFacesBr; i++) {
                        if (boundingRegion.collisionMesh->faces[i].collidesWithSphere(
                            boundingRegion.instance,
                            object,
                            normal
                        )) {
                            object.instance->handleCollision(boundingRegion.instance, normal);
                            break;
                        }
                    }
                }
            } else {
                if (noFacesObj) {
                    unsigned int noFacesObj = object.collisionMesh->faces.size();

                    for (int i = 0; i < noFacesObj; i++) {
                        if (object.collisionMesh->faces[i].collidesWithSphere(
                            object.instance,
                            boundingRegion,
                            normal
                        )) {
                            object.instance->handleCollision(boundingRegion.instance, normal);
                            break;
                        }
                    }
                } else {
                    normal = object.center - boundingRegion.center;
                    object.instance->handleCollision(boundingRegion.instance, normal);
                }
            }
        }
    }
}

void Octree::node::checkCollisionsChildren(BoundingRegion object) {
    if (children) {
        for (int flags = activeOctants, i = 0; flags > 0; flags >>= 1, i++) {
            if (States::isIndexActive(&flags, 0) && children[i]) {
                children[i]->checkCollisionsSelf(object);
                children[i]->checkCollisionsChildren(object);
            }
        }
    }
}

BoundingRegion* Octree::node::checkCollisionsRay(Ray ray, float& tmin) {
    float tmin_tmp = std::numeric_limits<float>::max();
    float tmax_tmp = std::numeric_limits<float>::lowest();
    float t_tmp = std::numeric_limits<float>::max();

    if (ray.intersectsBoundingRegion(region, tmin_tmp, tmax_tmp)) {
        if (tmin_tmp >= tmin) {
            return nullptr;
        }

        BoundingRegion* ret = nullptr, * ret_tmp = nullptr;

        for (BoundingRegion& boundingRegion : this->objects) {
            tmin_tmp = std::numeric_limits<float>::max();
            tmax_tmp = std::numeric_limits<float>::lowest();

            if (ray.intersectsBoundingRegion(boundingRegion, tmin_tmp, tmax_tmp)) {
                if (tmin_tmp > tmin) {
                    continue;
                } else if (boundingRegion.collisionMesh) {
                    t_tmp = std::numeric_limits<float>::max();
                    if (ray.intersectsMesh(boundingRegion.collisionMesh, boundingRegion.instance, t_tmp)) {
                        if (t_tmp < tmin) {
                            tmin = t_tmp;
                            ret = &boundingRegion;
                        }
                    }
                } else {
                    if (tmin_tmp < tmin) {
                        tmin = tmin_tmp;
                        ret = &boundingRegion;
                    }
                }
            }
        }

        if (children) {
            for (unsigned char flags = activeOctants, i = 0; flags > 0; flags >>= 1, i++) {
                if (States::isIndexActive(&flags, 0) && children[i] != nullptr) {
                    ret_tmp = children[i]->checkCollisionsRay(ray, tmin);
                    if (ret_tmp) {
                        ret = ret_tmp;
                    }
                }
            }
        }

        return ret;
    }

    return nullptr;
}

void Octree::node::destroy() {
    if (children != nullptr) {
        for (int flags = activeOctants, i = 0; flags > 0; flags >>= 1, i++) {
            if (States::isActive(&flags, 0)) {
                if (children[i] != nullptr) {
                    children[i]->destroy();
                    delete children[i];
                    children[i] = nullptr;
                }
            }
        }
    }

    objects.clear();
    while (queue.size() != 0) {
        queue.pop();
    }
}