#ifndef OCTREE_H
#define OCTREE_H

#include <vector>
#include <queue>
#include <stack>
#include "List.hpp"
#include "States.hpp"
#include "Bounds.h"

#define NO_CHILDREN 8
#define MIN_BOUNDS 0.5

namespace Octree {
	enum class Octant : unsigned char {
		O1 = 0x01, //0b00000001
		O2 = 0x02, //0b00000010
		O3 = 0x04, //0b00000100
		O4 = 0x08, //0b00001000
		O5 = 0x10, //0b00010000
		O6 = 0x20, //0b00100000
		O7 = 0x40, //0b01000000
		O8 = 0x80  //0b10000000
	};

	void calculateBounds(BoundingRegion* out, Octant octant, BoundingRegion parentRegion);

	class node {
	public:
		node* parent;
		node* children[NO_CHILDREN];

		unsigned char activeOctants;

		bool hasChildren = false;

		bool treeReady = false;
		bool treeBuilt = false;

		std::vector<BoundingRegion> objects;
		std::queue<BoundingRegion> queue;

		BoundingRegion region;

		node();

		node(BoundingRegion bounds);

		node(BoundingRegion bounds, std::vector<BoundingRegion> objectList);

		void build();

		void update();

		void processPending();

		bool insert(BoundingRegion object);

		void destroy();
	};
}

#endif