// file: Persistent.h
#ifndef PERSISTENT_H
#define PERSISTENT_H

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

#endif // !PERSISTENT_H
// end of file: Persistent.h
