#include "Items.h"
#include "../Game.h"
#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Fireball.h"
#include "../Actor/Confuser.h"
#include "../ActorTypes/Gold.h"
#include "Amulet.h"
#include "Armor.h"

HealthPotion::HealthPotion(Vector2D position) : Item(position, ActorData{ '!', "health potion", WHITE_RED_PAIR })
{
	pickable = std::make_unique<Healer>(10);
	value = 50;
}

ScrollOfLightningBolt::ScrollOfLightningBolt(Vector2D position) : Item(position, ActorData{'#', "scroll of lightning bolt", WHITE_BLUE_PAIR })
{
	pickable = std::make_unique<LightningBolt>(5, 20);
	value = 150; // 150 gp - powerful lightning magic
}

ScrollOfFireball::ScrollOfFireball(Vector2D position) : Item(position, ActorData{ '#', "scroll of fireball", RED_YELLOW_PAIR })
{
	pickable = std::make_unique<Fireball>(3, 12);
	value = 100;
}

ScrollOfConfusion::ScrollOfConfusion(Vector2D position) : Item(position, ActorData{ '#', "scroll of confusion", WHITE_GREEN_PAIR })
{
	pickable = std::make_unique<Confuser>(10, 8);
	value = 120; // 120 gp - tactical confusion magic
}

GoldPile::GoldPile(Vector2D position) : Item(position, ActorData{ '$', "gold pile", YELLOW_BLACK_PAIR })
{
	// Create a randomized amount of gold (between 5 and 20, increasing with dungeon level)
	int goldAmount = game.d.roll(5, 10 + game.dungeonLevel * 3);
	pickable = std::make_unique<Gold>(goldAmount);
	value = goldAmount; // Set the value equal to the gold amount for consistency
}

AmuletOfYendor::AmuletOfYendor(Vector2D position) : Item(position, ActorData{ '*', "Amulet of Yendor", RED_YELLOW_PAIR })
{
	pickable = std::make_unique<Amulet>();
	value = 1000; // Very valuable
}

LeatherArmorItem::LeatherArmorItem(Vector2D position) : Item(position, ActorData{ '[', "leather armor", BROWN_BLACK_PAIR })
{
	pickable = std::make_unique<LeatherArmor>();
	value = 5; // 5 gp - AD&D 2e leather armor price
}

ChainMailItem::ChainMailItem(Vector2D position) : Item(position, ActorData{ '[', "chain mail", BROWN_BLACK_PAIR })
{
	pickable = std::make_unique<ChainMail>();
	value = 75; // 75 gp - AD&D 2e chain mail price
}

PlateMailItem::PlateMailItem(Vector2D position) : Item(position, ActorData{ '[', "plate mail", BROWN_BLACK_PAIR })
{
	pickable = std::make_unique<PlateMail>();
	value = 400; // 400 gp - AD&D 2e plate mail price
}
