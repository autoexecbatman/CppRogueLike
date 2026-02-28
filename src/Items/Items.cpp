#include "Items.h"
#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "../ActorTypes/Gold.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/Teleporter.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "../Items/ItemClassification.h"
#include "../Systems/ContentRegistry.h"
#include "../Systems/LevelManager.h"
#include "../Systems/TileConfig.h"
#include "Amulet.h"
#include "Armor.h"

HealthPotion::HealthPotion(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::HEALTH_POTION), "health potion", WHITE_RED_PAIR })
{
	pickable = std::make_unique<Healer>(10);
	set_value(50);
}

ScrollOfLightningBolt::ScrollOfLightningBolt(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::SCROLL_LIGHTNING), "scroll of lightning bolt", WHITE_BLUE_PAIR })
{
	pickable = std::make_unique<TargetedScroll>(TargetMode::AUTO_NEAREST, ScrollAnimation::LIGHTNING, 5, 20, 0);
	set_value(150);
}

ScrollOfFireball::ScrollOfFireball(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::SCROLL_FIREBALL), "scroll of fireball", RED_YELLOW_PAIR })
{
	pickable = std::make_unique<TargetedScroll>(TargetMode::PICK_TILE_AOE, ScrollAnimation::EXPLOSION, 3, 12, 0);
	set_value(100);
}

ScrollOfConfusion::ScrollOfConfusion(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::SCROLL_CONFUSION), "scroll of confusion", WHITE_GREEN_PAIR })
{
	pickable = std::make_unique<TargetedScroll>(TargetMode::PICK_TILE_SINGLE, ScrollAnimation::NONE, 10, 0, 8);
	set_value(120);
}

ScrollOfTeleportation::ScrollOfTeleportation(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::SCROLL_TELEPORT), "scroll of teleportation", MAGENTA_BLACK_PAIR })
{
	pickable = std::make_unique<Teleporter>();
	set_value(200);
}

GoldPile::GoldPile(Vector2D position, GameContext& ctx)
	: Item(position, ActorData{ TileConfig::instance().get("TILE_GOLD"), "gold pile", YELLOW_BLACK_PAIR })
{
	// Create a randomized amount of gold (between 5 and 20, increasing with dungeon level)
	int goldAmount = ctx.dice->roll(5, 10 + ctx.level_manager->get_dungeon_level() * 3);
	pickable = std::make_unique<Gold>(goldAmount);
	set_value(goldAmount);
}

AmuletOfYendor::AmuletOfYendor(Vector2D position)
	: Item(position, ActorData{ TileConfig::instance().get("TILE_AMULET_YENDOR"), "Amulet of Yendor", RED_YELLOW_PAIR })
{
	pickable = std::make_unique<Amulet>();
	set_value(1000);
}
