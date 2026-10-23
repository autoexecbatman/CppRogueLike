#include <optional>

#include "Item.h"
#include "SavedEnum.h"

namespace
{
// The ItemClass a saved integer names, or nullopt for one no class has.
// The switch has no default, so a class added without a case here is a compile
// error (C4062, -Wswitch).
//
// Example:
//   item_class_from(1);     // -> ItemClass::DAGGER
//   item_class_from(25);    // -> nullopt
std::optional<ItemClass> item_class_from(int saved)
{
	const ItemClass candidate = static_cast<ItemClass>(saved);
	switch (candidate)
	{
	case ItemClass::UNKNOWN:
	case ItemClass::DAGGER:
	case ItemClass::SWORD:
	case ItemClass::GREAT_SWORD:
	case ItemClass::AXE:
	case ItemClass::HAMMER:
	case ItemClass::MACE:
	case ItemClass::STAFF:
	case ItemClass::BOW:
	case ItemClass::CROSSBOW:
	case ItemClass::SLING:
	case ItemClass::ARMOR:
	case ItemClass::SHIELD:
	case ItemClass::HELMET:
	case ItemClass::RING:
	case ItemClass::AMULET:
	case ItemClass::GAUNTLETS:
	case ItemClass::GIRDLE:
	case ItemClass::POTION:
	case ItemClass::SCROLL:
	case ItemClass::FOOD:
	case ItemClass::GOLD_COIN:
	case ItemClass::GEM:
	case ItemClass::TOOL:
	case ItemClass::QUEST_ITEM:
	{
		return candidate;
	}
	}
	return std::nullopt;
}

// The PrefixType a saved integer names, or nullopt for one no prefix has.
// The switch has no default, so a prefix added without a case here is a compile
// error (C4062, -Wswitch).
//
// Example:
//   prefix_type_from(1);     // -> PrefixType::SHARP
//   prefix_type_from(17);    // -> nullopt
std::optional<PrefixType> prefix_type_from(int saved)
{
	const PrefixType candidate = static_cast<PrefixType>(saved);
	switch (candidate)
	{
	case PrefixType::NONE:
	case PrefixType::SHARP:
	case PrefixType::KEEN:
	case PrefixType::MASTERWORK:
	case PrefixType::BLESSED:
	case PrefixType::FLAMING:
	case PrefixType::FROST:
	case PrefixType::SHOCK:
	case PrefixType::REINFORCED:
	case PrefixType::STUDDED:
	case PrefixType::ELVEN:
	case PrefixType::DWARVEN:
	case PrefixType::MAGICAL:
	case PrefixType::CURSED:
	case PrefixType::ANCIENT:
	case PrefixType::RUSTED:
	case PrefixType::CRACKED:
	{
		return candidate;
	}
	}
	return std::nullopt;
}

// The SuffixType a saved integer names, or nullopt for one no suffix has.
// The switch has no default, so a suffix added without a case here is a compile
// error (C4062, -Wswitch).
//
// Example:
//   suffix_type_from(1);     // -> SuffixType::OF_SLAYING
//   suffix_type_from(19);    // -> nullopt
std::optional<SuffixType> suffix_type_from(int saved)
{
	const SuffixType candidate = static_cast<SuffixType>(saved);
	switch (candidate)
	{
	case SuffixType::NONE:
	case SuffixType::OF_SLAYING:
	case SuffixType::OF_ACCURACY:
	case SuffixType::OF_PROTECTION:
	case SuffixType::OF_POWER:
	case SuffixType::OF_FIRE_RESISTANCE:
	case SuffixType::OF_COLD_RESISTANCE:
	case SuffixType::OF_LIGHTNING_RESISTANCE:
	case SuffixType::OF_POISON_RESISTANCE:
	case SuffixType::OF_SPEED:
	case SuffixType::OF_STEALTH:
	case SuffixType::OF_MAGIC:
	case SuffixType::OF_HEALTH:
	case SuffixType::OF_THE_EAGLE:
	case SuffixType::OF_THE_BEAR:
	case SuffixType::OF_THE_OWL:
	case SuffixType::OF_WEAKNESS:
	case SuffixType::OF_SLOWNESS:
	case SuffixType::OF_BRITTLENESS:
	{
		return candidate;
	}
	}
	return std::nullopt;
}

// The BlessingStatus a saved integer names, or nullopt for one no status has.
// The switch has no default, so a status added without a case here is a
// compile error (C4062, -Wswitch).
//
// Example:
//   blessing_status_from(1);    // -> BlessingStatus::BLESSED
//   blessing_status_from(3);    // -> nullopt
std::optional<BlessingStatus> blessing_status_from(int saved)
{
	const BlessingStatus candidate = static_cast<BlessingStatus>(saved);
	switch (candidate)
	{
	case BlessingStatus::UNCURSED:
	case BlessingStatus::BLESSED:
	case BlessingStatus::CURSED:
	{
		return candidate;
	}
	}
	return std::nullopt;
}
} // namespace

// An item built here looks like what it is; ItemCreator gives one made from
// the data its record's appearance.
Item::Item(Vector2D position, ActorData data) :
	Actor(position, data),
	appearance(data.name)
{
}

void Item::load(const json& source)
{
	Actor::load(source); // Call base class load
	baseValue = source.at("baseValue").get<int>();
	itemKey = source.at("itemKey").get<std::string>();
	appearance = source.at("appearance").get<std::string>();
	itemClass = saved_enum_field(
		source,
		"Item::load",
		"itemClass",
		"ItemClass",
		&item_class_from);

	// Load enhancement data
	{
		const auto& enh = source.at("enhancement");
		enhancement.prefix = saved_enum_field(
			enh,
			"Item::load",
			"prefix",
			"PrefixType",
			&prefix_type_from);
		enhancement.suffix = saved_enum_field(
			enh,
			"Item::load",
			"suffix",
			"SuffixType",
			&suffix_type_from);
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
		enhancement.lightningResistance =
			enh.at("lightningResistance").get<int>();
		enhancement.poisonResistance = enh.at("poisonResistance").get<int>();
		enhancement.blessing = saved_enum_field(
			enh,
			"Item::load",
			"blessing",
			"BlessingStatus",
			&blessing_status_from);
		enhancement.isMagical = enh.at("isMagical").get<bool>();
		enhancement.enhancementLevel = enh.at("enhancementLevel").get<int>();
		enhancement.valueModifier = enh.at("valueModifier").get<int>();
	}

	// Load identification status
	{
		const auto& id = source.at("identification");
		identification.identifiedType = id.at("identifiedType").get<bool>();
		identification.identifiedEnhancement =
			id.at("identifiedEnhancement").get<bool>();
		identification.identifiedBuc = id.at("identifiedBuc").get<bool>();
	}

	// Saved only when the item has a behaviour, so its absence is a real state.
	if (source.contains("pickable"))
	{
		behavior = load_behavior(source["pickable"]);
	}
}

void Item::save(json& output)
{
	Actor::save(output); // Call base class save
	output["baseValue"] = baseValue;
	output["itemKey"] = itemKey;
	output["appearance"] = appearance;
	output["itemClass"] = static_cast<int>(itemClass);

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
	output["enhancement"] = enh;

	// Save identification status
	json id;
	id["identifiedType"] = identification.identifiedType;
	id["identifiedEnhancement"] = identification.identifiedEnhancement;
	id["identifiedBuc"] = identification.identifiedBuc;
	output["identification"] = id;

	if (behavior)
	{
		json pickableJson;
		save_behavior(*behavior, pickableJson);
		output["pickable"] = pickableJson;
	}
}

// The name the player sees: its appearance until its type is identified and
// its true name after, identified enhancement parts by name, unknown ones as
// "?", and a known blessing or curse appended. Built per call and returned by
// value, so two names held at once are two strings.
//
// Example:
//   helm.get_name();    // -> "helm", a helm of brilliance not yet identified
//   helm.identify_all();
//   helm.get_name();    // -> "helm of brilliance"
//   // a long sword with the SHARP prefix -> "? long sword", then once its
//   // enhancement is identified "Sharp long sword", cursed "(cursed)" added
std::string Item::get_name() const
{
	const std::string& baseName =
		is_type_identified() ? actorData.name : appearance;
	if (!is_enhanced())
	{
		return baseName;
	}

	std::string displayName{};

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

	displayName += baseName;

	// Build suffix
	if (identification.identifiedEnhancement &&
		enhancement.suffix != SuffixType::NONE)
	{
		displayName += " " + enhancement.get_suffix_name();
	}
	else if (
		!identification.identifiedEnhancement &&
		enhancement.suffix != SuffixType::NONE)
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

void Item::generate_random_enhancement(
	MagicalPrefixes magicalPrefixes,
	RandomDice& dice)
{
	// Each kind of item draws from its own pool, and every pool holds magical
	// prefixes, so the choice goes to whichever generator runs.
	if (is_weapon())
	{
		enhancement =
			ItemEnhancement::generate_weapon_enhancement(magicalPrefixes, dice);
	}
	else if (is_armor())
	{
		enhancement =
			ItemEnhancement::generate_armor_enhancement(magicalPrefixes, dice);
	}
	else
	{
		enhancement =
			ItemEnhancement::generate_random_enhancement(magicalPrefixes, dice);
	}
}

bool Item::is_enhanced() const noexcept
{
	return enhancement.prefix != PrefixType::NONE ||
		enhancement.suffix != SuffixType::NONE;
}