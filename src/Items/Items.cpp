// file: Items.cpp
#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Items/ItemClassification.h"
#include "../Random/RandomDice.h"
#include "../Systems/BuffType.h"
#include "../Systems/ContentRegistry.h"
#include "../Systems/LevelManager.h"
#include "../Systems/TargetMode.h"
#include "../Systems/TileConfig.h"
#include "../Utils/Vector2D.h"
#include "Items.h"

HealthPotion::HealthPotion(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::HEALTH_POTION), "health potion", WHITE_RED_PAIR })
{
	behavior = Consumable{ ConsumableEffect::HEAL, 10, 0, BuffType::NONE, false };
	set_value(50);
}

ScrollOfLightningBolt::ScrollOfLightningBolt(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::SCROLL_LIGHTNING), "scroll of lightning bolt", WHITE_BLUE_PAIR })
{
	behavior = TargetedScroll{ TargetMode::AUTO_NEAREST, ScrollAnimation::LIGHTNING, 5, 20, 0, BuffType::NONE, 0 };
	set_value(150);
}

ScrollOfFireball::ScrollOfFireball(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::SCROLL_FIREBALL), "scroll of fireball", RED_YELLOW_PAIR })
{
	behavior = TargetedScroll{ TargetMode::PICK_TILE_AOE, ScrollAnimation::EXPLOSION, 3, 12, 0, BuffType::NONE, 0 };
	set_value(100);
}

ScrollOfConfusion::ScrollOfConfusion(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::SCROLL_CONFUSION), "scroll of confusion", WHITE_GREEN_PAIR })
{
	behavior = TargetedScroll{ TargetMode::PICK_TILE_SINGLE, ScrollAnimation::NONE, 10, 0, 8, BuffType::NONE, 0 };
	set_value(120);
}

ScrollOfTeleportation::ScrollOfTeleportation(Vector2D position)
	: Item(position, ActorData{ ContentRegistry::instance().get_tile(ItemId::SCROLL_TELEPORT), "scroll of teleportation", MAGENTA_BLACK_PAIR })
{
	behavior = Teleporter{};
	set_value(200);
}

GoldPile::GoldPile(Vector2D position, GameContext& ctx)
	: Item(position, ActorData{ ctx.tile_config->get("TILE_GOLD"), "gold pile", YELLOW_BLACK_PAIR })
{
	const int goldAmount = ctx.dice->roll(5, 10 + ctx.level_manager->get_dungeon_level() * 3);
	behavior = Gold{ goldAmount };
	set_value(goldAmount);
}

AmuletOfYendor::AmuletOfYendor(Vector2D position, const TileConfig& tc)
	: Item(position, ActorData{ tc.get("TILE_AMULET_YENDOR"), "Amulet of Yendor", RED_YELLOW_PAIR })
{
	behavior = Amulet{};
	set_value(1000);
}
