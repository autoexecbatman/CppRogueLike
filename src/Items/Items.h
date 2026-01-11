#pragma once

#include "../Actor/Actor.h"

struct GameContext;

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

class ScrollOfTeleportation : public Item
{
public:
	ScrollOfTeleportation(Vector2D position);
};

class GoldPile : public Item
{
public:
	GoldPile(Vector2D position, GameContext& ctx);
};

class AmuletOfYendor : public Item
{
public:
	AmuletOfYendor(Vector2D position);
};

class LeatherArmorItem : public Item
{
public:
	LeatherArmorItem(Vector2D position);
};

class ChainMailItem : public Item
{
public:
	ChainMailItem(Vector2D position);
};

class PlateMailItem : public Item
{
public:
	PlateMailItem(Vector2D position);
};
