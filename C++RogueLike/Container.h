// file: Container.h
#ifndef CONTAINER_H
#define CONTAINER_H

#include <vector>
#include <memory>

#include <libtcod.h>

#include "Persistent.h"

class Actor;

class Container : public Persistent
{
public:
	int invSize = 0;

	std::vector<std::shared_ptr<Actor>> inventoryList;

	Container(int invSize) noexcept;
	~Container() noexcept;

	// checks that the container is not full.
	bool add(Actor& actor);
	void remove(Actor& actor);

	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

	void print_container(std::vector<std::shared_ptr<Actor>> container);
};

#endif // !CONTAINER_H
// end of file: Container.h
