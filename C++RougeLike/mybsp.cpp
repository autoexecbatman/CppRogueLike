#include "libtcod.hpp"

//#include "list.hpp"
//#include "mersenne.hpp"
#include "mytree.cpp"
#include "mybsp.h"

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
	int y, x, h, w;
	int position;
	bool horizontal;
	uint8_t level;

	Bsp() : level(0) {}
	Bsp(int y, int x, int h, int w) : y(y), x(x), h(h), w(w), level(0) {}

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
