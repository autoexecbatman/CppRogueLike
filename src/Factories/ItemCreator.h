#pragma once

#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "../Items/ItemClassification.h"
#include "../Items/MagicalItemEffects.h"
#include "../Items/Weapons.h"
#include "../Systems/BuffType.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"
#include "../Systems/TargetMode.h"

struct Vector2D;
struct GameContext;
class ContentRegistry;

// Unified compositional item parameters
struct ItemParams
{
	// Display & Classification
	std::string_view name{ "" };
	int color{ 0 };
	ItemClass itemClass{ ItemClass::UNKNOWN };
	int value{ 0 };
	PickableType pickable_type{ PickableType::WEAPON };

	int consumable_amount{ 0 };
	int range{ 0 };
	int damage{ 0 };
	int confuse_turns{ 0 };
	int duration{ 0 };

	MagicalEffect effect{ MagicalEffect::NONE };
	int effect_bonus{ 0 };

	int str_bonus{ 0 };
	int dex_bonus{ 0 };
	int con_bonus{ 0 };
	int int_bonus{ 0 };
	int wis_bonus{ 0 };
	int cha_bonus{ 0 };
	bool is_set_mode{ false };

	int nutrition_value{ 0 };
	int gold_amount{ 0 };
	int ac_bonus{ 0 };

	bool ranged{ false };
	HandRequirement hand_requirement{ HandRequirement::ONE_HANDED };
	WeaponSize weapon_size{ WeaponSize::MEDIUM };

	ConsumableEffect consumable_effect{ ConsumableEffect::NONE };
	BuffType consumable_buff_type{ BuffType::INVISIBILITY };

	TargetMode target_mode{ TargetMode::AUTO_NEAREST };
	ScrollAnimation scroll_animation{ ScrollAnimation::NONE };

	int base_weight{ 0 };
	int level_minimum{ 1 };
	int level_maximum{ 0 };
	float level_scaling{ 0.0f };
	std::string_view category{ "" };
};

enum class EnhancedItemCategory
{
	WEAPON,
	ARMOR
};

struct EnhancedItemSpawnRule
{
	std::vector<std::string> item_pool;
	EnhancedItemCategory enhancement_category;
	int base_weight;
	int level_minimum;
	int level_maximum;
	float level_scaling;
	std::string category;
};

class ItemCreator
{
public:
	// Load all items from JSON. Must be called before Game construction.
	static void load(std::string_view path);
	static void save(std::string_view path);

	// All keys in insertion order.
	[[nodiscard]] static std::vector<std::string> get_all_keys();

	// Throws std::out_of_range if key unknown.
	[[nodiscard]] static const ItemParams& get_params(std::string_view key);
	static void set_params(std::string_view key, const ItemParams& params);
	// Update the owned name and category strings for an existing item (editor use).
	static void set_name_category(std::string_view key, std::string name, std::string category);

	// Create item from string key.
	[[nodiscard]] static std::unique_ptr<Item> create(std::string_view key, Vector2D pos, ContentRegistry& registry);
	[[nodiscard]] static std::unique_ptr<Item> create_with_gold_amount(Vector2D pos, int goldAmount, ContentRegistry& registry);
	[[nodiscard]] static std::unique_ptr<Item> create_with_enhancement(std::string_view key, Vector2D pos, PrefixType prefix, SuffixType suffix, ContentRegistry& registry);
	[[nodiscard]] static std::unique_ptr<Item> create_gold_pile(Vector2D pos, GameContext& ctx);
	[[nodiscard]] static std::unique_ptr<Item> create_random_of_category(std::string_view category, Vector2D pos, GameContext& ctx, int dungeonLevel);

	// Add new item. Returns actual key used (derived from name).
	[[nodiscard]] static std::string add_custom(std::string name, std::string category, ItemParams params);
	static void remove_custom(std::string_view key);
	[[nodiscard]] static bool is_builtin_key(std::string_view key);

	// Enhanced item spawn rules -- loaded from separate JSON file.
	static void load_enhanced_rules(std::string_view path);
	[[nodiscard]] static std::span<const EnhancedItemSpawnRule> get_enhanced_rules();
};
