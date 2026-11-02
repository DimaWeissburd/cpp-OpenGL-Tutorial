#include "Octree.h"

void Octree::calculateBounds(BoundingRegion* out, Octant octant, BoundingRegion parentRegion) {
    glm::vec3 center = parentRegion.calculateCenter();
    if (octant == Octant::O1) {
        out = new BoundingRegion(center, parentRegion.max);
    }
    else if (octant == Octant::O2) {
        out = new BoundingRegion(glm::vec3(parentRegion.min.x, center.y, center.z), glm::vec3(center.x, parentRegion.max.y, parentRegion.max.z));
    }
    else if (octant == Octant::O3) {
        out = new BoundingRegion(glm::vec3(parentRegion.min.x, parentRegion.min.y, center.z), glm::vec3(center.x, center.y, parentRegion.max.z));
    }
    else if (octant == Octant::O4) {
        out = new BoundingRegion(glm::vec3(center.x, parentRegion.min.y, center.z), glm::vec3(parentRegion.max.x, center.y, parentRegion.max.z));
    }
    else if (octant == Octant::O5) {
        out = new BoundingRegion(glm::vec3(center.x, center.y, parentRegion.min.z), glm::vec3(parentRegion.max.x, parentRegion.max.y, center.z));
    }
    else if (octant == Octant::O6) {
        out = new BoundingRegion(glm::vec3(parentRegion.min.x, center.y, parentRegion.min.z), glm::vec3(center.x, parentRegion.max.y, center.z));
    }
    else if (octant == Octant::O7) {
        out = new BoundingRegion(parentRegion.min, center);
    }
    else if (octant == Octant::O8) {
        out = new BoundingRegion(glm::vec3(center.x, parentRegion.min.y, parentRegion.min.z), glm::vec3(parentRegion.max.x, center.y, center.z));
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

void Octree::node::build() {
    if (objects.size() <= 1) {
        return;
    }

    glm::vec3 dimensions = region.calculateDimensions();
    for (int i = 0; i < 3; i++) {
        if (dimensions[i] < MIN_BOUNDS) {
            return;
        }
    }

    BoundingRegion octants[NO_CHILDREN];
    for (int i = 0; i < NO_CHILDREN; i++) {
        calculateBounds(&octants[i], (Octant)(1 << i), region);
    }

    std::vector<BoundingRegion> octLists[NO_CHILDREN];
    std::stack<int> delList;

    for (int i = 0, length = objects.size(); i < length; i++) {
        BoundingRegion br = objects[i];
        for (int j = 0; j < NO_CHILDREN; j++) {
            if (octants[j].containsRegion(br)) {
                octLists[j].push_back(br);
                delList.push(i);
                break;
            }
        }
    }

    while (delList.size() != 0) {
        objects.erase(objects.begin() + delList.top());
        delList.pop();
    }

    for (int i = 0; i < NO_CHILDREN; i++) {
        if (octLists[i].size() != 0) {
            children[i] = new node(octants[i], octLists[i]);
            States::activate(&activeOctants, i);
            children[i]->build();
            hasChildren = true;
        }
    }

    treeBuilt = true;
    treeReady = true;
}

void Octree::node::update() {
    if (treeBuilt && treeReady) {
        std::vector<BoundingRegion> movedObjects(objects.size());
        // TODO

        for (int i = 0, listSize = objects.size(); i < listSize; i++) {
            // TODO
        }

        if (children != nullptr) {
            for (unsigned char flags = activeOctants, i = 0; flags > 0; flags >>= 1, i++) {
                if (States::isActive(&flags, 0)) {
                    if (children[i] != nullptr) {
                        children[i]->update();
                    }
                }
            }
        }

        BoundingRegion movedObj;
        while (movedObjects.size() != 0) {
            movedObj = movedObjects[0];
            node* current = this;

            while (!current->region.containsRegion(movedObj)) {
                if (current->parent != nullptr) {
                    current = current->parent;
                } else {
                    break;
                }
            }

            movedObjects.erase(movedObjects.begin());
            objects.erase(objects.begin() + List::getIndexOf<BoundingRegion>(objects, movedObj));
            current->insert(movedObj);

            // TODO - collision detection
        }
    } else {
        if (queue.size() > 0) {
            processPending();
        }
    }
}

void Octree::node::processPending() {
    if (!treeBuilt) {
        while (queue.size() != 0) {
            objects.push_back(queue.front());
            queue.pop();
        }
        build();
    } else {
        while (queue.size() != 0) {
            insert(queue.front());
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
        return parent == nullptr ? false : parent->insert(object);
    }

    BoundingRegion octants[NO_CHILDREN];
    for (int i = 0; i < NO_CHILDREN; i++) {
        if (children[i] != nullptr) {
            octants[i] = children[i]->region;
        } else {
            calculateBounds(&octants[i], (Octant)(1 << i), region);
        }
    }

    for (int i = 0; i < NO_CHILDREN; i++) {
        if (octants[i].containsRegion(object)) {
            if (children[i] != nullptr) {
                return children[i]->insert(object);
            } else {
                children[i] = new node(octants[i], { object });
                States::activate(&activeOctants, i);
                return true;
            }
        }
    }

    objects.push_back(object);
    return true;
}

void Octree::node::destroy() {
    if (children != nullptr) {
        for (int flags = activeOctants, i = 0; flags > 0; flags >> 1, i++) {
            if (States::isActive(&flags, 0)) {
                if (children[i] != nullptr) {
                    children[i]->destroy();
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