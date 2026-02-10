#pragma once

#include <string>
#include <ranges>
#include <vector>
#include <memory>

#include "../Persistent/Persistent.h"
#include "../Utils/UniqueId.h"
#include "../Items/ItemClassification.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"
#include "../Colors/Colors.h"
#include "../Utils/Vector2D.h"
#include "InventoryData.h"
#include "../Systems/ShopKeeper.h"
#include "../Ai/Ai.h"
#include "../Actor/Destructible.h"
#include "../Actor/Attacker.h"
#include "../Actor/Pickable.h"

struct GameContext;
enum class EquipmentSlot;


struct ActorData
{
	char ch{ 'f' };
	std::string name{ "string" };
	int color{ WHITE_BLACK_PAIR };
};

enum class ActorState
{
	BLOCKS,
	FOV_ONLY,
	CAN_SWIM,
	IS_EQUIPPED,
	IS_RANGED,
	IS_CONFUSED,
	IS_INVISIBLE,
	IS_LEVITATING,  // For flying creatures like bats
};

//==Actor==
// a class for the actors in the game
// (player, monsters, items, etc.)
class Actor : public Persistent
{
public:
	Vector2D position{ 0,0 };
	Vector2D direction{ 0,0 };
	ActorData actorData{ 0,"string",0 };
	UniqueId::IdType uniqueId{};

	std::vector<ActorState> states;
	// C++ Core Guidelines F.6: noexcept for simple state operations
	bool has_state(ActorState state) const noexcept { return std::ranges::find(states, state) != states.end(); }
	void add_state(ActorState state) noexcept { states.push_back(state); }
	void remove_state(ActorState state) noexcept { std::erase_if(states, [state](ActorState s) { return s == state; }); }

	Actor(Vector2D position, ActorData data);
	virtual ~Actor() = default;

	void load(const json& j) override;
	void save(json& j) override;

	int get_tile_distance(Vector2D tilePosition) const noexcept;
	void render(const GameContext& ctx) const noexcept;
	bool is_visible(const GameContext& ctx) const noexcept;

	[[nodiscard]] std::string_view get_name() const
	{
		return actorData.name;
	}
};

class Creature : public Actor
{
private:
	//==Actor Attributes - Base values (buffs calculated dynamically)==
	int base_strength{ 0 };
	int base_dexterity{ 0 };
	int base_constitution{ 0 };
	int base_intelligence{ 0 };
	int base_wisdom{ 0 };
	int base_charisma{ 0 };

	int playerLevel{ 1 };
	int gold{ 0 };
	std::string gender{ "None" };
	std::string weaponEquipped{ "None" };

	// AD&D 2e: Calculate effective stat value (MAX(base, SET) + ADD)
	int calculate_effective_stat(int base_value, BuffType type) const noexcept;

public:
	// Unified buff system - modifier stack pattern (managed by BuffSystem)
	// Note: active_buffs vector is public for BuffSystem access
	std::vector<Buff> active_buffs;
	Creature(Vector2D position, ActorData data) : Actor(position, data), inventory_data(InventoryData(50))
	{
		add_state(ActorState::BLOCKS);
		/*add_state(ActorState::FOV_ONLY);*/
	};

	void load(const json& j) override;
	void save(json& j) override;

	void update(GameContext& ctx);

	// Const-correct getter methods - return effective values (AD&D 2e: MAX(base, SET) + ADD)
	int get_strength() const noexcept { return calculate_effective_stat(base_strength, BuffType::STRENGTH); }
	int get_dexterity() const noexcept { return calculate_effective_stat(base_dexterity, BuffType::DEXTERITY); }
	int get_constitution() const noexcept { return calculate_effective_stat(base_constitution, BuffType::CONSTITUTION); }
	int get_intelligence() const noexcept { return calculate_effective_stat(base_intelligence, BuffType::INTELLIGENCE); }
	int get_wisdom() const noexcept { return calculate_effective_stat(base_wisdom, BuffType::WISDOM); }
	int get_charisma() const noexcept { return calculate_effective_stat(base_charisma, BuffType::CHARISMA); }
	int get_player_level() const noexcept { return playerLevel; }

	// Virtual for polymorphism - monsters use HD, players override
	virtual int get_level() const noexcept { return playerLevel; }

	// AD&D 2e: Virtual method for Constitution HP bonus multiplier cap
	// Monsters: no cap (return level), Players: class-specific caps
	virtual int get_constitution_hp_multiplier() const noexcept { return get_level(); }

	int get_gold() const noexcept { return gold; }
	const std::string& get_gender() const noexcept { return gender; }
	const std::string& get_weapon_equipped() const noexcept { return weaponEquipped; }

	// Setter methods - modify base stats
	void set_strength(int value) noexcept { base_strength = value; }
	void set_dexterity(int value) noexcept { base_dexterity = value; }
	void set_constitution(int value) noexcept { base_constitution = value; }
	void set_intelligence(int value) noexcept { base_intelligence = value; }
	void set_wisdom(int value) noexcept { base_wisdom = value; }
	void set_charisma(int value) noexcept { base_charisma = value; }
	void set_player_level(int value) noexcept { playerLevel = value; }
	void set_gold(int value) noexcept { gold = value; }
	void set_gender(const std::string& new_gender) noexcept { gender = new_gender; }
	void set_weapon_equipped(const std::string& weapon) noexcept { weaponEquipped = weapon; }

	// Modifier methods for increment/decrement operations - modify base stats
	void adjust_strength(int delta) noexcept { base_strength += delta; }
	void adjust_dexterity(int delta) noexcept { base_dexterity += delta; }
	void adjust_constitution(int delta) noexcept { base_constitution += delta; }
	void adjust_intelligence(int delta) noexcept { base_intelligence += delta; }
	void adjust_wisdom(int delta) noexcept { base_wisdom += delta; }
	void adjust_charisma(int delta) noexcept { base_charisma += delta; }
	void adjust_gold(int delta) noexcept { gold += delta; }
	void adjust_level(int delta) noexcept { playerLevel += delta; }

	void apply_confusion(int nbTurns);

	void equip(Item& item, GameContext& ctx);
	void unequip(Item& item, GameContext& ctx);
	void sync_ranged_state(GameContext& ctx);
	void pick(GameContext& ctx);
	void drop(Item& item, GameContext& ctx);

	bool is_invisible() const noexcept { return has_state(ActorState::IS_INVISIBLE); }

	// Virtual equipment interface - LSP compliant polymorphic equipment operations
	// Default implementations for NPCs (do nothing/return false)
	// Player overrides these with actual slot-based equipment system
	virtual bool toggle_equipment(uint64_t item_id, EquipmentSlot slot, GameContext& ctx) { return false; }
	virtual bool toggle_weapon(uint64_t item_id, EquipmentSlot slot, GameContext& ctx) { return false; }
	virtual bool toggle_shield(uint64_t item_id, GameContext& ctx) { return false; }
	virtual bool is_item_equipped(uint64_t item_id) const noexcept { return false; }
	virtual bool is_slot_occupied(EquipmentSlot slot) const noexcept { return false; }
	virtual Item* get_equipped_item(EquipmentSlot slot) const noexcept { return nullptr; }

	std::unique_ptr<Attacker> attacker; // the actor can attack
	std::unique_ptr<Destructible> destructible; // the actor can be destroyed
	std::unique_ptr<Ai> ai; // the actor can have AI
	std::unique_ptr<ShopKeeper> shop; // shopkeeper component for trading
	InventoryData inventory_data;
};

class NPC : public Creature
{
	public:
	NPC(Vector2D position, ActorData data) : Creature(position, data) {};
};

class Object : public Actor
{
public:
	Object(Vector2D position, ActorData data) : Actor(position, data) {};
};

class Item : public Object
{
public:
	Item(Vector2D position, ActorData data);

	void load(const json& j) override;
	void save(json& j) override;

	// Name accessor - returns enhanced name if item has enhancements
	const std::string& get_name() const noexcept;
	const std::string& get_base_name() const noexcept { return actorData.name; }

	// Enhancement system
	void apply_enhancement(const ItemEnhancement& enhancement);
	void generate_random_enhancement(bool allow_magical = true);
	const ItemEnhancement& get_enhancement() const noexcept { return enhancement; }
	bool is_enhanced() const noexcept;

	// Enhanced value calculation
	int get_base_value() const noexcept { return base_value; }
	int get_enhanced_value() const noexcept;
	void set_value(int v) noexcept { base_value = v; value = v; }

	int value{ 1 };
	int base_value{ 1 };
	ItemId itemId{ ItemId::UNKNOWN }; // Specific item identity
	ItemClass itemClass{ ItemClass::UNKNOWN }; // Item category classification
	ItemEnhancement enhancement; // Enhancement data

	std::unique_ptr<Pickable> pickable; // the actor can be picked
	
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

class Stairs : public Object
{
	public:
	Stairs(Vector2D position) : Object(position, ActorData{ '>', "stairs", WHITE_BLACK_PAIR }) 
	{
		/*add_state(ActorState::FOV_ONLY);*/
	};
};
