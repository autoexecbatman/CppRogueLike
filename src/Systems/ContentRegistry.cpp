// file: ContentRegistry.cpp
#include <string>

#include "../Items/ItemClassification.h"
#include "../Renderer/Renderer.h"
#include "ContentRegistry.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

ContentRegistry& ContentRegistry::instance()
{
	static ContentRegistry reg;
	return reg;
}

// ---------------------------------------------------------------------------
// Getters / setters
// ---------------------------------------------------------------------------

TileRef ContentRegistry::get_tile(ItemId id) const
{
	auto key = static_cast<int>(id);
	return m_item_tiles.contains(key) ? m_item_tiles.at(key) : TileRef{};
}

void ContentRegistry::set_tile(ItemId id, TileRef tile)
{
	m_item_tiles[static_cast<int>(id)] = tile;
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
	case ItemId::SCROLL_SLEEP:
		return "scroll_sleep";
	case ItemId::SCROLL_HOLD_PERSON:
		return "scroll_hold_person";
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

// end of file: ContentRegistry.cpp
