// file: Food.h
// Food struct is now in Pickable.h
#pragma once

#include "../Actor/Actor.h"

struct Vector2D;

// Concrete Item subclasses for food items
class Ration : public Item
{
public:
	Ration(Vector2D position);
};

class Fruit : public Item
{
public:
	Fruit(Vector2D position);
};

class Bread : public Item
{
public:
	Bread(Vector2D position);
};

class Meat : public Item
{
public:
	Meat(Vector2D position);
};
