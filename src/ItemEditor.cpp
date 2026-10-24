// file: ItemEditor.cpp
#include <algorithm>
#include <cassert>
#include <cctype>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "BuffType.h"
#include "ContentRegistry.h"
#include "ContentRegistryIO.h"
#include "DiceExpr.h"
#include "GameContext.h"
#include "ItemClassification.h"
#include "ItemEditor.h"
#include "ItemRegistry.h"
#include "MagicalItemEffects.h"
#include "Paths.h"
#include "TargetMode.h"
#include "Weapons.h"

static_assert(
	is_well_formed_table(ItemEditorModel::FIELDS),
	"ItemEditorModel::FIELDS is out of order or misplaces a plain number");

namespace
{

// The item registry the editor edits, which every caller requires.
//
// Example:
//   item_registry(ctx).get_all_keys().front();   // -> "amulet_of_health"
ItemRegistry& item_registry(const GameContext& ctx)
{
	assert(
		ctx.itemRegistry && "ItemEditorModel: called without an itemRegistry");
	return *ctx.itemRegistry;
}

// A key made readable: underscores become spaces and each word is capitalised.
//
// Example:
//   prettify_key("long_sword");   // -> "Long Sword"
std::string prettify_key(std::string_view key)
{
	std::string result;
	bool isWordStart = true;
	for (char character : key)
	{
		if (character == '_')
		{
			result += ' ';
			isWordStart = true;
		}
		else if (isWordStart)
		{
			result += static_cast<char>(
				std::toupper(static_cast<unsigned char>(character)));
			isWordStart = false;
		}
		else
		{
			result += character;
		}
	}
	return result;
}

std::string_view consumable_effect_str(ConsumableEffect effect)
{
	switch (effect)
	{
	case ConsumableEffect::NONE:
	{
		return "none";
	}
	case ConsumableEffect::HEAL:
	{
		return "heal";
	}
	case ConsumableEffect::ADD_BUFF:
	{
		return "add_buff";
	}
	case ConsumableEffect::FAIL:
	{
		return "fail";
	}
	}
	return "none";
}

} // namespace

// ---------------------------------------------------------------------------
// Records
// ---------------------------------------------------------------------------

std::vector<std::string> ItemEditorModel::keys(const GameContext& ctx) const
{
	const ItemRegistry& items = item_registry(ctx);
	std::vector<std::string> keys = items.get_all_keys();
	// Shipped items first and custom ones after, each still in key order.
	const auto is_shipped = [&items](const std::string& key)
	{
		return items.is_builtin_key(key);
	};
	std::ranges::stable_partition(keys, is_shipped);
	return keys;
}

void ItemEditorModel::load(const GameContext& ctx, std::string_view key)
{
	working = item_registry(ctx).get_params(key);
	workingTile =
		ctx.contentRegistry ? ctx.contentRegistry->get_tile(key) : TileRef{};
}

void ItemEditorModel::commit(GameContext& ctx, std::string_view key) const
{
	ItemRegistry& items = item_registry(ctx);
	items.set_name_category(key, working.name, working.category);
	items.set_params(key, working);
	if (ctx.contentRegistry)
	{
		ctx.contentRegistry->set_tile(key, workingTile);
	}
}

void ItemEditorModel::save(GameContext& ctx) const
{
	item_registry(ctx).save(Paths::ITEMS);
	if (ctx.contentRegistry)
	{
		ContentRegistryIO::save(*ctx.contentRegistry, Paths::CONTENT_TILES);
	}
}

std::string ItemEditorModel::add_record(GameContext& ctx) const
{
	ItemParams defaults;
	defaults.itemClass = ItemClass::SWORD;
	defaults.pickableType = PickableType::WEAPON;
	defaults.value = 10;
	defaults.baseWeight = 10;
	defaults.levelMin = 1;
	defaults.levelMax = 5;
	defaults.handRequirement = HandRequirement::ONE_HANDED;
	defaults.weaponSize = WeaponSize::MEDIUM;
	return item_registry(ctx).add_custom("New Item", "weapon", defaults);
}

void ItemEditorModel::remove_record(
	GameContext& ctx,
	std::string_view key) const
{
	// An item removed before any commit was never given a sprite.
	if (ctx.contentRegistry &&
		ctx.contentRegistry->all_tiles().contains(std::string{ key }))
	{
		ctx.contentRegistry->remove_tile(key);
	}
	item_registry(ctx).remove_custom(key);
}

bool ItemEditorModel::is_builtin(
	const GameContext& ctx,
	std::string_view key) const
{
	return item_registry(ctx).is_builtin_key(key);
}

std::string ItemEditorModel::row_label(
	const GameContext& ctx,
	std::string_view key) const
{
	const std::string& name = item_registry(ctx).get_params(key).name;
	return name.empty() ? prettify_key(key) : name;
}

std::string ItemEditorModel::working_row_label(std::string_view key) const
{
	return working.name.empty() ? prettify_key(key) : working.name;
}

TileRef ItemEditorModel::row_tile(
	const GameContext& ctx,
	std::string_view key) const
{
	return ctx.contentRegistry ? ctx.contentRegistry->get_tile(key) : TileRef{};
}

// ---------------------------------------------------------------------------
// Fields
// ---------------------------------------------------------------------------

std::string ItemEditorModel::field_label(FieldId field) const
{
	return std::string{ descriptor_of(FIELDS, field).label };
}

std::string ItemEditorModel::field_value(FieldId field) const
{
	const Descriptor& descriptor = descriptor_of(FIELDS, field);
	if (descriptor.plain.has_value())
	{
		return plain_text(*descriptor.plain, working);
	}

	const ItemParams& params = working;
	switch (field)
	{
	case FieldId::NAME:
	{
		return working.name;
	}
	case FieldId::APPEARANCE:
	{
		return working.appearance;
	}
	case FieldId::CATEGORY:
	{
		return working.category;
	}
	case FieldId::ITEM_CLASS:
	{
		return std::string{ encode_item_class(params.itemClass) };
	}
	case FieldId::PICKABLE_TYPE:
	{
		return std::string{ encode_pickable_type(params.pickableType) };
	}
	case FieldId::COLOR:
	{
		return std::string{ color_pair_name(params.color) };
	}
	case FieldId::LEVEL_SCALING:
	{
		return std::format("{:.2f}", params.levelScaling);
	}
	case FieldId::CONSUMABLE_EFFECT:
	{
		return std::string{ consumable_effect_str(params.consumableEffect) };
	}
	case FieldId::CONSUMABLE_BUFF:
	{
		return std::string{ encode_buff_type(params.consumableBuffType) };
	}
	case FieldId::CONSUMABLE_AMT_DICE:
	{
		return std::format(
			"{} ({})",
			params.consumableAmount.num,
			to_text(params.consumableAmount));
	}
	case FieldId::CONSUMABLE_AMT_SIDES:
	{
		return std::format("{}", params.consumableAmount.sides);
	}
	case FieldId::CONSUMABLE_AMT_BONUS:
	{
		return std::format("{}", params.consumableAmount.bonus);
	}
	case FieldId::DURATION_DICE:
	{
		return std::format(
			"{} ({})",
			params.duration.num,
			to_text(params.duration));
	}
	case FieldId::DURATION_SIDES:
	{
		return std::format("{}", params.duration.sides);
	}
	case FieldId::DURATION_BONUS:
	{
		return std::format("{}", params.duration.bonus);
	}
	case FieldId::DURATION_UNIT:
	{
		return std::string{ encode_duration_unit(params.durationUnit) };
	}
	case FieldId::TARGET_MODE:
	{
		return std::string{ encode_target_mode(params.targetMode) };
	}
	case FieldId::SCROLL_ANIM:
	{
		return std::string{ encode_scroll_animation(params.scrollAnimation) };
	}
	case FieldId::RANGED:
	{
		return params.ranged ? "yes" : "no";
	}
	case FieldId::HAND_REQUIREMENT:
	{
		return std::string{ encode_hand_requirement(params.handRequirement) };
	}
	case FieldId::WEAPON_SIZE:
	{
		return std::string{ encode_weapon_size(params.weaponSize) };
	}
	case FieldId::EFFECT:
	{
		return std::string{ encode_magical_effect(params.effect) };
	}
	case FieldId::APPLICATION:
	{
		return std::string{ encode_buff_application(params.application) };
	}
	case FieldId::TILE:
	{
		return "(tile)";
	}
	default:
	{
		refuse_misrouted_field(
			descriptor.label,
			"ItemEditorModel::field_value");
	}
	}
}

void ItemEditorModel::field_adjust(FieldId field, int delta)
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

	ItemParams& params = working;
	switch (field)
	{
	case FieldId::LEVEL_SCALING:
	{
		float raw = params.levelScaling + static_cast<float>(delta) * 0.01f;
		params.levelScaling = std::clamp(raw, 0.0f, 5.0f);
		break;
	}
	case FieldId::CONSUMABLE_AMT_DICE:
	{
		// No dice is a fixed amount; any dice need a die to roll, and a d2 is the least.
		params.consumableAmount.num =
			clamp_val(params.consumableAmount.num, delta, 0, 99);
		params.consumableAmount.sides = params.consumableAmount.num == 0 ?
			0 :
			std::max(params.consumableAmount.sides, 2);
		break;
	}
	case FieldId::CONSUMABLE_AMT_SIDES:
	{
		params.consumableAmount.sides = params.consumableAmount.num == 0 ?
			0 :
			clamp_val(params.consumableAmount.sides, delta, 2, 20);
		break;
	}
	case FieldId::CONSUMABLE_AMT_BONUS:
	{
		params.consumableAmount.bonus =
			clamp_val(params.consumableAmount.bonus, delta, 0, 9999);
		break;
	}
	case FieldId::DURATION_DICE:
	{
		// No dice is a fixed duration; any dice need a die to roll, and a d2 is the least.
		params.duration.num = clamp_val(params.duration.num, delta, 0, 99);
		params.duration.sides =
			params.duration.num == 0 ? 0 : std::max(params.duration.sides, 2);
		break;
	}
	case FieldId::DURATION_SIDES:
	{
		params.duration.sides = params.duration.num == 0 ?
			0 :
			clamp_val(params.duration.sides, delta, 2, 20);
		break;
	}
	case FieldId::DURATION_BONUS:
	{
		params.duration.bonus =
			clamp_val(params.duration.bonus, delta, 0, 9999);
		break;
	}
	default:
	{
		refuse_misrouted_field(
			descriptor.label,
			"ItemEditorModel::field_adjust");
	}
	}
}

void ItemEditorModel::field_toggle(FieldId field)
{
	ItemParams& params = working;
	switch (field)
	{
	case FieldId::COLOR:
	{
		params.color = next_color_pair(params.color);
		break;
	}
	case FieldId::ITEM_CLASS:
	{
		params.itemClass = next_item_class(params.itemClass);
		break;
	}
	case FieldId::PICKABLE_TYPE:
	{
		params.pickableType = next_pickable_type(params.pickableType);
		break;
	}
	case FieldId::CONSUMABLE_EFFECT:
	{
		params.consumableEffect =
			next_consumable_effect(params.consumableEffect);
		break;
	}
	case FieldId::CONSUMABLE_BUFF:
	{
		params.consumableBuffType = next_buff_type(params.consumableBuffType);
		break;
	}
	case FieldId::DURATION_UNIT:
	{
		params.durationUnit = next_duration_unit(params.durationUnit);
		break;
	}
	case FieldId::TARGET_MODE:
	{
		params.targetMode = next_target_mode(params.targetMode);
		break;
	}
	case FieldId::SCROLL_ANIM:
	{
		params.scrollAnimation = next_scroll_animation(params.scrollAnimation);
		break;
	}
	case FieldId::RANGED:
	{
		params.ranged = !params.ranged;
		break;
	}
	case FieldId::HAND_REQUIREMENT:
	{
		params.handRequirement = next_hand_requirement(params.handRequirement);
		break;
	}
	case FieldId::WEAPON_SIZE:
	{
		params.weaponSize = next_weapon_size(params.weaponSize);
		break;
	}
	case FieldId::EFFECT:
	{
		params.effect = next_magical_effect(params.effect);
		break;
	}
	case FieldId::APPLICATION:
	{
		params.application = params.application == BuffApplication::ADD ?
			BuffApplication::SET :
			BuffApplication::ADD;
		break;
	}
	default:
	{
		refuse_misrouted_field(
			descriptor_of(FIELDS, field).label,
			"ItemEditorModel::field_toggle");
	}
	}
}

void ItemEditorModel::field_set_string(FieldId field, std::string newValue)
{
	switch (field)
	{
	case FieldId::NAME:
	{
		working.name = std::move(newValue);
		break;
	}
	case FieldId::APPEARANCE:
	{
		working.appearance = std::move(newValue);
		break;
	}
	case FieldId::CATEGORY:
	{
		working.category = std::move(newValue);
		break;
	}
	default:
	{
		refuse_misrouted_field(
			descriptor_of(FIELDS, field).label,
			"ItemEditorModel::field_set_string");
	}
	}
}

// end of file: ItemEditor.cpp
