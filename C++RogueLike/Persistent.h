#ifndef PROJECT_PATH_PERSISTENT_H_
#define PROJECT_PATH_PERSISTENT_H_

//#include "libtcod.hpp"
#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)
class Persistent
{
public:
	virtual void load(TCODZip& zip) = 0;
	virtual void save(TCODZip& zip) = 0;
};

#endif // !PROJECT_PATH_PERSISTENT_H_