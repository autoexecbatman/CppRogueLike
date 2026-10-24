// file: SpellEditor.cpp
#include <cassert>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "GameContext.h"
#include "Paths.h"
#include "SpellEditor.h"
#include "SpellRegistry.h"

static_assert(
	is_well_formed_table(SpellEditorModel::FIELDS),
	"SpellEditorModel::FIELDS is out of order or misplaces a plain number");

namespace
{

// The spell registry the editor edits, which every caller requires.
//
// Example:
//   spell_registry(ctx).get_all_keys().front();   // -> "cure_light_wounds"
SpellRegistry& spell_registry(const GameContext& ctx)
{
	assert(
		ctx.spellRegistry &&
		"SpellEditorModel: called without a spellRegistry");
	return *ctx.spellRegistry;
}

// The mark a spell's list row carries: "C" in blue for a cleric spell, "W" in
// orange for a wizard spell or one both classes cast.
//
// Example:
//   class_tag(SpellClass::CLERIC).text;   // -> "C"
RowTag class_tag(SpellClass spellClass)
{
	if (spellClass == SpellClass::CLERIC)
	{
		return RowTag{ "C", Color{ 100, 220, 255, 255 } };
	}
	return RowTag{ "W", Color{ 255, 160, 80, 255 } };
}

std::string effect_type_name(SpellEffectType effectType)
{
	switch (effectType)
	{
	case SpellEffectType::CURE_LIGHT_WOUNDS:
	{
		return "Cure Light Wounds";
	}
	case SpellEffectType::BLESS:
	{
		return "Bless";
	}
	case SpellEffectType::SANCTUARY:
	{
		return "Sanctuary";
	}
	case SpellEffectType::HOLD_PERSON:
	{
		return "Hold Person";
	}
	case SpellEffectType::SILENCE:
	{
		return "Silence";
	}
	case SpellEffectType::MAGIC_MISSILE:
	{
		return "Magic Missile";
	}
	case SpellEffectType::SHIELD:
	{
		return "Shield";
	}
	case SpellEffectType::SLEEP:
	{
		return "Sleep";
	}
	case SpellEffectType::INVISIBILITY:
	{
		return "Invisibility";
	}
	case SpellEffectType::WEB:
	{
		return "Web";
	}
	case SpellEffectType::FIREBALL:
	{
		return "Fireball";
	}
	case SpellEffectType::TELEPORT:
	{
		return "Teleport";
	}
	default:
	{
		return "None";
	}
	}
}

SpellEffectType effect_type_next(SpellEffectType effectType)
{
	int value = static_cast<int>(effectType) + 1;
	constexpr int LAST = static_cast<int>(SpellEffectType::NONE);
	if (value > LAST)
	{
		value = 0;
	}
	return static_cast<SpellEffectType>(value);
}

SpellEffectType effect_type_prev(SpellEffectType effectType)
{
	int value = static_cast<int>(effectType) - 1;
	constexpr int LAST = static_cast<int>(SpellEffectType::NONE);
	if (value < 0)
	{
		value = LAST;
	}
	return static_cast<SpellEffectType>(value);
}

} // namespace

// ---------------------------------------------------------------------------
// Records
// ---------------------------------------------------------------------------

std::vector<std::string> SpellEditorModel::keys(const GameContext& ctx) const
{
	return spell_registry(ctx).get_all_keys();
}

void SpellEditorModel::load(const GameContext& ctx, std::string_view key)
{
	working = spell_registry(ctx).get_by_key(key);
}

void SpellEditorModel::commit(GameContext& ctx, std::string_view key) const
{
	spell_registry(ctx).set_by_key(key, working);
}

void SpellEditorModel::save(GameContext& ctx) const
{
	spell_registry(ctx).save(Paths::SPELLS);
}

std::string SpellEditorModel::add_record(GameContext& ctx) const
{
	SpellDefinition definition;
	definition.name = "New Spell";
	definition.level = 1;
	definition.spellClass = SpellClass::WIZARD;
	definition.description = "No effect yet.";
	return spell_registry(ctx).add_custom(std::move(definition));
}

void SpellEditorModel::remove_record(
	GameContext& ctx,
	std::string_view key) const
{
	spell_registry(ctx).remove_custom(key);
}

bool SpellEditorModel::is_builtin(
	const GameContext& ctx,
	std::string_view key) const
{
	return spell_registry(ctx).is_builtin_key(key);
}

std::string SpellEditorModel::row_label(
	const GameContext& ctx,
	std::string_view key) const
{
	const SpellDefinition& definition = spell_registry(ctx).get_by_key(key);
	return std::format("[{}] {}", definition.level, definition.name);
}

std::string SpellEditorModel::working_row_label(
	[[maybe_unused]] std::string_view key) const
{
	return std::format("[{}] {}", working.level, working.name);
}

std::optional<RowTag> SpellEditorModel::row_tag(
	const GameContext& ctx,
	std::string_view key) const
{
	return class_tag(spell_registry(ctx).get_by_key(key).spellClass);
}

std::optional<RowTag> SpellEditorModel::working_row_tag() const
{
	return class_tag(working.spellClass);
}

// ---------------------------------------------------------------------------
// Fields
// ---------------------------------------------------------------------------

std::string SpellEditorModel::field_label(FieldId field) const
{
	return std::string{ descriptor_of(FIELDS, field).label };
}

std::string SpellEditorModel::field_value(FieldId field) const
{
	const Descriptor& descriptor = descriptor_of(FIELDS, field);
	if (descriptor.plain.has_value())
	{
		return plain_text(*descriptor.plain, working);
	}

	switch (field)
	{
	case FieldId::NAME:
	{
		return working.name;
	}
	case FieldId::CLASS:
	{
		if (working.spellClass == SpellClass::CLERIC)
		{
			return "cleric";
		}
		if (working.spellClass == SpellClass::WIZARD)
		{
			return "wizard";
		}
		return "both";
	}
	case FieldId::EFFECT_TYPE:
	{
		return effect_type_name(working.effect_type);
	}
	case FieldId::DESCRIPTION:
	{
		return working.description;
	}
	default:
	{
		refuse_misrouted_field(
			descriptor.label,
			"SpellEditorModel::field_value");
	}
	}
}

void SpellEditorModel::field_adjust(FieldId field, int delta)
{
	const Descriptor& descriptor = descriptor_of(FIELDS, field);
	if (descriptor.plain.has_value())
	{
		adjust_plain(*descriptor.plain, working, delta);
		return;
	}

	if (field == FieldId::EFFECT_TYPE)
	{
		if (delta > 0)
		{
			working.effect_type = effect_type_next(working.effect_type);
		}
		else
		{
			working.effect_type = effect_type_prev(working.effect_type);
		}
	}
	else
	{
		refuse_misrouted_field(
			descriptor.label,
			"SpellEditorModel::field_adjust");
	}
}

void SpellEditorModel::field_toggle(FieldId field)
{
	if (field == FieldId::CLASS)
	{
		if (working.spellClass == SpellClass::CLERIC)
		{
			working.spellClass = SpellClass::WIZARD;
		}
		else if (working.spellClass == SpellClass::WIZARD)
		{
			working.spellClass = SpellClass::BOTH;
		}
		else
		{
			working.spellClass = SpellClass::CLERIC;
		}
	}
	else if (field == FieldId::EFFECT_TYPE)
	{
		working.effect_type = effect_type_next(working.effect_type);
	}
	else
	{
		refuse_misrouted_field(
			descriptor_of(FIELDS, field).label,
			"SpellEditorModel::field_toggle");
	}
}

void SpellEditorModel::field_set_string(FieldId field, std::string newValue)
{
	if (field == FieldId::NAME)
	{
		working.name = std::move(newValue);
	}
	else if (field == FieldId::DESCRIPTION)
	{
		working.description = std::move(newValue);
	}
	else
	{
		refuse_misrouted_field(
			descriptor_of(FIELDS, field).label,
			"SpellEditorModel::field_set_string");
	}
}

// end of file: SpellEditor.cpp
