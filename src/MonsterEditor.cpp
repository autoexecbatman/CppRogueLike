// file: MonsterEditor.cpp
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Ai.h"
#include "Attitude.h"
#include "DiceExpr.h"
#include "GameContext.h"
#include "IntelligenceRating.h"
#include "MonsterEditor.h"
#include "MonsterRegistry.h"
#include "MonsterStrength.h"
#include "Paths.h"
#include "Spellcasting.h"

static_assert(
	is_well_formed_table(MonsterEditorModel::FIELDS),
	"MonsterEditorModel::FIELDS is out of order or misplaces a plain number");

namespace
{

// The monster registry the editor edits, which every caller requires.
//
// Example:
//   monster_registry(ctx).get_all_keys().front();   // -> "goblin"
MonsterRegistry& monster_registry(const GameContext& ctx)
{
	assert(
		ctx.monsterRegistry &&
		"MonsterEditorModel: called without a monsterRegistry");
	return *ctx.monsterRegistry;
}

// Alignment fields cycle through their axis rather than being typed, so each
// press moves one step and wraps at the end. The names come from Alignment.h,
// beside the enums, so the editor shows what a record carries.
Ethics next_ethics(Ethics ethics)
{
	switch (ethics)
	{
	case Ethics::LAWFUL:
	{
		return Ethics::NEUTRAL;
	}
	case Ethics::NEUTRAL:
	{
		return Ethics::CHAOTIC;
	}
	case Ethics::CHAOTIC:
	{
		return Ethics::LAWFUL;
	}
	}
	return Ethics::NEUTRAL;
}

Morality next_morality(Morality morality)
{
	switch (morality)
	{
	case Morality::GOOD:
	{
		return Morality::NEUTRAL;
	}
	case Morality::NEUTRAL:
	{
		return Morality::EVIL;
	}
	case Morality::EVIL:
	{
		return Morality::GOOD;
	}
	}
	return Morality::NEUTRAL;
}

std::string prettify_key(std::string_view key)
{
	std::string result;
	bool capNext = true;
	for (char character : key)
	{
		if (character == '_')
		{
			result += ' ';
			capNext = true;
		}
		else if (capNext)
		{
			result += static_cast<char>(
				std::toupper(static_cast<unsigned char>(character)));
			capNext = false;
		}
		else
		{
			result += character;
		}
	}
	return result;
}

} // namespace

// ---------------------------------------------------------------------------
// Records
// ---------------------------------------------------------------------------

std::vector<std::string> MonsterEditorModel::keys(const GameContext& ctx) const
{
	return monster_registry(ctx).get_all_keys();
}

void MonsterEditorModel::load(const GameContext& ctx, std::string_view key)
{
	working = monster_registry(ctx).get_params(key);
	selectedXpCount = 0;
	selectedPower = 0;
}

void MonsterEditorModel::commit(GameContext& ctx, std::string_view key) const
{
	monster_registry(ctx).set_params(key, working);
}

void MonsterEditorModel::save(GameContext& ctx) const
{
	monster_registry(ctx).save(Paths::MONSTERS);
}

std::string MonsterEditorModel::add_record(GameContext& ctx) const
{
	MonsterParams defaults;
	defaults.name = "New Monster";
	defaults.corpseName = "dead monster";
	defaults.corpseNutrition = 40;
	defaults.corpseTaste = "It tastes like nothing you have eaten before.";
	defaults.hitDice = MonsterHitDice{ 8, 0, { { 1, 1 } } };
	defaults.thaco = 20;
	defaults.ac = 10;
	defaults.strength = std::nullopt;
	defaults.naturalAttack = "claws";
	defaults.damage = DamageInfo{ DiceExpr{ 1, 4, 0 }, DamageType::PHYSICAL };
	defaults.baseWeight = 10;
	defaults.levelMinimum = 1;
	defaults.levelMaximum = 5;
	return monster_registry(ctx).add_custom(std::move(defaults));
}

void MonsterEditorModel::remove_record(
	GameContext& ctx,
	std::string_view key) const
{
	monster_registry(ctx).remove_custom(key);
}

bool MonsterEditorModel::is_builtin(
	const GameContext& ctx,
	std::string_view key) const
{
	return monster_registry(ctx).is_builtin(key);
}

std::string MonsterEditorModel::row_label(
	const GameContext& ctx,
	std::string_view key) const
{
	const std::string& name = monster_registry(ctx).get_params(key).name;
	return name.empty() ? prettify_key(key) : name;
}

std::string MonsterEditorModel::working_row_label(std::string_view key) const
{
	return working.name.empty() ? prettify_key(key) : working.name;
}

TileRef MonsterEditorModel::row_tile(
	const GameContext& ctx,
	std::string_view key) const
{
	return monster_registry(ctx).get_tile(key);
}

std::optional<RowTag> MonsterEditorModel::row_tag(
	[[maybe_unused]] const GameContext& ctx,
	[[maybe_unused]] std::string_view key) const
{
	return std::nullopt;
}

std::optional<RowTag> MonsterEditorModel::working_row_tag() const
{
	return std::nullopt;
}

// ---------------------------------------------------------------------------
// Fields
// ---------------------------------------------------------------------------

std::string MonsterEditorModel::field_label(FieldId field) const
{
	return std::string{ descriptor_of(FIELDS, field).label };
}

void MonsterEditorModel::set_damage_dice(int num, int sides, int bonus)
{
	working.damage =
		DamageInfo{ DiceExpr{ num, sides, bonus }, working.damage.damageType };
}

std::string MonsterEditorModel::field_value(FieldId field) const
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
	case FieldId::CORPSE:
	{
		return working.corpseName;
	}
	case FieldId::CORPSE_TASTE:
	{
		return working.corpseTaste;
	}
	case FieldId::HD_LOWEST:
	{
		return std::format("{}", working.hitDice.counts.front().count);
	}
	case FieldId::HD_HIGHEST:
	{
		return std::format("{}", highest_count(working.hitDice).count);
	}
	case FieldId::HD_SIDES:
	{
		return std::format("{}", working.hitDice.sides);
	}
	case FieldId::HD_BONUS:
	{
		return std::format("{}", working.hitDice.bonus);
	}
	case FieldId::XP_COUNT:
	{
		return std::format(
			"{} HD",
			working.hitDice.counts.at(selectedXpCount).count);
	}
	case FieldId::XP:
	{
		return std::format("{}", working.hitDice.counts.at(selectedXpCount).xp);
	}
	case FieldId::STR_SOURCE:
	{
		if (working.strength.has_value())
		{
			return "Printed in its entry";
		}
		return "Table 6 by size";
	}
	case FieldId::STR_SCORE:
	{
		return working.strength.has_value() ?
			std::format("{}", working.strength->score) :
			"-";
	}
	case FieldId::STR_EXCEPTIONAL:
	{
		// 0 is no percentile, 1 to 100 is the percentile with 100 as 18/00, and only an 18 has one.
		if (!working.strength.has_value())
		{
			return "-";
		}
		const MonsterStrength strength = *working.strength;
		if (strength.score != MONSTER_STRENGTH_EXCEPTIONAL_SCORE)
		{
			return "only on an 18";
		}
		if (strength.exceptional == MONSTER_STRENGTH_EXCEPTIONAL_NONE)
		{
			return "0 (none)";
		}
		if (strength.exceptional == MONSTER_STRENGTH_EXCEPTIONAL_MAXIMUM)
		{
			return std::format("{} (18/00)", strength.exceptional);
		}
		return std::format(
			"{} (18/{:02})",
			strength.exceptional,
			strength.exceptional);
	}
	case FieldId::WEAPON:
	{
		return working.naturalAttack;
	}
	case FieldId::DMG_NUM:
	{
		return std::format("{}", working.damage.dice.num);
	}
	case FieldId::DMG_SIDES:
	{
		return std::format("{}", working.damage.dice.sides);
	}
	case FieldId::DMG_BONUS:
	{
		return std::format("{}", working.damage.dice.bonus);
	}
	case FieldId::DMG_TYPE:
	{
		return std::string(damage_type_name(working.damage.damageType));
	}
	case FieldId::AI_TYPE:
	{
		return std::string(encode_ai_type(working.aiType));
	}
	case FieldId::ETHICS:
	{
		return std::string(encode_ethics(working.ethics));
	}
	case FieldId::MORALITY:
	{
		return std::string(encode_morality(working.morality));
	}
	case FieldId::SIZE:
	{
		return std::string(encode_creature_size(working.size));
	}
	case FieldId::INTELLIGENCE:
	{
		return std::string(encode_intelligence_rating(working.intelligence));
	}
	case FieldId::CAN_SWIM:
	{
		return working.canSwim ? "yes" : "no";
	}
	case FieldId::CAN_FLY:
	{
		return working.canFly ? "yes" : "no";
	}
	case FieldId::WALKS_WEBS:
	{
		return working.walksWebs ? "yes" : "no";
	}
	case FieldId::ATTITUDE:
	{
		return std::string(encode_attitude(working.attitude));
	}
	case FieldId::DISPLACEABLE:
	{
		return working.displaceable ? "yes" : "no";
	}
	case FieldId::CASTER_CLASS:
	{
		return working.spellcasterTraining.has_value() ?
			std::string(
				encode_caster_class(working.spellcasterTraining->casterClass)) :
			"none";
	}
	case FieldId::CASTER_LEVEL:
	{
		return working.spellcasterTraining.has_value() ?
			std::format("{}", working.spellcasterTraining->casterLevel) :
			"-";
	}
	case FieldId::INNATE_POWER:
	{
		if (working.innatePowers.empty())
		{
			return "none";
		}
		return std::format(
			"{} of {}",
			selectedPower + 1,
			working.innatePowers.size());
	}
	case FieldId::INNATE_SPELL:
	{
		return working.innatePowers.empty() ?
			"" :
			working.innatePowers.at(selectedPower).spell;
	}
	case FieldId::INNATE_FREQUENCY:
	{
		return working.innatePowers.empty() ?
			"-" :
			std::string(encode_power_frequency(
				working.innatePowers.at(selectedPower).frequency));
	}
	case FieldId::CORPSE_EDIBLE:
	{
		return is_corpse_edible(working.corpseEdibility) ? "yes" : "no";
	}
	case FieldId::TILE:
	{
		return "(tile)";
	}
	default:
	{
		refuse_misrouted_field(
			descriptor.label,
			"MonsterEditorModel::field_value");
	}
	}
}

void MonsterEditorModel::field_adjust(FieldId field, int delta)
{
	const Descriptor& descriptor = descriptor_of(FIELDS, field);
	if (descriptor.plain.has_value())
	{
		adjust_plain(*descriptor.plain, working, delta);
		return;
	}

	auto clamp_val = [](int value, int step, int low, int high) -> int
	{
		return std::clamp(value + step, low, high);
	};

	switch (field)
	{
	case FieldId::HD_LOWEST:
	{
		working.hitDice = with_lowest_count(
			working.hitDice,
			working.hitDice.counts.front().count + delta);
		// The range may have shrunk under the selected count.
		selectedXpCount =
			std::min(selectedXpCount, working.hitDice.counts.size() - 1);
		assert(
			is_valid_monster_hit_dice(working.hitDice) &&
			"MonsterEditor: the lowest count left hit dice the loader refuses");
		break;
	}
	case FieldId::HD_HIGHEST:
	{
		working.hitDice = with_highest_count(
			working.hitDice,
			highest_count(working.hitDice).count + delta);
		selectedXpCount =
			std::min(selectedXpCount, working.hitDice.counts.size() - 1);
		assert(
			is_valid_monster_hit_dice(working.hitDice) &&
			"MonsterEditor: the highest count left hit dice the loader refuses");
		break;
	}
	case FieldId::HD_SIDES:
	{
		working.hitDice.sides = clamp_val(working.hitDice.sides, delta, 1, 20);
		break;
	}
	case FieldId::HD_BONUS:
	{
		working.hitDice.bonus =
			clamp_val(working.hitDice.bonus, delta, -99, 99);
		break;
	}
	case FieldId::XP_COUNT:
	{
		// One count a step, whatever the step size, and never past either end.
		if (delta < 0 && selectedXpCount > 0)
		{
			--selectedXpCount;
		}
		else if (
			delta > 0 && selectedXpCount + 1 < working.hitDice.counts.size())
		{
			++selectedXpCount;
		}
		break;
	}
	case FieldId::XP:
	{
		HitDiceCount& selected = working.hitDice.counts.at(selectedXpCount);
		selected.xp = clamp_val(selected.xp, delta, 0, 99999);
		break;
	}
	case FieldId::STR_SCORE:
	{
		// A Table 6 monster has no score to adjust.
		if (!working.strength.has_value())
		{
			break;
		}
		MonsterStrength& strength = *working.strength;
		strength.score = clamp_val(
			strength.score,
			delta,
			MONSTER_STRENGTH_SCORE_MINIMUM,
			MONSTER_STRENGTH_SCORE_MAXIMUM);
		// A percentile rides only on an 18, so leaving 18 drops it rather than writing what the loader refuses.
		if (strength.score != MONSTER_STRENGTH_EXCEPTIONAL_SCORE)
		{
			strength.exceptional = MONSTER_STRENGTH_EXCEPTIONAL_NONE;
		}
		assert(
			is_valid_monster_strength(strength) &&
			"MonsterEditor: the Strength score left a Strength the loader refuses");
		break;
	}
	case FieldId::STR_EXCEPTIONAL:
	{
		if (!working.strength.has_value() ||
			working.strength->score != MONSTER_STRENGTH_EXCEPTIONAL_SCORE)
		{
			break;
		}
		MonsterStrength& strength = *working.strength;
		strength.exceptional = clamp_val(
			strength.exceptional,
			delta,
			MONSTER_STRENGTH_EXCEPTIONAL_NONE,
			MONSTER_STRENGTH_EXCEPTIONAL_MAXIMUM);
		assert(
			is_valid_monster_strength(strength) &&
			"MonsterEditor: the percentile left a Strength the loader refuses");
		break;
	}
	// The dice are the record: every change rebuilds the damage, so its text and
	// its range follow rather than being edited into disagreement.
	case FieldId::DMG_NUM:
	{
		set_damage_dice(
			clamp_val(working.damage.dice.num, delta, 1, 99),
			working.damage.dice.sides,
			working.damage.dice.bonus);
		break;
	}
	case FieldId::DMG_SIDES:
	{
		set_damage_dice(
			working.damage.dice.num,
			clamp_val(working.damage.dice.sides, delta, 2, 20),
			working.damage.dice.bonus);
		break;
	}
	case FieldId::DMG_BONUS:
	{
		set_damage_dice(
			working.damage.dice.num,
			working.damage.dice.sides,
			clamp_val(working.damage.dice.bonus, delta, -99, 99));
		break;
	}
	case FieldId::CASTER_LEVEL:
	{
		// A monster that memorizes nothing has no level to adjust; the tables print 1st to 20th.
		if (working.spellcasterTraining.has_value())
		{
			working.spellcasterTraining->casterLevel = clamp_val(
				working.spellcasterTraining->casterLevel,
				delta,
				1,
				20);
		}
		break;
	}
	case FieldId::INNATE_POWER:
	{
		// One power a step, as the XP field's count is chosen, and never past either end.
		if (delta < 0 && selectedPower > 0)
		{
			--selectedPower;
		}
		else if (delta > 0 && selectedPower + 1 < working.innatePowers.size())
		{
			++selectedPower;
		}
		break;
	}
	default:
	{
		refuse_misrouted_field(
			descriptor.label,
			"MonsterEditorModel::field_adjust");
	}
	}
}

void MonsterEditorModel::field_toggle(FieldId field)
{
	if (field == FieldId::AI_TYPE)
	{
		working.aiType = next_authorable_ai_type(working.aiType);
	}
	else if (field == FieldId::CAN_SWIM)
	{
		working.canSwim = !working.canSwim;
	}
	else if (field == FieldId::CAN_FLY)
	{
		working.canFly = !working.canFly;
	}
	else if (field == FieldId::WALKS_WEBS)
	{
		working.walksWebs = !working.walksWebs;
	}
	else if (field == FieldId::ATTITUDE)
	{
		working.attitude = next_attitude(working.attitude);
	}
	else if (field == FieldId::DISPLACEABLE)
	{
		working.displaceable = !working.displaceable;
	}
	else if (field == FieldId::CORPSE_EDIBLE)
	{
		working.corpseEdibility = is_corpse_edible(working.corpseEdibility) ?
			CorpseEdibility::INEDIBLE :
			CorpseEdibility::EDIBLE;
	}
	else if (field == FieldId::ETHICS)
	{
		working.ethics = next_ethics(working.ethics);
	}
	else if (field == FieldId::MORALITY)
	{
		working.morality = next_morality(working.morality);
	}
	else if (field == FieldId::SIZE)
	{
		working.size = next_creature_size(working.size);
	}
	else if (field == FieldId::INTELLIGENCE)
	{
		working.intelligence = next_intelligence_rating(working.intelligence);
	}
	else if (field == FieldId::STR_SOURCE)
	{
		// A printed Strength starts at 10, the middle of 3d6, for the author to set to the entry's figure.
		constexpr MonsterStrength startingPrintedStrength{
			10,
			MONSTER_STRENGTH_EXCEPTIONAL_NONE,
		};
		if (working.strength.has_value())
		{
			working.strength.reset();
		}
		else
		{
			working.strength = startingPrintedStrength;
		}
	}
	else if (field == FieldId::DMG_TYPE)
	{
		// The dice are unchanged; only what they deal is.
		working.damage = DamageInfo{
			working.damage.dice,
			next_damage_type(working.damage.damageType),
		};
	}
	else if (field == FieldId::CASTER_CLASS)
	{
		// None, then a 1st-level wizard, then a priest at the same level, then none again.
		if (!working.spellcasterTraining.has_value())
		{
			working.spellcasterTraining =
				SpellcasterTraining{ CasterClass::WIZARD, 1 };
		}
		else if (
			working.spellcasterTraining->casterClass == CasterClass::WIZARD)
		{
			working.spellcasterTraining->casterClass = CasterClass::CLERIC;
		}
		else
		{
			working.spellcasterTraining.reset();
		}
	}
	else if (field == FieldId::INNATE_FREQUENCY)
	{
		// With no power listed there is no frequency to step.
		if (!working.innatePowers.empty())
		{
			InnatePower& power = working.innatePowers.at(selectedPower);
			if (power.frequency == PowerFrequency::AT_WILL)
			{
				power.frequency = PowerFrequency::ONCE_PER_DAY;
			}
			else
			{
				power.frequency = PowerFrequency::AT_WILL;
			}
		}
	}
	else
	{
		refuse_misrouted_field(
			descriptor_of(FIELDS, field).label,
			"MonsterEditorModel::field_toggle");
	}
}

void MonsterEditorModel::field_set_string(FieldId field, std::string newValue)
{
	switch (field)
	{
	case FieldId::NAME:
	{
		working.name = std::move(newValue);
		break;
	}
	case FieldId::CORPSE:
	{
		working.corpseName = std::move(newValue);
		break;
	}
	case FieldId::CORPSE_TASTE:
	{
		working.corpseTaste = std::move(newValue);
		break;
	}
	case FieldId::WEAPON:
	{
		working.naturalAttack = std::move(newValue);
		break;
	}
	case FieldId::INNATE_SPELL:
	{
		if (working.innatePowers.empty())
		{
			if (!newValue.empty())
			{
				working.innatePowers.push_back(
					InnatePower{
						std::move(newValue),
						PowerFrequency::AT_WILL,
						0,
					});
				selectedPower = 0;
			}
		}
		else if (newValue.empty())
		{
			working.innatePowers.erase(
				working.innatePowers.begin() +
				static_cast<std::ptrdiff_t>(selectedPower));
			selectedPower = selectedPower > 0 ? selectedPower - 1 : 0;
		}
		else
		{
			working.innatePowers.at(selectedPower).spell = std::move(newValue);
		}
		break;
	}
	default:
	{
		refuse_misrouted_field(
			descriptor_of(FIELDS, field).label,
			"MonsterEditorModel::field_set_string");
	}
	}
}

// end of file: MonsterEditor.cpp
