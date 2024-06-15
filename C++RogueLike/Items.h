#pragma once
#ifndef ITEMS_H
#define ITEMS_H

#include "Actor/Actor.h"

class HealthPotion : public Item
{
public:
	HealthPotion(Vector2D position);
};

class ScrollOfLightningBolt : public Item
{
public:
	ScrollOfLightningBolt(Vector2D position);
};

class ScrollOfFireball : public Item
{
public:
	ScrollOfFireball(Vector2D position);
};

class ScrollOfConfusion : public Item
{
public:
	ScrollOfConfusion(Vector2D position);
};

class GoldPile : public Item
{
public:
	GoldPile(Vector2D position);
};

#endif // !ITEMS_H
