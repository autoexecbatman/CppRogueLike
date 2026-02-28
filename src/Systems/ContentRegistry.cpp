// file: ContentRegistry.cpp
#include <string>

#include "../Factories/MonsterCreator.h"
#include "../Items/ItemClassification.h"
#include "../Renderer/TileId.h"
#include "ContentRegistry.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

ContentRegistry& ContentRegistry::instance()
{
	static ContentRegistry reg;
	return reg;
}

ContentRegistry::ContentRegistry()
{
	bootstrap_items();
	bootstrap_monsters();
}

// ---------------------------------------------------------------------------
// Getters / setters
// ---------------------------------------------------------------------------

int ContentRegistry::get_tile(ItemId id) const
{
	auto key = static_cast<int>(id);
	return m_item_tiles.contains(key) ? m_item_tiles.at(key) : 0;
}

int ContentRegistry::get_tile(MonsterId id) const
{
	auto key = static_cast<int>(id);
	return m_monster_tiles.contains(key) ? m_monster_tiles.at(key) : 0;
}

void ContentRegistry::set_tile(ItemId id, int tile_id)
{
	m_item_tiles[static_cast<int>(id)] = tile_id;
}

void ContentRegistry::set_tile(MonsterId id, int tile_id)
{
	m_monster_tiles[static_cast<int>(id)] = tile_id;
}

// ---------------------------------------------------------------------------
// String keys for JSON serialization
// ---------------------------------------------------------------------------

std::string_view ContentRegistry::item_key(ItemId id)
{
	switch (id)
	{
	case ItemId::UNKNOWN:
		return "unknown";
	// Potions
	case ItemId::HEALTH_POTION:
		return "health_potion";
	case ItemId::POTION_OF_EXTRA_HEALING:
		return "potion_of_extra_healing";
	case ItemId::MANA_POTION:
		return "mana_potion";
	case ItemId::INVISIBILITY_POTION:
		return "invisibility_potion";
	case ItemId::POTION_OF_GIANT_STRENGTH:
		return "potion_of_giant_strength";
	case ItemId::POTION_OF_FIRE_RESISTANCE:
		return "potion_of_fire_resistance";
	case ItemId::POTION_OF_COLD_RESISTANCE:
		return "potion_of_cold_resistance";
	case ItemId::POTION_OF_SPEED:
		return "potion_of_speed";
	// Scrolls
	case ItemId::SCROLL_LIGHTNING:
		return "scroll_lightning";
	case ItemId::SCROLL_FIREBALL:
		return "scroll_fireball";
	case ItemId::SCROLL_CONFUSION:
		return "scroll_confusion";
	case ItemId::SCROLL_TELEPORT:
		return "scroll_teleport";
	// Melee weapons
	case ItemId::DAGGER:
		return "dagger";
	case ItemId::SHORT_SWORD:
		return "short_sword";
	case ItemId::LONG_SWORD:
		return "long_sword";
	case ItemId::BASTARD_SWORD:
		return "bastard_sword";
	case ItemId::TWO_HANDED_SWORD:
		return "two_handed_sword";
	case ItemId::GREAT_SWORD:
		return "great_sword";
	case ItemId::SCIMITAR:
		return "scimitar";
	case ItemId::RAPIER:
		return "rapier";
	case ItemId::HAND_AXE:
		return "hand_axe";
	case ItemId::BATTLE_AXE:
		return "battle_axe";
	case ItemId::GREAT_AXE:
		return "great_axe";
	case ItemId::WAR_HAMMER:
		return "war_hammer";
	case ItemId::MACE:
		return "mace";
	case ItemId::MORNING_STAR:
		return "morning_star";
	case ItemId::FLAIL:
		return "flail";
	case ItemId::CLUB:
		return "club";
	case ItemId::QUARTERSTAFF:
		return "quarterstaff";
	case ItemId::STAFF:
		return "staff";
	// Ranged weapons
	case ItemId::SHORT_BOW:
		return "short_bow";
	case ItemId::LONG_BOW:
		return "long_bow";
	case ItemId::COMPOSITE_BOW:
		return "composite_bow";
	case ItemId::LIGHT_CROSSBOW:
		return "light_crossbow";
	case ItemId::HEAVY_CROSSBOW:
		return "heavy_crossbow";
	case ItemId::CROSSBOW:
		return "crossbow";
	case ItemId::SLING:
		return "sling";
	// Armor
	case ItemId::PADDED_ARMOR:
		return "padded_armor";
	case ItemId::LEATHER_ARMOR:
		return "leather_armor";
	case ItemId::STUDDED_LEATHER:
		return "studded_leather";
	case ItemId::HIDE_ARMOR:
		return "hide_armor";
	case ItemId::RING_MAIL:
		return "ring_mail";
	case ItemId::SCALE_MAIL:
		return "scale_mail";
	case ItemId::CHAIN_MAIL:
		return "chain_mail";
	case ItemId::BRIGANDINE:
		return "brigandine";
	case ItemId::SPLINT_MAIL:
		return "splint_mail";
	case ItemId::BANDED_MAIL:
		return "banded_mail";
	case ItemId::PLATE_MAIL:
		return "plate_mail";
	case ItemId::FIELD_PLATE:
		return "field_plate";
	case ItemId::FULL_PLATE:
		return "full_plate";
	// Shields
	case ItemId::SMALL_SHIELD:
		return "small_shield";
	case ItemId::MEDIUM_SHIELD:
		return "medium_shield";
	case ItemId::LARGE_SHIELD:
		return "large_shield";
	// Helmets
	case ItemId::HELM_OF_BRILLIANCE:
		return "helm_of_brilliance";
	case ItemId::HELM_OF_TELEPORTATION:
		return "helm_of_teleportation";
	case ItemId::HELM_OF_TELEPATHY:
		return "helm_of_telepathy";
	case ItemId::HELM_OF_UNDERWATER_ACTION:
		return "helm_of_underwater_action";
	// Rings
	case ItemId::RING_OF_PROTECTION_PLUS_1:
		return "ring_of_protection_plus_1";
	case ItemId::RING_OF_PROTECTION_PLUS_2:
		return "ring_of_protection_plus_2";
	case ItemId::RING_OF_FREE_ACTION:
		return "ring_of_free_action";
	case ItemId::RING_OF_REGENERATION:
		return "ring_of_regeneration";
	case ItemId::RING_OF_INVISIBILITY:
		return "ring_of_invisibility";
	case ItemId::RING_OF_FIRE_RESISTANCE:
		return "ring_of_fire_resistance";
	case ItemId::RING_OF_COLD_RESISTANCE:
		return "ring_of_cold_resistance";
	case ItemId::RING_OF_SPELL_STORING:
		return "ring_of_spell_storing";
	// Amulets
	case ItemId::AMULET_OF_HEALTH:
		return "amulet_of_health";
	case ItemId::AMULET_OF_WISDOM:
		return "amulet_of_wisdom";
	case ItemId::AMULET_OF_PROTECTION:
		return "amulet_of_protection";
	case ItemId::AMULET_OF_OGRE_POWER:
		return "amulet_of_ogre_power";
	case ItemId::AMULET_OF_YENDOR:
		return "amulet_of_yendor";
	// Gauntlets
	case ItemId::GAUNTLETS_OF_OGRE_POWER:
		return "gauntlets_of_ogre_power";
	case ItemId::GAUNTLETS_OF_DEXTERITY:
		return "gauntlets_of_dexterity";
	case ItemId::GAUNTLETS_OF_SWIMMING_AND_CLIMBING:
		return "gauntlets_of_swimming_and_climbing";
	case ItemId::GAUNTLETS_OF_FUMBLING:
		return "gauntlets_of_fumbling";
	// Girdles
	case ItemId::GIRDLE_OF_HILL_GIANT_STRENGTH:
		return "girdle_of_hill_giant_strength";
	case ItemId::GIRDLE_OF_STONE_GIANT_STRENGTH:
		return "girdle_of_stone_giant_strength";
	case ItemId::GIRDLE_OF_FROST_GIANT_STRENGTH:
		return "girdle_of_frost_giant_strength";
	case ItemId::GIRDLE_OF_FIRE_GIANT_STRENGTH:
		return "girdle_of_fire_giant_strength";
	case ItemId::GIRDLE_OF_CLOUD_GIANT_STRENGTH:
		return "girdle_of_cloud_giant_strength";
	case ItemId::GIRDLE_OF_STORM_GIANT_STRENGTH:
		return "girdle_of_storm_giant_strength";
	// Boots
	case ItemId::BOOTS_OF_SPEED:
		return "boots_of_speed";
	case ItemId::BOOTS_OF_ELVENKIND:
		return "boots_of_elvenkind";
	// Cloaks
	case ItemId::CLOAK_OF_PROTECTION:
		return "cloak_of_protection";
	case ItemId::CLOAK_OF_DISPLACEMENT:
		return "cloak_of_displacement";
	case ItemId::CLOAK_OF_ELVENKIND:
		return "cloak_of_elvenkind";
	// Food
	case ItemId::FOOD_RATION:
		return "food_ration";
	case ItemId::BREAD:
		return "bread";
	case ItemId::MEAT:
		return "meat";
	case ItemId::FRUIT:
		return "fruit";
	// Treasure
	case ItemId::GOLD_COIN:
		return "gold";
	case ItemId::GEM:
		return "gem";
	// Tools
	case ItemId::TORCH:
		return "torch";
	case ItemId::ROPE:
		return "rope";
	case ItemId::LOCKPICK:
		return "lockpick";
	}
	return "unknown";
}

std::string_view ContentRegistry::monster_key(MonsterId id)
{
	switch (id)
	{
	case MonsterId::GOBLIN:
		return "goblin";
	case MonsterId::ORC:
		return "orc";
	case MonsterId::TROLL:
		return "troll";
	case MonsterId::DRAGON:
		return "dragon";
	case MonsterId::ARCHER:
		return "archer";
	case MonsterId::MAGE:
		return "mage";
	case MonsterId::WOLF:
		return "wolf";
	case MonsterId::FIRE_WOLF:
		return "fire_wolf";
	case MonsterId::ICE_WOLF:
		return "ice_wolf";
	case MonsterId::BAT:
		return "bat";
	case MonsterId::KOBOLD:
		return "kobold";
	case MonsterId::MIMIC:
		return "mimic";
	case MonsterId::SHOPKEEPER:
		return "shopkeeper";
	case MonsterId::SPIDER_SMALL:
		return "spider_small";
	case MonsterId::SPIDER_GIANT:
		return "spider_giant";
	case MonsterId::SPIDER_WEAVER:
		return "spider_weaver";
	}
	return "unknown";
}

// ---------------------------------------------------------------------------
// Bootstrap defaults -- mirrors TileId.h constants exactly.
// Use set_tile() or the GameEditor to override per-item at runtime.
// ---------------------------------------------------------------------------

void ContentRegistry::bootstrap_items()
{
	auto set = [this](ItemId id, int tile_id)
	{
		m_item_tiles[static_cast<int>(id)] = tile_id;
	};

	// Potions -- sequential columns in SHEET_POTION
	set(ItemId::HEALTH_POTION, make_tile(TileSheet::SHEET_POTION, 0, 0));
	set(ItemId::POTION_OF_EXTRA_HEALING, make_tile(TileSheet::SHEET_POTION, 1, 0));
	set(ItemId::MANA_POTION, make_tile(TileSheet::SHEET_POTION, 2, 0));
	set(ItemId::INVISIBILITY_POTION, make_tile(TileSheet::SHEET_POTION, 3, 0));
	set(ItemId::POTION_OF_GIANT_STRENGTH, make_tile(TileSheet::SHEET_POTION, 4, 0));
	set(ItemId::POTION_OF_FIRE_RESISTANCE, make_tile(TileSheet::SHEET_POTION, 5, 0));
	set(ItemId::POTION_OF_COLD_RESISTANCE, make_tile(TileSheet::SHEET_POTION, 6, 0));
	set(ItemId::POTION_OF_SPEED, make_tile(TileSheet::SHEET_POTION, 7, 0));

	// Scrolls
	set(ItemId::SCROLL_LIGHTNING, make_tile(TileSheet::SHEET_SCROLL, 0, 0));
	set(ItemId::SCROLL_FIREBALL, make_tile(TileSheet::SHEET_SCROLL, 1, 0));
	set(ItemId::SCROLL_CONFUSION, make_tile(TileSheet::SHEET_SCROLL, 2, 0));
	set(ItemId::SCROLL_TELEPORT, make_tile(TileSheet::SHEET_SCROLL, 3, 0));

	// Melee -- short weapons
	set(ItemId::DAGGER, make_tile(TileSheet::SHEET_SHORT_WEP, 0, 0));
	set(ItemId::SHORT_SWORD, make_tile(TileSheet::SHEET_SHORT_WEP, 1, 0));
	set(ItemId::SCIMITAR, make_tile(TileSheet::SHEET_SHORT_WEP, 2, 0));
	set(ItemId::RAPIER, make_tile(TileSheet::SHEET_SHORT_WEP, 3, 0));
	set(ItemId::HAND_AXE, make_tile(TileSheet::SHEET_SHORT_WEP, 4, 0));
	set(ItemId::MACE, make_tile(TileSheet::SHEET_SHORT_WEP, 5, 0));
	set(ItemId::CLUB, make_tile(TileSheet::SHEET_SHORT_WEP, 6, 0));

	// Melee -- medium weapons
	set(ItemId::LONG_SWORD, make_tile(TileSheet::SHEET_MED_WEP, 0, 0));
	set(ItemId::BASTARD_SWORD, make_tile(TileSheet::SHEET_MED_WEP, 1, 0));
	set(ItemId::BATTLE_AXE, make_tile(TileSheet::SHEET_MED_WEP, 2, 0));
	set(ItemId::WAR_HAMMER, make_tile(TileSheet::SHEET_MED_WEP, 3, 0));
	set(ItemId::MORNING_STAR, make_tile(TileSheet::SHEET_MED_WEP, 4, 0));
	set(ItemId::FLAIL, make_tile(TileSheet::SHEET_MED_WEP, 5, 0));

	// Melee -- long weapons
	set(ItemId::TWO_HANDED_SWORD, make_tile(TileSheet::SHEET_LONG_WEP, 0, 0));
	set(ItemId::GREAT_SWORD, make_tile(TileSheet::SHEET_LONG_WEP, 1, 0));
	set(ItemId::GREAT_AXE, make_tile(TileSheet::SHEET_LONG_WEP, 2, 0));
	set(ItemId::QUARTERSTAFF, make_tile(TileSheet::SHEET_LONG_WEP, 3, 0));
	set(ItemId::STAFF, make_tile(TileSheet::SHEET_LONG_WEP, 4, 0));

	// Ranged weapons (SHEET_SHORT_WEP rows 4+)
	set(ItemId::SHORT_BOW, make_tile(TileSheet::SHEET_SHORT_WEP, 0, 4));
	set(ItemId::LONG_BOW, make_tile(TileSheet::SHEET_SHORT_WEP, 1, 4));
	set(ItemId::COMPOSITE_BOW, make_tile(TileSheet::SHEET_SHORT_WEP, 2, 4));
	set(ItemId::LIGHT_CROSSBOW, make_tile(TileSheet::SHEET_SHORT_WEP, 3, 4));
	set(ItemId::HEAVY_CROSSBOW, make_tile(TileSheet::SHEET_SHORT_WEP, 4, 4));
	set(ItemId::CROSSBOW, make_tile(TileSheet::SHEET_SHORT_WEP, 5, 4));
	set(ItemId::SLING, make_tile(TileSheet::SHEET_SHORT_WEP, 6, 4));

	// Armor -- row 0: light to heavy
	set(ItemId::PADDED_ARMOR, make_tile(TileSheet::SHEET_ARMOR, 0, 0));
	set(ItemId::LEATHER_ARMOR, make_tile(TileSheet::SHEET_ARMOR, 1, 0));
	set(ItemId::STUDDED_LEATHER, make_tile(TileSheet::SHEET_ARMOR, 2, 0));
	set(ItemId::HIDE_ARMOR, make_tile(TileSheet::SHEET_ARMOR, 3, 0));
	set(ItemId::RING_MAIL, make_tile(TileSheet::SHEET_ARMOR, 4, 0));
	set(ItemId::SCALE_MAIL, make_tile(TileSheet::SHEET_ARMOR, 5, 0));
	set(ItemId::CHAIN_MAIL, make_tile(TileSheet::SHEET_ARMOR, 6, 0));
	set(ItemId::BRIGANDINE, make_tile(TileSheet::SHEET_ARMOR, 7, 0));
	// Armor -- row 1: heavier
	set(ItemId::SPLINT_MAIL, make_tile(TileSheet::SHEET_ARMOR, 0, 1));
	set(ItemId::BANDED_MAIL, make_tile(TileSheet::SHEET_ARMOR, 1, 1));
	set(ItemId::PLATE_MAIL, make_tile(TileSheet::SHEET_ARMOR, 2, 1));
	set(ItemId::FIELD_PLATE, make_tile(TileSheet::SHEET_ARMOR, 3, 1));
	set(ItemId::FULL_PLATE, make_tile(TileSheet::SHEET_ARMOR, 4, 1));

	// Shields
	set(ItemId::SMALL_SHIELD, make_tile(TileSheet::SHEET_SHIELD, 0, 0));
	set(ItemId::MEDIUM_SHIELD, make_tile(TileSheet::SHEET_SHIELD, 1, 0));
	set(ItemId::LARGE_SHIELD, make_tile(TileSheet::SHEET_SHIELD, 2, 0));

	// Helmets
	set(ItemId::HELM_OF_BRILLIANCE, make_tile(TileSheet::SHEET_HAT, 0, 0));
	set(ItemId::HELM_OF_TELEPORTATION, make_tile(TileSheet::SHEET_HAT, 1, 0));
	set(ItemId::HELM_OF_TELEPATHY, make_tile(TileSheet::SHEET_HAT, 2, 0));
	set(ItemId::HELM_OF_UNDERWATER_ACTION, make_tile(TileSheet::SHEET_HAT, 3, 0));

	// Rings
	set(ItemId::RING_OF_PROTECTION_PLUS_1, make_tile(TileSheet::SHEET_RING, 0, 0));
	set(ItemId::RING_OF_PROTECTION_PLUS_2, make_tile(TileSheet::SHEET_RING, 1, 0));
	set(ItemId::RING_OF_FREE_ACTION, make_tile(TileSheet::SHEET_RING, 2, 0));
	set(ItemId::RING_OF_REGENERATION, make_tile(TileSheet::SHEET_RING, 3, 0));
	set(ItemId::RING_OF_INVISIBILITY, make_tile(TileSheet::SHEET_RING, 4, 0));
	set(ItemId::RING_OF_FIRE_RESISTANCE, make_tile(TileSheet::SHEET_RING, 5, 0));
	set(ItemId::RING_OF_COLD_RESISTANCE, make_tile(TileSheet::SHEET_RING, 6, 0));
	set(ItemId::RING_OF_SPELL_STORING, make_tile(TileSheet::SHEET_RING, 7, 0));

	// Amulets
	set(ItemId::AMULET_OF_HEALTH, make_tile(TileSheet::SHEET_AMULET_ITEM, 0, 0));
	set(ItemId::AMULET_OF_WISDOM, make_tile(TileSheet::SHEET_AMULET_ITEM, 1, 0));
	set(ItemId::AMULET_OF_PROTECTION, make_tile(TileSheet::SHEET_AMULET_ITEM, 3, 0));
	set(ItemId::AMULET_OF_OGRE_POWER, make_tile(TileSheet::SHEET_AMULET_ITEM, 4, 0));
	set(ItemId::AMULET_OF_YENDOR, TILE_AMULET_YENDOR);

	// Gauntlets (armor row 3)
	set(ItemId::GAUNTLETS_OF_OGRE_POWER, make_tile(TileSheet::SHEET_ARMOR, 0, 3));
	set(ItemId::GAUNTLETS_OF_DEXTERITY, make_tile(TileSheet::SHEET_ARMOR, 1, 3));
	set(ItemId::GAUNTLETS_OF_SWIMMING_AND_CLIMBING, make_tile(TileSheet::SHEET_ARMOR, 2, 3));
	set(ItemId::GAUNTLETS_OF_FUMBLING, make_tile(TileSheet::SHEET_ARMOR, 3, 3));

	// Girdles (armor row 2)
	set(ItemId::GIRDLE_OF_HILL_GIANT_STRENGTH, make_tile(TileSheet::SHEET_ARMOR, 0, 2));
	set(ItemId::GIRDLE_OF_STONE_GIANT_STRENGTH, make_tile(TileSheet::SHEET_ARMOR, 1, 2));
	set(ItemId::GIRDLE_OF_FROST_GIANT_STRENGTH, make_tile(TileSheet::SHEET_ARMOR, 2, 2));
	set(ItemId::GIRDLE_OF_FIRE_GIANT_STRENGTH, make_tile(TileSheet::SHEET_ARMOR, 3, 2));
	set(ItemId::GIRDLE_OF_CLOUD_GIANT_STRENGTH, make_tile(TileSheet::SHEET_ARMOR, 4, 2));
	set(ItemId::GIRDLE_OF_STORM_GIANT_STRENGTH, make_tile(TileSheet::SHEET_ARMOR, 5, 2));

	// Boots and cloaks (armor row 4)
	set(ItemId::BOOTS_OF_SPEED, make_tile(TileSheet::SHEET_ARMOR, 0, 4));
	set(ItemId::BOOTS_OF_ELVENKIND, make_tile(TileSheet::SHEET_ARMOR, 1, 4));
	set(ItemId::CLOAK_OF_PROTECTION, make_tile(TileSheet::SHEET_ARMOR, 2, 4));
	set(ItemId::CLOAK_OF_DISPLACEMENT, make_tile(TileSheet::SHEET_ARMOR, 3, 4));
	set(ItemId::CLOAK_OF_ELVENKIND, make_tile(TileSheet::SHEET_ARMOR, 4, 4));

	// Food
	set(ItemId::FOOD_RATION, make_tile(TileSheet::SHEET_FOOD, 0, 0));
	set(ItemId::BREAD, make_tile(TileSheet::SHEET_FOOD, 1, 0));
	set(ItemId::MEAT, make_tile(TileSheet::SHEET_FOOD, 2, 0));
	set(ItemId::FRUIT, make_tile(TileSheet::SHEET_FOOD, 3, 0));

	// Treasure
	set(ItemId::GOLD_COIN, make_tile(TileSheet::SHEET_MONEY, 0, 0));
	set(ItemId::GEM, make_tile(TileSheet::SHEET_MONEY, 1, 0));

	// Tools
	set(ItemId::TORCH, make_tile(TileSheet::SHEET_MISC0, 0, 0));
	set(ItemId::ROPE, make_tile(TileSheet::SHEET_MISC0, 1, 0));
	set(ItemId::LOCKPICK, make_tile(TileSheet::SHEET_MISC0, 2, 0));
}

void ContentRegistry::bootstrap_monsters()
{
	auto set = [this](MonsterId id, int tile_id)
	{
		m_monster_tiles[static_cast<int>(id)] = tile_id;
	};

	set(MonsterId::GOBLIN, TILE_GOBLIN);
	set(MonsterId::ORC, TILE_ORC);
	set(MonsterId::TROLL, TILE_TROLL);
	set(MonsterId::DRAGON, TILE_DRAGON);
	set(MonsterId::ARCHER, TILE_ARCHER);
	set(MonsterId::MAGE, TILE_MAGE);
	set(MonsterId::WOLF, TILE_WOLF);
	set(MonsterId::FIRE_WOLF, TILE_FIRE_WOLF);
	set(MonsterId::ICE_WOLF, TILE_ICE_WOLF);
	set(MonsterId::BAT, TILE_BAT);
	set(MonsterId::KOBOLD, TILE_KOBOLD);
	set(MonsterId::MIMIC, TILE_MIMIC);
	set(MonsterId::SHOPKEEPER, TILE_SHOPKEEPER);
	set(MonsterId::SPIDER_SMALL, TILE_SPIDER_SMALL);
	set(MonsterId::SPIDER_GIANT, TILE_SPIDER_GIANT);
	set(MonsterId::SPIDER_WEAVER, TILE_SPIDER_WEAVER);
}

// end of file: ContentRegistry.cpp
