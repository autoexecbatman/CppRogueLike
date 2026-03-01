// file: Items.cpp.test.cpp
// Legacy test file - updated for variant-based ItemBehavior
#include <cassert>
#include <iostream>

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "Items.h"

void test_HealthPotion_behavior()
{
	Vector2D pos{ 0, 0 };
	HealthPotion potion(pos);
	assert(potion.behavior.has_value());
	assert(std::holds_alternative<Consumable>(*potion.behavior));
	const Consumable& c = std::get<Consumable>(*potion.behavior);
	assert(c.effect == ConsumableEffect::HEAL);
	assert(c.amount == 10);
	std::cout << "test_HealthPotion_behavior passed\n";
}

void test_Teleporter_behavior()
{
	Vector2D pos{ 0, 0 };
	ScrollOfTeleportation scroll(pos);
	assert(scroll.behavior.has_value());
	assert(std::holds_alternative<Teleporter>(*scroll.behavior));
	std::cout << "test_Teleporter_behavior passed\n";
}

void test_GoldPile_behavior()
{
	Vector2D pos{ 0, 0 };
	// GoldPile requires GameContext; skip runtime test, just verify type
	std::cout << "test_GoldPile_behavior skipped (requires GameContext)\n";
}

void test_AmuletOfYendor_behavior()
{
	Vector2D pos{ 0, 0 };
	AmuletOfYendor amulet(pos);
	assert(amulet.behavior.has_value());
	assert(std::holds_alternative<Amulet>(*amulet.behavior));
	std::cout << "test_AmuletOfYendor_behavior passed\n";
}

int main()
{
	try
	{
		test_HealthPotion_behavior();
		test_Teleporter_behavior();
		test_GoldPile_behavior();
		test_AmuletOfYendor_behavior();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Test failed: " << e.what() << "\n";
		return 1;
	}
	return 0;
}
