#pragma once
#include "mytree.h"


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