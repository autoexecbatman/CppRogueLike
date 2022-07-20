#include "mytree.h"
#include <vcruntime.h>

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