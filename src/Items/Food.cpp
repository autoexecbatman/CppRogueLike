// file: Food.cpp
#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "../Colors/Colors.h"
#include "../Items/ItemClassification.h"
#include "../Systems/ContentRegistry.h"
#include "../Utils/Vector2D.h"
#include "Food.h"

Ration::Ration(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::FOOD_RATION), "ration", WHITE_GREEN_PAIR })
{
	behavior = Food{ 300 };
	set_value(10);
}

Fruit::Fruit(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::FRUIT), "fruit", GREEN_BLACK_PAIR })
{
	behavior = Food{ 100 };
	set_value(3);
}

Bread::Bread(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::BREAD), "bread", RED_YELLOW_PAIR })
{
	behavior = Food{ 200 };
	set_value(5);
}

Meat::Meat(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::MEAT), "meat", RED_BLACK_PAIR })
{
	behavior = Food{ 250 };
	set_value(8);
}
