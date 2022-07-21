#pragma once

typedef struct _my_tree_t 
{
	struct _my_tree_t* father;
	struct _my_tree_t* next;
	struct _my_tree_t* sons;
} my_tree_t;

my_tree_t* my_tree_new(void);
void my_tree_add_son(my_tree_t* node, my_tree_t* son);

class Tree
{
public:
	Tree* next;
	Tree* father;
	Tree* sons;

	Tree() : next(NULL), father(NULL), sons(NULL) {}

	void addSon(Tree* data)
	{
		data->father = this;
		Tree* lastson = sons;
		while (lastson && lastson->next) lastson = lastson->next;
		if (lastson)
		{
			lastson->next = data;
		}
		else
		{
			sons = data;
		}
	}
};