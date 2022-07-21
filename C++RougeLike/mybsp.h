#pragma once

#include "libtcod.hpp"//TCODRandom and TCOD_Random
#include "mytree.h"//tree class and typedef struct m_tree_t


typedef struct 
{
	my_tree_t tree; /* pseudo oop : bsp inherit tree */
	int x, y, w, h; /* node position & size */
	int position; /* position of splitting */
	uint8_t level; /* level in the tree */
	bool horizontal; /* horizontal splitting ? */
} my_bsp_t;

typedef bool (*my_bsp_callback_t)(my_bsp_t* node, void* userData);

bool my_bsp_traverse_inverted_level_order(my_bsp_t* node, my_bsp_callback_t listener, void* userData);

void my_bsp_split_recursive(my_bsp_t* node, TCOD_Random* randomizer, int nb, int minHSize, int minVSize, float maxHRatio, float maxVRatio);


//====
class Bsp;
class BspCallback
{
public:
	virtual ~BspCallback() {}
	virtual bool visitNode(Bsp* node, void* userData) = 0;
};

class Bsp : public Tree
{
public:
	int y = 0, x = 0, h = 0, w = 0;
	int position = 0;
	bool horizontal = false;
	uint8_t level = 0;

	Bsp() : level(0) {}
	Bsp(int y, int x, int h, int w) : y(y), x(x), h(h), w(w), level(0) {}

	/*virtual ~Bsp();*/

	void mysplitRecursive(TCODRandom* randomizer, int nb, int minHSize, int minVSize, float maxHRatio, float maxVRatio);

	bool isLeaf() const { return sons == NULL; }

	bool traverseInvertedLevelOrder(BspCallback* listener, void* userData);

protected:
	Bsp(Bsp* father, bool left);
};

void Bsp::mysplitRecursive(TCODRandom* randomizer, int nb, int minHSize, int minVSize, float maxHRatio, float maxVRatio)
{
}

bool Bsp::traverseInvertedLevelOrder(BspCallback* listener, void* userData)
{
	return 0;
}