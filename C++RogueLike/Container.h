// file: Container.h
#ifndef CONTAINER_H
#define CONTAINER_H

class Container
{
public:
	int invSize = 0;

	std::vector<std::shared_ptr<Actor>> inventoryList;

	Container(int invSize);
	~Container();

	// checks that the container is not full.
	bool add(Actor& actor);
	void remove(Actor& actor);

	void load(TCODZip& zip);
	void save(TCODZip& zip);

	void print_container(std::vector<std::shared_ptr<Actor>> container);
};

#endif // !CONTAINER_H
// end of file: Container.h