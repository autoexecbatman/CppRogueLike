#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Ai/Ai.h"
#include "../Combat/ArmorClass.h"
#include "../Combat/ConstitutionTracker.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/ExperienceReward.h"
#include "../Combat/HealthPool.h"
#include "../Core/GameContext.h"
#include "Alignment.h"
#include "CreatureClass.h"
#include "../Persistent/Persistent.h"
#include "../Renderer/Renderer.h"
#include "../Systems/BuffType.h"
#include "../Systems/ShopKeeper.h"
#include "../Utils/Vector2D.h"
#include "Actor.h"
#include "Attacker.h"
#include "EquipmentSlot.h"
#include "InventoryData.h"
#include "Item.h"

class Web;

// How a creature feels about the player. Ordered from most hostile to most
// friendly, so threshold comparisons are meaningful.
//
// This is deliberately separate from whether the creature has noticed the
// player: a hostile creature that has not seen you is still hostile.
enum class Attitude
{
	HOSTILE, // attacks the player on sight
	PEACEFUL, // will not attack unless provoked
	FRIENDLY, // will not attack, and sides with the player
	TAME, // a companion under the player's direction
};

// True when attacking this creature is a deliberate act the player should be
// asked to confirm.
//
// Example:
//   needs_attack_confirmation(Attitude::HOSTILE);  // -> false
//   needs_attack_confirmation(Attitude::PEACEFUL); // -> true
[[nodiscard]] constexpr bool needs_attack_confirmation(Attitude attitude)
{
	return attitude >= Attitude::PEACEFUL;
}

// Turns a creature keeps tracking the player after losing sight of them.
inline constexpr int AWARENESS_TURNS = 3;

// What a struggle against a web achieved this turn.
enum class WebEscape
{
	BROKE_FREE, // the creature tore loose on its own
	STRUGGLED_FREE, // the binding ran out and released it
	STILL_STUCK, // still held; the turn is spent
};

class Creature : public Actor
{
private:
	Attitude attitude{ Attitude::HOSTILE }; // how this creature feels about the player
	Ethics ethics{ Ethics::NEUTRAL }; // law/chaos axis; true neutral until data says otherwise
	Morality morality{ Morality::NEUTRAL }; // good/evil axis
	int awarenessTurns{ 0 }; // turns of memory left after last seeing the player
	bool undead{ false }; // whether a priest may attempt to turn this creature
	bool displaceable{ true }; // whether the player may swap places with it
	int webStuckTurns{ 0 }; // turns remaining before the web lets go
	int webStrength{ 0 }; // strength of the web holding this creature
	Web* trappingWeb{ nullptr }; // the web holding it, destroyed on escape

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
	int corpseWeight{ 50 };

	TileRef invisibleTile{}; // lazily resolved from TileConfig on first update()

	// AD&D 2e: Calculate effective stat value (MAX(base, SET) + ADD)
	int calculate_effective_stat(int base_value, BuffType type) const noexcept;

public:
	// Unified buff system - modifier stack pattern (managed by BuffSystem)
	// Note: active_buffs vector is public for BuffSystem access
	std::vector<Buff> activeBuffs;
	Creature(Vector2D position, ActorData data)
		: Actor(position, data), constitutionTracker(std::make_unique<ConstitutionTracker>()), inventoryData(CreatureInventory(50))
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
	int get_corpse_weight() const noexcept { return corpseWeight; }
	void set_corpse_weight(int value) noexcept { corpseWeight = value; }

	virtual void apply_confusion(int nbTurns);

	void equip(Item& item, GameContext& ctx);
	void unequip(Item& item, GameContext& ctx);
	void drop(Item& item, GameContext& ctx);

	bool is_invisible() const noexcept { return has_state(ActorState::IS_INVISIBLE); }

	// Equipment query. Creatures other than the player have no slots, so the
	// honest answer for them is nothing. Read by ArmorClass, Web and targeting,
	// each of which runs for any creature.
	virtual Item* get_equipped_item(EquipmentSlot slot) const noexcept { return nullptr; }

	// Type query - allows polymorphic identification without RTTI or cross-module deps
	virtual bool is_player() const noexcept { return false; }

	// Binds this creature into a web for duration turns. Pure state: the web
	// decides what, if anything, the player is told.
	void apply_web_effect(int duration, int strength, class Web* web);

	// Whether this creature currently knows where the player is. Separate from
	// attitude: a hostile creature that has not seen you is still hostile.
	// Whether this creature is undead, which is what turning acts on.
	[[nodiscard]] bool is_undead() const noexcept { return undead; }
	void set_undead(bool value) noexcept { undead = value; }

	// Hit dice, the row a turning attempt reads from Table 61. Derived from
	// the creature's level, which is what the hit dice roll was based on.
	//
	// Example:
	//   skeleton.get_hit_dice(); // -> 1
	[[nodiscard]] int get_hit_dice() const noexcept { return creatureLevel; }

	[[nodiscard]] bool is_aware() const noexcept { return awarenessTurns > 0; }

	// Refreshes awareness for this turn. Seeing the player resets the memory to
	// full; losing sight lets it decay one turn at a time, so a creature keeps
	// hunting briefly rather than forgetting the moment it loses line of sight.
	// An invisible player is never seen.
	//
	// Example:
	//   creature.update_awareness(ctx); // player in view -> is_aware() == true
	//   creature.update_awareness(ctx); // out of view    -> still true, decaying
	void update_awareness(const GameContext& ctx);

	[[nodiscard]] Ethics get_ethics() const noexcept { return ethics; }
	void set_ethics(Ethics value) noexcept { ethics = value; }

	[[nodiscard]] Morality get_morality() const noexcept { return morality; }
	void set_morality(Morality value) noexcept { morality = value; }

	// True when this creature is evil, which is what alignment-keyed effects
	// such as Protection from Evil test.
	//
	// Example:
	//   goblin.set_morality(Morality::EVIL);
	//   goblin.is_evil(); // -> true
	[[nodiscard]] bool is_evil() const noexcept { return morality == Morality::EVIL; }

	[[nodiscard]] Attitude get_attitude() const noexcept { return attitude; }
	void set_attitude(Attitude value) noexcept { attitude = value; }

	// Whether the player swaps places with this creature instead of being
	// blocked. Unique NPCs that must not be pushed around set this false.
	[[nodiscard]] bool is_displaceable() const noexcept { return displaceable; }
	void set_displaceable(bool value) noexcept { displaceable = value; }

	[[nodiscard]] bool is_webbed() const noexcept { return webStuckTurns > 0; }

	// Rolls once against the web's strength and ages the binding by a turn.
	// Frees the creature and destroys the web on either escape.
	WebEscape try_break_web(GameContext& ctx);

private:
	void release_from_web();

public:

	// Armor Class accessors
	[[nodiscard]] int get_armor_class() const noexcept { return armorClass->get_armor_class(); }
	[[nodiscard]] int get_base_armor_class() const noexcept { return armorClass->get_base_armor_class(); }
	void set_armor_class(int value) noexcept { armorClass->set_armor_class(value); }
	void set_base_armor_class(int value) noexcept { armorClass->set_base_armor_class(value); }
	void update_armor_class(GameContext& ctx);

	// Health Pool accessors
	[[nodiscard]] bool is_dead() const noexcept { return healthPool->is_dead(); }
	[[nodiscard]] int get_hp() const noexcept { return healthPool->get_hp(); }
	[[nodiscard]] int get_max_hp() const noexcept { return healthPool->get_max_hp(); }
	[[nodiscard]] int get_hp_base() const noexcept { return healthPool->get_hp_base(); }
	[[nodiscard]] int get_temp_hp() const noexcept { return healthPool->get_temp_hp(); }
	[[nodiscard]] int get_effective_hp() const noexcept { return healthPool->get_effective_hp(); }
	void set_hp(int value) noexcept { healthPool->set_hp(value); }
	void set_max_hp(int value) noexcept { healthPool->set_max_hp(value); }
	void set_hp_base(int value) noexcept { healthPool->set_hp_base(value); }
	void set_temp_hp(int value) noexcept { healthPool->set_temp_hp(value); }
	void add_temp_hp(int amount) noexcept { healthPool->add_temp_hp(amount); }
	int heal(int hpToHeal) { return healthPool->heal(hpToHeal); }
	int take_damage(int damage, GameContext& ctx, DamageType damageType = DamageType::PHYSICAL);
	void take_damage_and_check_death(int damage, GameContext& ctx, DamageType damageType = DamageType::PHYSICAL);

	// Constitution tracking accessors
	[[nodiscard]] int get_last_constitution() const noexcept { return constitutionTracker->get_last_constitution(); }
	void set_last_constitution(int value) noexcept { constitutionTracker->set_last_constitution(value); }
	void update_constitution_bonus(GameContext& ctx);

	// Lifecycle hooks — Player overrides; monsters no-op
	// Called when a creature dies — Player saves/defeats, monsters drop corpses
	virtual void die(GameContext& ctx);

	TileRef get_display_tile() const noexcept override;
	int get_display_color() const noexcept override;

	std::unique_ptr<Attacker> attacker; // the actor can attack
	std::unique_ptr<ExperienceReward> experienceReward; // the actor can earn experience
	std::unique_ptr<ArmorClass> armorClass; // the actor has armor class
	std::unique_ptr<HealthPool> healthPool; // the actor has health
	std::unique_ptr<ConstitutionTracker> constitutionTracker; // tracks constitution changes for HP adjustments
	std::unique_ptr<Ai> ai; // the actor can have AI
	std::unique_ptr<ShopKeeper> shop; // shopkeeper component for trading
	CreatureInventory inventoryData;
};