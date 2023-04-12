#include <iostream>
#include <vector>

#include "main.h"

Container::Container(int invSize) : invSize(invSize) {}

Container::~Container() 
{
	inventoryList.clear();
}

// checks that the container is not full.
bool Container::add(Actor& actor)
{	
	//if (invSize < 0 && inventory.size() <= invSize)
	//{
	//	// inventory full
	//	return false;
	//}

	if (// check if the inventory is full
		//invSize > 0
		//&& 
		//inventoryList.size() >= static_cast<unsigned int>(invSize)
		// use gsl
		invSize > 0
		&&
		inventoryList.size() >= gsl::narrow_cast<unsigned int>(invSize)
		)
	{
		// inventory full return false
		return false;
	}
	/*inventoryList.emplace_back(&actor);*/

	inventoryList.push_back(std::make_shared<Actor>(actor));

	/*inventoryList.push_back(actor);*/ // add the actor to the inventory

	return true;
}


void Container::remove(Actor& actor)
{
	std::clog << "Removing item from inventory" << std::endl;
	/*inventoryList.erase(std::remove(inventoryList.begin(), inventoryList.end(), actor), inventoryList.end());*/

	// iterate through the inventory and remove the item
	for (auto it = inventoryList.begin(); it != inventoryList.end(); it++)
	{
		if (it->get() == &actor)
		{
			inventoryList.erase(it);
			std::clog << "Item removed from inventory" << *it << std::endl;
			break;
		}
	}

}

void Container::load(TCODZip& zip)
{
	invSize = zip.getInt();
	int nbActors = zip.getInt();
	while (nbActors > 0) 
	{
		/*Actor* actor = new Actor(0, 0, 0, nullptr, 0);*/
		std::shared_ptr<Actor> actor = std::make_shared<Actor>(0, 0, 0, nullptr, 0, 0);
		actor->load(zip);
		inventoryList.push_back(actor);
		/*inventoryList.emplace_back(actor);*/
		nbActors--;
	}
}

void Container::save(TCODZip& zip)
{
	zip.putInt(invSize);
	zip.putInt(inventoryList.size());
	// iterate through the inventory and save the item
	for (std::shared_ptr<Actor> actor : inventoryList)
	{
		actor->save(zip);
	}
}

void Container::print_container(std::vector<std::shared_ptr<Actor>> container)
{
	int i = 0;
	for (const auto& item : inventoryList)
	{
		std::cout << item->name.c_str() << i << " ";
		i++;
	}
	std::cout << '\n';
}