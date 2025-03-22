// file: Container.h
#ifndef CONTAINER_H
#define CONTAINER_H

#include <vector>
#include <memory>

#include <libtcod.h>

#include "../Persistent/Persistent.h"

class Actor;
class Item;

class Container : public Persistent
{
public:
	size_t invSize;

	std::vector<std::unique_ptr<Item>> inv;

	Container(size_t invSize) noexcept;

	bool add(std::unique_ptr<Item> actor);
	void remove(std::unique_ptr<Item> actor);

	void load(const json& j) override;
	void save(json& j) override;

	void print_container(std::span<std::unique_ptr<Actor>> container);
};

#endif // !CONTAINER_H
// end of file: Container.h
