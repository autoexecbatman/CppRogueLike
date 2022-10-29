#include <iostream>
#include <curses.h>
#include <math.h>

//#include "main.h"
#include "libtcod.hpp"
class Actor;
#include "Persistent.h"
#include "Destructible.h"
#include "Attacker.h"
#include "Ai.h"
#include "Pickable.h"
#include "Container.h"
#include "Actor.h"
#include "Map.h"
#include "Engine.h"


//====
//the actor constructor initializes the actor's position,name and color
Actor::Actor(
	int y,
	int x,
	int ch,
	const char* name,
	int col
) : 
	posY(y),
	posX(x),
	ch(ch),
	col(col),
	name(name),
	blocks(true),
	fovOnly(true),
	attacker(nullptr),
	destructible(nullptr),
	ai(nullptr),
	container(nullptr),
	pickable(nullptr)
{}

Actor::~Actor() {}

//====
void Actor::load(TCODZip& zip)
{
	posX = zip.getInt();
	posY = zip.getInt();
	ch = zip.getInt();
	col = zip.getInt();
	name = _strdup(zip.getString());
	blocks = zip.getInt();

	bool hasAttacker = zip.getInt();
	bool hasDestructible = zip.getInt();
	bool hasAi = zip.getInt();
	bool hasPickable = zip.getInt();
	bool hasContainer = zip.getInt();

	if (hasAttacker) 
	{
		attacker = std::make_unique<Attacker>(0);
		attacker->load(zip);
	}
	if (hasDestructible) 
	{
		destructible = Destructible::create(zip);
	}
	if (hasAi) 
	{
		ai = Ai::create(zip);
	}
	if (hasPickable) 
	{
		pickable = Pickable::create(zip);
	}
	if (hasContainer) 
	{
		container = new Container(0);
		container->load(zip);
	}
}

void Actor::save(TCODZip& zip)
{
	zip.putInt(posX);
	zip.putInt(posY);
	zip.putInt(ch);
	zip.putInt(col);
	zip.putString(name);
	zip.putInt(blocks);

	zip.putInt(attacker != nullptr);
	zip.putInt(destructible != nullptr);
	zip.putInt(ai != nullptr);
	zip.putInt(pickable != nullptr);
	zip.putInt(container != nullptr);

	if (attacker) attacker->save(zip);
	if (destructible) destructible->save(zip);
	if (ai) ai->save(zip);
	if (pickable) pickable->save(zip);
	if (container) container->save(zip);
}

//the actor render function with color
void Actor::render() const
{
	attron(COLOR_PAIR(col));
	mvaddch(posY, posX, ch);
	attroff(COLOR_PAIR(col));
}

void Actor::pickItem(int x, int y)
{
	// add item to inventory
	container->add(this);
}

//the monster update
void Actor::update()
{	
	if (ai)
	{
		ai->update(this);
	}
}

// a function to get the distance from an actor to a specific tile of the map
int Actor::get_distance(int tileX, int tileY) const
{
	// using chebyshev distance
	int distance = std::max(abs(posX - tileX), abs(posY - tileY));

	mvprintw(10, 0, "Distance: %d", distance);

	return distance;
}

//namespace test {
//
//#ifndef TEST_CASSERT
//#include <cassert>
//#endif // !TEST_CASSERT
//
//#ifndef TEST_GOBLIN
//#include "Goblin.h"
//#endif // !TEST_GOBLIN
//
//
//	int id = 0;
//
//	std::unordered_map<int, std::unique_ptr<Actor>> new_actors;
//	std::deque<int> actor_turn_order;
//	int next_id = 0;
//
//	/// Add a new Actor and return it's ID.
//	template <typename T>
//	auto new_actor(int new_y, int new_x) -> int {
//		new_actors.emplace_back<T>(next_id, std::make_unique<T>(new_y, new_x));
//		actor_turn_order.push_back(next_id);
//		return next_id++;
//	}
//
//	/// Remove an actor, ignores missing ID's.
//	auto delete_actor(int id) {
//		auto it = new_actors.find(id);
//		if (it != new_actors.end()) new_actors.erase(it);
//	}
//
//	/// Returns actor by ID if exists, otherwise returns nullptr.
//	auto get_actor(int id) -> Actor* {
//		auto it = new_actors.find(id);
//		if (it != new_actors.end()) return it->second.get();
//		return nullptr;
//	}
//
//	/// Return the next scheduled actor.
//	auto next_actor(int id) -> Actor& {
//		assert(actor_turn_order.size());
//		while (true) {
//			int next_id = actor_turn_order.front();
//			actor_turn_order.pop_front();
//			auto it = new_actors.find(id);
//			if (it == new_actors.end()) continue;  // Actor was deleted.
//			actor_turn_order.push_back(next_id);
//				return *(it->second);
//		}
//	}
//
//
//	void test() {
//		// Create a new actor.
//		int id = new_actor<Goblin>(0, 0);
//		// Get the actor.
//		Actor* actor = get_actor(id);
//		// Delete the actor.
//		delete_actor(id);
//		// Get the next scheduled actor.
//		Actor& next = next_actor(id);
//	}
//	
//	void create_monster(int mon_y, int mon_x) {
//		for (int i = 0; i < 10; ++i) {
//			// New actors handled by implemtation.
//			
//			test::id = new_actor<Goblin>(mon_y, mon_x);
//			auto new_actor = get_actor(id);
//		}
//
//
//		for (const auto& it : new_actors) {
//			// No changes expected while rendering.
//			it.second.get();
//		}
//
//		while (true) {
//			auto& actor = next_actor(id);
//			// Can call delete_actor while handling actor.
//		}
//	}
//}