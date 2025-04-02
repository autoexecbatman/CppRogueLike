#include "Items.h"
#include "Game.h"
#include "Actor/Actor.h"
#include "Actor/Pickable.h"
#include "ActorTypes/Healer.h"
#include "ActorTypes/LightningBolt.h"
#include "ActorTypes/Fireball.h"
#include "Actor/Confuser.h"
#include "ActorTypes/Gold.h"

HealthPotion::HealthPotion(Vector2D position) : Item(position, ActorData{ '!', "health potion", HPBARMISSING_PAIR })
{
	pickable = std::make_unique<Healer>(10);
}

ScrollOfLightningBolt::ScrollOfLightningBolt(Vector2D position) : Item(position, ActorData{'#', "scroll of lightning bolt", LIGHTNING_PAIR })
{
	pickable = std::make_unique<LightningBolt>(5, 20);
}

ScrollOfFireball::ScrollOfFireball(Vector2D position) : Item(position, ActorData{ '#', "scroll of fireball", FIREBALL_PAIR })
{
	pickable = std::make_unique<Fireball>(3, 12);
}

ScrollOfConfusion::ScrollOfConfusion(Vector2D position) : Item(position, ActorData{ '#', "scroll of confusion", CONFUSION_PAIR })
{
	pickable = std::make_unique<Confuser>(10, 8);
}

GoldPile::GoldPile(Vector2D position) : Item(position, ActorData{ '$', "gold pile", GOLD_PAIR })
{
	// Create a randomized amount of gold (between 5 and 20, increasing with dungeon level)
	int goldAmount = game.d.roll(5, 10 + game.dungeonLevel * 3);
	pickable = std::make_unique<Gold>(goldAmount);
	value = goldAmount; // Set the value equal to the gold amount for consistency
}
