#include "Item.h"

Item::Item(Vector2D position, ActorData data)
	: Object(position, data) {
		  // ItemClass should be set by ItemCreator, not by fragile string matching
	  };

void Item::load(const json& j)
{
	Object::load(j); // Call base class load
	baseValue = j.at("baseValue").get<int>();
	if (j.contains("itemKey"))
	{
		itemKey = j.at("itemKey").get<std::string>();
	}
	itemClass = static_cast<ItemClass>(j.at("itemClass").get<int>());

	// Load enhancement data
	if (j.contains("enhancement"))
	{
		const auto& enh = j["enhancement"];
		enhancement.prefix = static_cast<PrefixType>(enh.at("prefix").get<int>());
		enhancement.suffix = static_cast<SuffixType>(enh.at("suffix").get<int>());
		enhancement.damageBonus = enh.at("damageBonus").get<int>();
		enhancement.toHitBonus = enh.at("toHitBonus").get<int>();
		enhancement.acBonus = enh.at("acBonus").get<int>();
		enhancement.strengthBonus = enh.at("strengthBonus").get<int>();
		enhancement.dexterityBonus = enh.at("dexterityBonus").get<int>();
		enhancement.intelligenceBonus = enh.at("intelligenceBonus").get<int>();
		enhancement.hpBonus = enh.at("hpBonus").get<int>();
		enhancement.manaBonus = enh.at("manaBonus").get<int>();
		enhancement.speedBonus = enh.at("speedBonus").get<int>();
		enhancement.stealthBonus = enh.at("stealthBonus").get<int>();
		enhancement.fireResistance = enh.at("fireResistance").get<int>();
		enhancement.coldResistance = enh.at("coldResistance").get<int>();
		enhancement.lightningResistance = enh.at("lightningResistance").get<int>();
		enhancement.poisonResistance = enh.at("poisonResistance").get<int>();
		enhancement.blessing = static_cast<BlessingStatus>(enh.at("blessing").get<int>());
		enhancement.isMagical = enh.at("isMagical").get<bool>();
		enhancement.enhancementLevel = enh.at("enhancementLevel").get<int>();
		enhancement.valueModifier = enh.at("valueModifier").get<int>();
	}

	// Load identification status
	if (j.contains("identification"))
	{
		const auto& id = j["identification"];
		identification.identifiedType = id.at("identifiedType").get<bool>();
		identification.identifiedEnhancement = id.at("identifiedEnhancement").get<bool>();
		identification.identifiedBuc = id.at("identifiedBuc").get<bool>();
	}

	if (j.contains("pickable"))
		behavior = load_behavior(j["pickable"]);
}

void Item::save(json& j)
{
	Object::save(j); // Call base class save
	j["baseValue"] = baseValue;
	j["itemKey"] = itemKey;
	j["itemClass"] = static_cast<int>(itemClass);

	// Save enhancement data
	json enh;
	enh["prefix"] = static_cast<int>(enhancement.prefix);
	enh["suffix"] = static_cast<int>(enhancement.suffix);
	enh["damageBonus"] = enhancement.damageBonus;
	enh["toHitBonus"] = enhancement.toHitBonus;
	enh["acBonus"] = enhancement.acBonus;
	enh["strengthBonus"] = enhancement.strengthBonus;
	enh["dexterityBonus"] = enhancement.dexterityBonus;
	enh["intelligenceBonus"] = enhancement.intelligenceBonus;
	enh["hpBonus"] = enhancement.hpBonus;
	enh["manaBonus"] = enhancement.manaBonus;
	enh["speedBonus"] = enhancement.speedBonus;
	enh["stealthBonus"] = enhancement.stealthBonus;
	enh["fireResistance"] = enhancement.fireResistance;
	enh["coldResistance"] = enhancement.coldResistance;
	enh["lightningResistance"] = enhancement.lightningResistance;
	enh["poisonResistance"] = enhancement.poisonResistance;
	enh["blessing"] = static_cast<int>(enhancement.blessing);
	enh["isMagical"] = enhancement.isMagical;
	enh["enhancementLevel"] = enhancement.enhancementLevel;
	enh["valueModifier"] = enhancement.valueModifier;
	j["enhancement"] = enh;

	// Save identification status
	json id;
	id["identifiedType"] = identification.identifiedType;
	id["identifiedEnhancement"] = identification.identifiedEnhancement;
	id["identifiedBuc"] = identification.identifiedBuc;
	j["identification"] = id;

	if (behavior)
	{
		json pickableJson;
		save_behavior(*behavior, pickableJson);
		j["pickable"] = pickableJson;
	}
}

const std::string& Item::get_true_name() const noexcept
{
	// Returns the complete, true name (for internal use)
	if (is_enhanced())
	{
		static std::string enhancedName;
		enhancedName = enhancement.get_full_name(actorData.name);
		return enhancedName;
	}
	return actorData.name;
}

const std::string& Item::get_name() const noexcept
{
	// Returns identified parts only; unknown parts marked with "?"
	static std::string displayName;

	if (!is_enhanced())
	{
		// No enhancement: just use base name, maybe with unknown marker
		return actorData.name;
	}

	displayName = "";

	// Build prefix
	if (!identification.identifiedEnhancement)
	{
		// Don't know enhancement level yet
		displayName = "? ";
	}
	else
	{
		// We know the enhancement
		if (enhancement.prefix != PrefixType::NONE)
		{
			displayName += enhancement.get_prefix_name() + " ";
		}
	}

	// Base name
	displayName += actorData.name;

	// Build suffix
	if (identification.identifiedEnhancement && enhancement.suffix != SuffixType::NONE)
	{
		displayName += " " + enhancement.get_suffix_name();
	}
	else if (!identification.identifiedEnhancement && enhancement.suffix != SuffixType::NONE)
	{
		displayName += " ?";
	}

	// Append BUC if identified
	if (identification.identifiedBuc)
	{
		switch (enhancement.blessing)
		{

		case BlessingStatus::BLESSED:
		{
			displayName += " (blessed)";
			break;
		}

		case BlessingStatus::CURSED:
		{
			displayName += " (cursed)";
			break;
		}

		case BlessingStatus::UNCURSED:
		{
			// Don't append uncursed to keep name clean
			break;
		}

		}
	}

	return displayName;
}

void Item::apply_enhancement(const ItemEnhancement& newEnhancement)
{
	enhancement = newEnhancement;
	// Name is computed dynamically by get_name() -- do not modify actorData.name
}

void Item::generate_random_enhancement(bool allowMagical)
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
		enhancement = ItemEnhancement::generate_random_enhancement(allowMagical);
	}
}

bool Item::is_enhanced() const noexcept
{
	return enhancement.prefix != PrefixType::NONE || enhancement.suffix != SuffixType::NONE;
}