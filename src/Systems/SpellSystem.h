#pragma once

#include <vector>
#include <string>
#include <unordered_map>

// Forward declarations
class Player;
class Creature;
struct GameContext;

// Mirror of Player::PlayerClassState to avoid circular dependency
enum class CasterClass { CLERIC, WIZARD, NONE };

// Spell identifiers
enum class SpellId
{
    // Cleric Level 1
    CURE_LIGHT_WOUNDS,
    BLESS,
    SANCTUARY,

    // Cleric Level 2
    HOLD_PERSON,
    SILENCE,

    // Wizard Level 1
    MAGIC_MISSILE,
    SHIELD,
    SLEEP,

    // Wizard Level 2
    INVISIBILITY,
    WEB,

    // Shared/Special
    NONE
};

enum class SpellClass { CLERIC, WIZARD, BOTH };

struct SpellDefinition
{
    SpellId id;
    std::string name;
    int level;
    SpellClass spellClass;
    std::string description;
};

class SpellSystem
{
public:
    // Get spell slots for class/level (AD&D 2e tables)
    static std::vector<int> get_spell_slots(CasterClass classState, int level);

    // Get all spells available to a class at given level
    static std::vector<SpellId> get_available_spells(CasterClass classState, int maxSpellLevel);

    // Cast a spell
    static bool cast_spell(SpellId spell, Creature& caster, GameContext& ctx);

    // Get spell info
    static const SpellDefinition& get_spell_definition(SpellId id);
    static int get_spell_level(SpellId id);
    static const std::string& get_spell_name(SpellId id);

    // Memorization
    static void show_memorization_menu(Player& player, GameContext& ctx);
    static void show_casting_menu(Player& player, GameContext& ctx);

private:
    static const std::unordered_map<SpellId, SpellDefinition>& get_spell_table();

    // Spell effects
    static bool cast_cure_light_wounds(Creature& caster, GameContext& ctx);
    static bool cast_bless(Creature& caster, GameContext& ctx);
    static bool cast_magic_missile(Creature& caster, GameContext& ctx);
    static bool cast_shield(Creature& caster, GameContext& ctx);
    static bool cast_sleep(Creature& caster, GameContext& ctx);
    static bool cast_invisibility(Creature& caster, GameContext& ctx);
};
