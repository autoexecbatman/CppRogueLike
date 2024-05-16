// file: Container.h
#ifndef CONTAINER_H
#define CONTAINER_H

#include <vector>
#include <memory>

#include <libtcod.h>

#include "../Persistent/Persistent.h"

class Actor;

class Container : public Persistent
{
public:
	size_t invSize;

	std::vector<std::unique_ptr<Actor>> inventoryList;

	Container(size_t invSize) noexcept;

	bool add(std::unique_ptr<Actor> actor);
	void remove(std::unique_ptr<Actor> actor);

	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

	void print_container(std::span<std::unique_ptr<Actor>> container);
};

#endif // !CONTAINER_H
// end of file: Container.h
