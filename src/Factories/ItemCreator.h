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
	PickableType pickableType{ PickableType::WEAPON };

	int consumableAmount{ 0 };
	int range{ 0 };
	int damage{ 0 };
	int confuseTurns{ 0 };
	int duration{ 0 };

	MagicalEffect effect{ MagicalEffect::NONE };
	int effectBonus{ 0 };

	int strBonus{ 0 };
	int dexBonus{ 0 };
	int conBonus{ 0 };
	int intBonus{ 0 };
	int wisBonus{ 0 };
	int chaBonus{ 0 };
	bool isSetMode{ false };

	int nutritionValue{ 0 };
	int goldAmount{ 0 };
	int acBonus{ 0 };

	bool ranged{ false };
	HandRequirement handRequirement{ HandRequirement::ONE_HANDED };
	WeaponSize weaponSize{ WeaponSize::MEDIUM };

	ConsumableEffect consumableEffect{ ConsumableEffect::NONE };
	BuffType consumableBuffType{ BuffType::INVISIBILITY };

	TargetMode targetMode{ TargetMode::AUTO_NEAREST };
	ScrollAnimation scrollAnimation{ ScrollAnimation::NONE };

	int baseWeight{ 0 };
	int levelMin{ 1 };
	int levelMax{ 0 };
	float levelScaling{ 0.0f };
	std::string_view category{ "" };
};

enum class EnhancedItemCategory
{
	WEAPON,
	ARMOR
};

struct EnhancedItemSpawnRule
{
	std::vector<std::string> itemPool;
	EnhancedItemCategory enhancementCategory{};
	int baseWeight{ 0 };
	int levelMin{ 0 };
	int levelMax{ 0 };
	float levelScaling{ 0.0f };
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
	[[nodiscard]] static std::unique_ptr<Item> create(std::string_view key, Vector2D pos, ContentRegistry& tiles);
	[[nodiscard]] static std::unique_ptr<Item> create_with_gold_amount(Vector2D pos, int goldAmount, ContentRegistry& tiles);
	[[nodiscard]] static std::unique_ptr<Item> create_with_enhancement(std::string_view key, Vector2D pos, PrefixType prefix, SuffixType suffix, ContentRegistry& tiles);
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
