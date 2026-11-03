#include "Octree.h"
#include "../Rendering/Models/Box.hpp"

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
        if (boundingRegion.intersectsWith(object)) {
            if (boundingRegion.instance->instanceId != object.instance->instanceId) {
                std::cout << "Instance " << boundingRegion.instance->instanceId
                    << " (" << boundingRegion.instance->modelId << ") collides with instance "
                    << object.instance->instanceId << " (" << object.instance->modelId << ")" << std::endl;
                boundingRegion.intersectsWith(object);
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