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
	//==Actor Attributes==
	int strength{ 0 };
	int dexterity{ 0 };
	int constitution{ 0 };
	int intelligence{ 0 };
	int wisdom{ 0 };
	int charisma{ 0 };

	int playerLevel{ 1 };
	int gold{ 0 };
	std::string gender{ "None" };
	std::string weaponEquipped{ "None" };
	int invisibleTurnsRemaining{ 0 };
public:
	Creature(Vector2D position, ActorData data) : Actor(position, data), inventory_data(InventoryData(50))
	{
		add_state(ActorState::BLOCKS);
		/*add_state(ActorState::FOV_ONLY);*/
	};

	void load(const json& j) override;
	void save(json& j) override;

	void update(GameContext& ctx);

	// Const-correct getter methods
	int get_strength() const noexcept { return strength; }
	int get_dexterity() const noexcept { return dexterity; }
	int get_constitution() const noexcept { return constitution; }
	int get_intelligence() const noexcept { return intelligence; }
	int get_wisdom() const noexcept { return wisdom; }
	int get_charisma() const noexcept { return charisma; }
	int get_player_level() const noexcept { return playerLevel; }
	int get_gold() const noexcept { return gold; }
	const std::string& get_gender() const noexcept { return gender; }
	const std::string& get_weapon_equipped() const noexcept { return weaponEquipped; }

	// Setter methods
	void set_strength(int value) noexcept { strength = value; }
	void set_dexterity(int value) noexcept { dexterity = value; }
	void set_constitution(int value) noexcept { constitution = value; }
	void set_intelligence(int value) noexcept { intelligence = value; }
	void set_wisdom(int value) noexcept { wisdom = value; }
	void set_charisma(int value) noexcept { charisma = value; }
	void set_player_level(int value) noexcept { playerLevel = value; }
	void set_gold(int value) noexcept { gold = value; }
	void set_gender(const std::string& new_gender) noexcept { gender = new_gender; }
	void set_weapon_equipped(const std::string& weapon) noexcept { weaponEquipped = weapon; }
	
	// Modifier methods for increment/decrement operations
	void adjust_strength(int delta) noexcept { strength += delta; }
	void adjust_dexterity(int delta) noexcept { dexterity += delta; }
	void adjust_constitution(int delta) noexcept { constitution += delta; }
	void adjust_intelligence(int delta) noexcept { intelligence += delta; }
	void adjust_wisdom(int delta) noexcept { wisdom += delta; }
	void adjust_charisma(int delta) noexcept { charisma += delta; }
	void adjust_gold(int delta) noexcept { gold += delta; }
	void adjust_level(int delta) noexcept { playerLevel += delta; }

	void equip(Item& item, GameContext& ctx);
	void unequip(Item& item, GameContext& ctx);
	void sync_ranged_state(GameContext& ctx);
	void pick(GameContext& ctx);
	void drop(Item& item, GameContext& ctx);

	bool is_invisible() const noexcept { return has_state(ActorState::IS_INVISIBLE); }
	void set_invisible(int turns) noexcept
	{
		invisibleTurnsRemaining = turns;
		if (turns > 0)
		{
			add_state(ActorState::IS_INVISIBLE);
		}
	}
	void clear_invisible() noexcept
	{
		invisibleTurnsRemaining = 0;
		remove_state(ActorState::IS_INVISIBLE);
	}
	int get_invisible_turns() const noexcept { return invisibleTurnsRemaining; }
	void decrement_invisible() noexcept
	{

		if (invisibleTurnsRemaining > 0)
		{
			--invisibleTurnsRemaining;
			if (invisibleTurnsRemaining == 0) remove_state(ActorState::IS_INVISIBLE);
		}
	}

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

	// Initialize item type from name (temporary bridge until creation system is refactored)
	void initialize_item_type_from_name();

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
	ItemClass itemClass{ ItemClass::UNKNOWN }; // Proper item classification
	ItemEnhancement enhancement; // Enhancement data

	std::unique_ptr<Pickable> pickable; // the actor can be picked
	
	// Item classification utility functions
	ItemCategory get_category() const noexcept { return ItemClassificationUtils::get_category(itemClass); }
	bool is_weapon() const noexcept { return ItemClassificationUtils::is_weapon(itemClass); }
	bool is_armor() const noexcept { return ItemClassificationUtils::is_armor(itemClass); }
	bool is_shield() const noexcept { return ItemClassificationUtils::is_shield(itemClass); }
	bool is_consumable() const noexcept { return ItemClassificationUtils::is_consumable(itemClass); }
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
