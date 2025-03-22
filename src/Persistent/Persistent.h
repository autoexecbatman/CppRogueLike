// file: Persistent.h
#ifndef PERSISTENT_H
#define PERSISTENT_H

#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)

#include <nlohmann/json.hpp>
using json = nlohmann::json;

class Persistent
{
public:
	virtual ~Persistent() = default;

	virtual void load(const json& json) = 0;
	virtual void save(json& json) = 0;
};

#endif // !PERSISTENT_H
// end of file: Persistent.h
