#include "Item.h"

Item::Item(Vector2D position, ActorData data)
	: Object(position, data) {
		  // ItemClass should be set by ItemCreator, not by fragile string matching
	  };

void Item::load(const json& j)
{
	Object::load(j); // Call base class load
	baseValue = j.at("baseValue").get<int>();
	if (j.contains("itemId"))
	{
		itemId = static_cast<ItemId>(j.at("itemId").get<int>());
	}
	itemClass = static_cast<ItemClass>(j.at("itemClass").get<int>());

	// Load enhancement data
	if (j.contains("enhancement"))
	{
		const auto& enh = j["enhancement"];
		enhancement.prefix = static_cast<PrefixType>(enh.at("prefix").get<int>());
		enhancement.suffix = static_cast<SuffixType>(enh.at("suffix").get<int>());
		enhancement.damage_bonus = enh.at("damage_bonus").get<int>();
		enhancement.to_hit_bonus = enh.at("to_hit_bonus").get<int>();
		enhancement.ac_bonus = enh.at("ac_bonus").get<int>();
		enhancement.strength_bonus = enh.at("strength_bonus").get<int>();
		enhancement.dexterity_bonus = enh.at("dexterity_bonus").get<int>();
		enhancement.intelligence_bonus = enh.at("intelligence_bonus").get<int>();
		enhancement.hp_bonus = enh.at("hp_bonus").get<int>();
		enhancement.mana_bonus = enh.at("mana_bonus").get<int>();
		enhancement.speed_bonus = enh.at("speed_bonus").get<int>();
		enhancement.stealth_bonus = enh.at("stealth_bonus").get<int>();
		enhancement.fire_resistance = enh.at("fire_resistance").get<int>();
		enhancement.cold_resistance = enh.at("cold_resistance").get<int>();
		enhancement.lightning_resistance = enh.at("lightning_resistance").get<int>();
		enhancement.poison_resistance = enh.at("poison_resistance").get<int>();
		enhancement.is_cursed = enh.at("is_cursed").get<bool>();
		enhancement.is_blessed = enh.at("is_blessed").get<bool>();
		enhancement.is_magical = enh.at("is_magical").get<bool>();
		enhancement.enhancement_level = enh.at("enhancement_level").get<int>();
		enhancement.value_modifier = enh.at("value_modifier").get<int>();
	}

	if (j.contains("pickable"))
		behavior = load_behavior(j["pickable"]);
}

void Item::save(json& j)
{
	Object::save(j); // Call base class save
	j["baseValue"] = baseValue;
	j["itemId"] = static_cast<int>(itemId);
	j["itemClass"] = static_cast<int>(itemClass);

	// Save enhancement data
	json enh;
	enh["prefix"] = static_cast<int>(enhancement.prefix);
	enh["suffix"] = static_cast<int>(enhancement.suffix);
	enh["damage_bonus"] = enhancement.damage_bonus;
	enh["to_hit_bonus"] = enhancement.to_hit_bonus;
	enh["ac_bonus"] = enhancement.ac_bonus;
	enh["strength_bonus"] = enhancement.strength_bonus;
	enh["dexterity_bonus"] = enhancement.dexterity_bonus;
	enh["intelligence_bonus"] = enhancement.intelligence_bonus;
	enh["hp_bonus"] = enhancement.hp_bonus;
	enh["mana_bonus"] = enhancement.mana_bonus;
	enh["speed_bonus"] = enhancement.speed_bonus;
	enh["stealth_bonus"] = enhancement.stealth_bonus;
	enh["fire_resistance"] = enhancement.fire_resistance;
	enh["cold_resistance"] = enhancement.cold_resistance;
	enh["lightning_resistance"] = enhancement.lightning_resistance;
	enh["poison_resistance"] = enhancement.poison_resistance;
	enh["is_cursed"] = enhancement.is_cursed;
	enh["is_blessed"] = enhancement.is_blessed;
	enh["is_magical"] = enhancement.is_magical;
	enh["enhancement_level"] = enhancement.enhancement_level;
	enh["value_modifier"] = enhancement.value_modifier;
	j["enhancement"] = enh;

	if (behavior)
	{
		json pickableJson;
		save_behavior(*behavior, pickableJson);
		j["pickable"] = pickableJson;
	}
}

const std::string& Item::get_name() const noexcept
{
	if (is_enhanced())
	{
		static std::string enhanced_name;
		enhanced_name = enhancement.get_full_name(actorData.name);
		return enhanced_name;
	}
	return actorData.name;
}

void Item::apply_enhancement(const ItemEnhancement& new_enhancement)
{
	enhancement = new_enhancement;
	// Name is computed dynamically by get_name() -- do not modify actorData.name
}

void Item::generate_random_enhancement(bool allow_magical)
{
	if (is_weapon())
	{
		enhancement = ItemEnhancement::generate_weapon_enhancement();
	}
	else if (is_armor())
	{
		enhancement = ItemEnhancement::generate_armor_enhancement();
	}
	else
	{
		enhancement = ItemEnhancement::generate_random_enhancement(allow_magical);
	}
}

bool Item::is_enhanced() const noexcept
{
	return enhancement.prefix != PrefixType::NONE || enhancement.suffix != SuffixType::NONE;
}