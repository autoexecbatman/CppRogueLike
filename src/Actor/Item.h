#pragma once
#include <optional>
#include <string>

#include "../Items/ItemClassification.h"
#include "../Persistent/Persistent.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"
#include "../Utils/Vector2D.h"
#include "Actor.h"
#include "Object.h"
#include "Pickable.h"

class Item : public Object
{
private:
	int baseValue{ 1 }; // base price set at creation; get_value() applies enhancement modifier

public:
	Item(Vector2D position, ActorData data);

	void load(const json& j) override;
	void save(json& j) override;

	// Name accessor - returns enhanced name if item has enhancements
	const std::string& get_name() const noexcept;
	const std::string& get_base_name() const noexcept { return actorData.name; }

	// Enhancement system
	void apply_enhancement(const ItemEnhancement& enhancement);
	void generate_random_enhancement(bool allow_magical);
	const ItemEnhancement& get_enhancement() const noexcept { return enhancement; }
	bool is_enhanced() const noexcept;

	// Value: baseValue * enhancement.value_modifier / 100  (modifier defaults to 100)
	int get_value() const noexcept { return (baseValue * enhancement.value_modifier) / 100; }
	void set_value(int v) noexcept { baseValue = v; }

	ItemId itemId{ ItemId::UNKNOWN }; // Specific item identity
	ItemClass itemClass{ ItemClass::UNKNOWN }; // Item category classification
	ItemEnhancement enhancement; // Enhancement data

	std::optional<ItemBehavior> behavior; // item behavior (replaces pickable hierarchy)

	// Item classification utility functions
	ItemCategory get_category() const noexcept { return ItemClassificationUtils::get_category(itemClass); }
	bool is_weapon() const noexcept { return ItemClassificationUtils::is_weapon(itemClass); }
	bool is_armor() const noexcept { return ItemClassificationUtils::is_armor(itemClass); }
	bool is_helmet() const noexcept { return ItemClassificationUtils::is_helmet(itemClass); }
	bool is_shield() const noexcept { return ItemClassificationUtils::is_shield(itemClass); }
	bool is_gauntlets() const noexcept { return ItemClassificationUtils::is_gauntlets(itemClass); }
	bool is_girdle() const noexcept { return ItemClassificationUtils::is_girdle(itemClass); }
	bool is_consumable() const noexcept { return ItemClassificationUtils::is_consumable(itemClass); }
	bool is_jewelry() const noexcept { return ItemClassificationUtils::is_jewelry(itemClass); }
	bool is_amulet() const noexcept { return ItemClassificationUtils::is_amulet(itemClass); }
	bool is_ring() const noexcept { return ItemClassificationUtils::is_ring(itemClass); }
	bool is_tool() const noexcept { return ItemClassificationUtils::is_tool(itemClass); }
	bool can_equip_to_right_hand() const noexcept { return ItemClassificationUtils::can_equip_to_right_hand(itemClass); }
	bool can_equip_to_left_hand() const noexcept { return ItemClassificationUtils::can_equip_to_left_hand(itemClass); }
	bool can_equip_to_body() const noexcept { return ItemClassificationUtils::can_equip_to_body(itemClass); }
	bool is_two_handed_weapon() const noexcept { return ItemClassificationUtils::is_two_handed_weapon(itemClass); }
	bool is_ranged_weapon() const noexcept { return ItemClassificationUtils::is_ranged_weapon(itemClass); }
};
