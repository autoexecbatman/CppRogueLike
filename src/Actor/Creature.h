#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Ai/Ai.h"
#include "../Colors/Colors.h"
#include "../Combat/ArmorClass.h"
#include "../Combat/ExperienceReward.h"
#include "../Core/GameContext.h"
#include "../Persistent/Persistent.h"
#include "../Renderer/Renderer.h"
#include "../Systems/BuffType.h"
#include "../Systems/ShopKeeper.h"
#include "../Utils/Vector2D.h"
#include "Actor.h"
#include "Attacker.h"
#include "Destructible.h"
#include "EquipmentSlot.h"
#include "InventoryData.h"
#include "Item.h"

enum class CreatureClass
{
	FIGHTER,
	ROGUE,
	CLERIC,
	WIZARD,
	MONSTER,
};

class Creature : public Actor
{
protected:
	// Shared per-turn logic (buffs, armor, constitution) used by both
	// Creature::update() and Player::update().
	void update_creature_state(GameContext& ctx);

private:
	//==Actor Attributes - Base values (buffs calculated dynamically)==
	int baseStrength{ 0 };
	int baseDexterity{ 0 };
	int baseConstitution{ 0 };
	int baseIntelligence{ 0 };
	int baseWisdom{ 0 };
	int baseCharisma{ 0 };

	// Creature level and gold
	int creatureLevel{ 1 };
	int gold{ 0 };

	// Creature gender and weapon
	std::string gender{ "None" };
	std::string weaponEquipped{ "None" };

	// Combat class and hit die (set by class selection or monster registry)
	CreatureClass creatureClass{ CreatureClass::MONSTER };
	int hitDie{ 8 };
	float attacksPerRound{ 1.0f };
	int morale{ 10 };
	int damageResistance{ 0 };
	int thaco{ 20 };

	TileRef invisibleTile{}; // lazily resolved from TileConfig on first update()

	// AD&D 2e: Calculate effective stat value (MAX(base, SET) + ADD)
	int calculate_effective_stat(int base_value, BuffType type) const noexcept;

public:
	// Unified buff system - modifier stack pattern (managed by BuffSystem)
	// Note: active_buffs vector is public for BuffSystem access
	std::vector<Buff> activeBuffs;
	Creature(Vector2D position, ActorData data)
		: Actor(position, data), inventoryData(InventoryData(50))
	{
		add_state(ActorState::BLOCKS);
		/*add_state(ActorState::FOV_ONLY);*/
	};

	void load(const json& j) override;
	void save(json& j) override;

	virtual void update(GameContext& ctx);

	// Const-correct getter methods - return effective values (AD&D 2e: MAX(base, SET) + ADD)
	int get_strength() const noexcept { return calculate_effective_stat(baseStrength, BuffType::STRENGTH); }
	int get_dexterity() const noexcept { return calculate_effective_stat(baseDexterity, BuffType::DEXTERITY); }
	int get_constitution() const noexcept { return calculate_effective_stat(baseConstitution, BuffType::CONSTITUTION); }
	int get_intelligence() const noexcept { return calculate_effective_stat(baseIntelligence, BuffType::INTELLIGENCE); }
	int get_wisdom() const noexcept { return calculate_effective_stat(baseWisdom, BuffType::WISDOM); }
	int get_charisma() const noexcept { return calculate_effective_stat(baseCharisma, BuffType::CHARISMA); }
	int get_creature_level() const noexcept { return creatureLevel; }

	// Virtual for polymorphism - monsters use HD, players override
	virtual int get_level() const noexcept { return creatureLevel; }

	// AD&D 2e: Virtual method for Constitution HP bonus multiplier cap
	// Monsters: no cap (return level), Players: class-specific caps
	virtual int get_constitution_hp_multiplier() const noexcept { return get_level(); }

	int get_gold() const noexcept { return gold; }
	const std::string& get_gender() const noexcept { return gender; }
	const std::string& get_weapon_equipped() const noexcept { return weaponEquipped; }

	// Setter methods - modify base stats
	void set_strength(int value) noexcept { baseStrength = value; }
	void set_dexterity(int value) noexcept { baseDexterity = value; }
	void set_constitution(int value) noexcept { baseConstitution = value; }
	void set_intelligence(int value) noexcept { baseIntelligence = value; }
	void set_wisdom(int value) noexcept { baseWisdom = value; }
	void set_charisma(int value) noexcept { baseCharisma = value; }
	void set_creature_level(int value) noexcept { creatureLevel = value; }
	void set_gold(int value) noexcept { gold = value; }
	void set_gender(const std::string& new_gender) noexcept { gender = new_gender; }
	void set_weapon_equipped(const std::string& weapon) noexcept { weaponEquipped = weapon; }

	// Modifier methods for increment/decrement operations - modify base stats
	void adjust_strength(int delta) noexcept { baseStrength += delta; }
	void adjust_dexterity(int delta) noexcept { baseDexterity += delta; }
	void adjust_constitution(int delta) noexcept { baseConstitution += delta; }
	void adjust_intelligence(int delta) noexcept { baseIntelligence += delta; }
	void adjust_wisdom(int delta) noexcept { baseWisdom += delta; }
	void adjust_charisma(int delta) noexcept { baseCharisma += delta; }
	void adjust_gold(int delta) noexcept { gold += delta; }
	void adjust_level(int delta) noexcept { creatureLevel += delta; }

	// Experience reward
	[[nodiscard]] int get_xp() const noexcept { return experienceReward->get_xp(); }
	void set_xp(int value) noexcept { experienceReward->set_xp(value); }
	void add_xp(int amount) noexcept { experienceReward->add_xp(amount); }

	CreatureClass get_creature_class() const noexcept { return creatureClass; }
	int get_hit_die() const noexcept { return hitDie; }
	float get_attacks_per_round() const noexcept { return attacksPerRound; }
	void set_creature_class(CreatureClass cc) noexcept { creatureClass = cc; }
	void set_hit_die(int hd) noexcept { hitDie = hd; }
	void set_attacks_per_round(float apr) noexcept { attacksPerRound = apr; }
	int get_morale() const noexcept { return morale; }
	void set_morale(int value) noexcept { morale = value; }

	int get_dr() const noexcept { return damageResistance; }
	int get_thaco() const noexcept { return thaco; }
	void set_dr(int value) noexcept { damageResistance = value; }
	void set_thaco(int value) noexcept { thaco = value; }

	virtual void apply_confusion(int nbTurns);

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

	// Type query - allows polymorphic identification without RTTI or cross-module deps
	virtual bool is_player() const noexcept { return false; }

	// Web effect handler - Player overrides this, monsters ignore webs by default
	virtual void apply_web_effect(int duration, int strength, class Web* web, GameContext& ctx) { /* do nothing */ }

	// Display interface — Player overrides with actual values; monsters return sentinels
	virtual std::string get_class_display_name() const { return {}; }
	virtual std::string get_race_display_name() const { return {}; }
	virtual int get_kill_count() const noexcept { return 0; }
	virtual std::string get_equipped_weapon_damage_roll() const noexcept { return "?"; }

	// AD&D 2e Open Locks skill — Rogue only. Returns percentage chance (0-95).
	// Non-Rogue classes return 0 (cannot pick locks).
	virtual int get_open_locks_skill() const noexcept { return 0; }

	// Armor Class accessors
	[[nodiscard]] int get_armor_class() const noexcept { return armorClass->get_armor_class(); }
	[[nodiscard]] int get_base_armor_class() const noexcept { return armorClass->get_base_armor_class(); }
	void set_armor_class(int value) noexcept { armorClass->set_armor_class(value); }
	void set_base_armor_class(int value) noexcept { armorClass->set_base_armor_class(value); }
	void update_armor_class(GameContext& ctx) { armorClass->update(*this, ctx); }

	// Lifecycle hooks — Player overrides; monsters no-op
	// Called once at game start (level 1, new game) to apply race/class setup
	virtual void on_new_game_start(GameContext& ctx) {}
	// Called every STARTUP to sync combat stats (THAC0 etc.)
	virtual void recalculate_combat_stats() {}
	// Called when a creature dies — Player saves/defeats, monsters drop corpses
	virtual void die(GameContext& ctx);
	// Called by Destructible when a monster dies — rewards the player who killed it
	virtual void on_kill_reward(int xp, GameContext& ctx) {}

	TileRef get_display_tile() const noexcept override;
	int get_display_color() const noexcept override;

	std::unique_ptr<Attacker> attacker; // the actor can attack
	std::unique_ptr<ExperienceReward> experienceReward; // the actor can earn experience
	std::unique_ptr<ArmorClass> armorClass; // the actor has armor class
	std::unique_ptr<Destructible> destructible; // the actor can be destroyed
	std::unique_ptr<Ai> ai; // the actor can have AI
	std::unique_ptr<ShopKeeper> shop; // shopkeeper component for trading
	InventoryData inventoryData;
};