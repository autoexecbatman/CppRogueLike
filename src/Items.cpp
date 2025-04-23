#include "Items.h"
#include "Game.h"
#include "Actor/Actor.h"
#include "Actor/Pickable.h"
#include "ActorTypes/Healer.h"
#include "ActorTypes/LightningBolt.h"
#include "ActorTypes/Fireball.h"
#include "Actor/Confuser.h"
#include "ActorTypes/Gold.h"
#include "Amulet.h"
#include "Armor.h"

HealthPotion::HealthPotion(Vector2D position) : Item(position, ActorData{ '!', "health potion", HPBARMISSING_PAIR })
{
	pickable = std::make_unique<Healer>(10);
	value = 50;
}

ScrollOfLightningBolt::ScrollOfLightningBolt(Vector2D position) : Item(position, ActorData{'#', "scroll of lightning bolt", LIGHTNING_PAIR })
{
	pickable = std::make_unique<LightningBolt>(5, 20);
}

ScrollOfFireball::ScrollOfFireball(Vector2D position) : Item(position, ActorData{ '#', "scroll of fireball", FIREBALL_PAIR })
{
	pickable = std::make_unique<Fireball>(3, 12);
	value = 100;
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

AmuletOfYendor::AmuletOfYendor(Vector2D position) : Item(position, ActorData{ '*', "Amulet of Yendor", FIREBALL_PAIR })
{
	pickable = std::make_unique<Amulet>();
	value = 1000; // Very valuable
}

LeatherArmorItem::LeatherArmorItem(Vector2D position) : Item(position, ActorData{ '[', "leather armor", DOOR_PAIR })
{
	pickable = std::make_unique<LeatherArmor>();
	value = 30; // Set a value for the armor
}

ChainMailItem::ChainMailItem(Vector2D position) : Item(position, ActorData{ '[', "chain mail", DOOR_PAIR })
{
	pickable = std::make_unique<ChainMail>();
	value = 75; // More expensive than leather armor
}

PlateMailItem::PlateMailItem(Vector2D position) : Item(position, ActorData{ '[', "plate mail", DOOR_PAIR })
{
	pickable = std::make_unique<PlateMail>();
	value = 150; // Most expensive armor type
}
