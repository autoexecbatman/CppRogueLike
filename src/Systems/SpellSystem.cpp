#include <curses.h>
#include <format>
#include <cmath>
#include <algorithm>
#include <map>

#include "SpellSystem.h"
#include "../Core/GameContext.h"
#include "../Colors/Colors.h"
#include "../ActorTypes/Player.h"
#include "../Utils/Vector2D.h"
#include "MessageSystem.h"
#include "CreatureManager.h"
#include "../Map/Map.h"

// Helper to convert PlayerClassState to CasterClass
static CasterClass to_caster_class(Player::PlayerClassState state)
{
    switch (state)
    {
    case Player::PlayerClassState::CLERIC: return CasterClass::CLERIC;
    case Player::PlayerClassState::WIZARD: return CasterClass::WIZARD;
    default: return CasterClass::NONE;
    }
}

const std::map<SpellId, SpellDefinition>& SpellSystem::get_spell_table()
{
    static const std::map<SpellId, SpellDefinition> spells = {
        // Cleric Level 1
        {SpellId::CURE_LIGHT_WOUNDS, {SpellId::CURE_LIGHT_WOUNDS, "Cure Light Wounds", 1, SpellClass::CLERIC, "Heals 1d8 HP"}},
        {SpellId::BLESS, {SpellId::BLESS, "Bless", 1, SpellClass::CLERIC, "+1 to hit for 6 turns"}},
        {SpellId::SANCTUARY, {SpellId::SANCTUARY, "Sanctuary", 1, SpellClass::CLERIC, "Enemies ignore you for 3 turns"}},

        // Cleric Level 2
        {SpellId::HOLD_PERSON, {SpellId::HOLD_PERSON, "Hold Person", 2, SpellClass::CLERIC, "Paralyze target for 4 turns"}},
        {SpellId::SILENCE, {SpellId::SILENCE, "Silence", 2, SpellClass::CLERIC, "Prevent target from casting"}},

        // Wizard Level 1
        {SpellId::MAGIC_MISSILE, {SpellId::MAGIC_MISSILE, "Magic Missile", 1, SpellClass::WIZARD, "1d4+1 force damage, auto-hit"}},
        {SpellId::SHIELD, {SpellId::SHIELD, "Shield", 1, SpellClass::WIZARD, "+4 AC for 5 turns"}},
        {SpellId::SLEEP, {SpellId::SLEEP, "Sleep", 1, SpellClass::WIZARD, "Put weak enemies to sleep"}},

        // Wizard Level 2
        {SpellId::INVISIBILITY, {SpellId::INVISIBILITY, "Invisibility", 2, SpellClass::WIZARD, "Become invisible for 20 turns"}},
        {SpellId::WEB, {SpellId::WEB, "Web", 2, SpellClass::WIZARD, "Create webs to trap enemies"}},

        {SpellId::NONE, {SpellId::NONE, "None", 0, SpellClass::BOTH, ""}}
    };
    return spells;
}

const SpellDefinition& SpellSystem::get_spell_definition(SpellId id)
{
    const auto& table = get_spell_table();
    auto it = table.find(id);
    if (it != table.end())
    {
        return it->second;
    }
    return table.at(SpellId::NONE);
}

int SpellSystem::get_spell_level(SpellId id)
{
    return get_spell_definition(id).level;
}

const std::string& SpellSystem::get_spell_name(SpellId id)
{
    return get_spell_definition(id).name;
}

std::vector<int> SpellSystem::get_spell_slots(CasterClass classState, int level)
{
    // AD&D 2e spell progression tables
    // Returns slots per spell level [level1, level2, level3, ...]

    if (classState == CasterClass::CLERIC)
    {
        // Cleric spell progression
        static const std::vector<std::vector<int>> clericSlots = {
            {1},           // Level 1
            {2},           // Level 2
            {2, 1},        // Level 3
            {3, 2},        // Level 4
            {3, 3, 1},     // Level 5
            {3, 3, 2},     // Level 6
            {3, 3, 2, 1},  // Level 7
            {3, 3, 3, 2},  // Level 8
            {4, 4, 3, 2, 1}, // Level 9
            {4, 4, 3, 3, 2}, // Level 10
        };
        int idx = std::min(level, 10) - 1;
        return idx >= 0 ? clericSlots[idx] : std::vector<int>{};
    }
    else if (classState == CasterClass::WIZARD)
    {
        // Wizard spell progression
        static const std::vector<std::vector<int>> wizardSlots = {
            {1},           // Level 1
            {2},           // Level 2
            {2, 1},        // Level 3
            {3, 2},        // Level 4
            {4, 2, 1},     // Level 5
            {4, 2, 2},     // Level 6
            {4, 3, 2, 1},  // Level 7
            {4, 3, 3, 2},  // Level 8
            {4, 3, 3, 2, 1}, // Level 9
            {4, 4, 3, 2, 2}, // Level 10
        };
        int idx = std::min(level, 10) - 1;
        return idx >= 0 ? wizardSlots[idx] : std::vector<int>{};
    }

    return {};
}

std::vector<SpellId> SpellSystem::get_available_spells(CasterClass classState, int maxSpellLevel)
{
    std::vector<SpellId> available;
    SpellClass targetClass = (classState == CasterClass::CLERIC)
        ? SpellClass::CLERIC
        : SpellClass::WIZARD;

    for (const auto& [id, def] : get_spell_table())
    {
        if (id == SpellId::NONE) continue;
        if ((def.spellClass == targetClass || def.spellClass == SpellClass::BOTH)
            && def.level <= maxSpellLevel)
        {
            available.push_back(id);
        }
    }
    return available;  // std::map iteration is already ordered by SpellId
}

bool SpellSystem::cast_spell(SpellId spell, Creature& caster, GameContext& ctx)
{
    switch (spell)
    {
    case SpellId::CURE_LIGHT_WOUNDS:
        return cast_cure_light_wounds(caster, ctx);
    case SpellId::BLESS:
        return cast_bless(caster, ctx);
    case SpellId::MAGIC_MISSILE:
        return cast_magic_missile(caster, ctx);
    case SpellId::SHIELD:
        return cast_shield(caster, ctx);
    case SpellId::SLEEP:
        return cast_sleep(caster, ctx);
    case SpellId::INVISIBILITY:
        return cast_invisibility(caster, ctx);
    default:
        ctx.message_system->message(WHITE_BLACK_PAIR, "Spell not implemented yet.", true);
        return false;
    }
}

static void animate_heal(const Vector2D& pos)
{
    const char* frames[] = {"+", "*", "+"};
    for (int i = 0; i < 3; ++i)
    {
        attron(COLOR_PAIR(GREEN_BLACK_PAIR));
        mvaddch(pos.y, pos.x, frames[i][0]);
        attroff(COLOR_PAIR(GREEN_BLACK_PAIR));
        refresh();
        napms(100);
    }
}

bool SpellSystem::cast_cure_light_wounds(Creature& caster, GameContext& ctx)
{
    if (!caster.destructible) return false;

    animate_heal(caster.position);

    int healing = ctx.dice->roll(1, 8);
    int oldHp = caster.destructible->get_hp();
    int maxHp = caster.destructible->get_max_hp();
    int newHp = std::min(oldHp + healing, maxHp);
    int actualHealing = newHp - oldHp;

    caster.destructible->set_hp(newHp);

    ctx.message_system->append_message_part(CYAN_BLACK_PAIR, "Cure Light Wounds! ");
    ctx.message_system->append_message_part(GREEN_BLACK_PAIR, std::format("+{} HP", actualHealing));
    ctx.message_system->finalize_message();

    return true;
}

bool SpellSystem::cast_bless(Creature& caster, GameContext& ctx)
{
    // TODO: Add buff system for temporary bonuses
    ctx.message_system->append_message_part(CYAN_BLACK_PAIR, "Bless! ");
    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "+1 to hit for 6 turns.");
    ctx.message_system->finalize_message();
    return true;
}

static void animate_magic_missile(const Vector2D& from, const Vector2D& to, int missileNum)
{
    // Different colors for each missile
    static const int colors[] = {MAGENTA_BLACK_PAIR, CYAN_BLACK_PAIR, BLUE_BLACK_PAIR, GREEN_BLACK_PAIR, WHITE_BLACK_PAIR};
    int color = colors[missileNum % 5];

    // Bresenham's line for projectile path
    int dx = std::abs(to.x - from.x);
    int dy = std::abs(to.y - from.y);
    int sx = (from.x < to.x) ? 1 : -1;
    int sy = (from.y < to.y) ? 1 : -1;
    int err = dx - dy;

    int x = from.x;
    int y = from.y;

    while (x != to.x || y != to.y)
    {
        attron(COLOR_PAIR(color));
        mvaddch(y, x, '*');
        attroff(COLOR_PAIR(color));
        refresh();
        napms(25);

        mvaddch(y, x, ' ');

        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y += sy;
        }
    }

    // Impact flash
    attron(COLOR_PAIR(YELLOW_BLACK_PAIR));
    mvaddch(to.y, to.x, '*');
    attroff(COLOR_PAIR(YELLOW_BLACK_PAIR));
    refresh();
    napms(50);
}

static int calculate_num_missiles(int casterLevel)
{
    // AD&D 2e: 1 missile at level 1, +1 every 2 levels, max 5
    return std::min(5, 1 + (casterLevel - 1) / 2);
}

bool SpellSystem::cast_magic_missile(Creature& caster, GameContext& ctx)
{
    // Get caster level
    auto* player = dynamic_cast<Player*>(&caster);
    int casterLevel = player ? player->get_player_level() : 1;
    int numMissiles = calculate_num_missiles(casterLevel);
    ctx.message_system->log(std::format("DEBUG: casterLevel={}, numMissiles={}", casterLevel, numMissiles));

    // Find all valid targets in FOV
    std::vector<Creature*> targets;
    for (const auto& creature : *ctx.creatures)
    {
        if (creature && creature->destructible && !creature->destructible->is_dead())
        {
            if (ctx.map->is_in_fov(creature->position))
            {
                targets.push_back(creature.get());
            }
        }
    }

    if (targets.empty())
    {
        ctx.message_system->message(RED_BLACK_PAIR, "No valid target in sight!", true);
        return false;
    }

    // Sort by distance (nearest first)
    std::sort(targets.begin(), targets.end(), [&caster](Creature* a, Creature* b) {
        return caster.get_tile_distance(a->position) < caster.get_tile_distance(b->position);
    });

    int totalDamage = 0;
    std::unordered_map<Creature*, int> damagePerTarget;

    // Fire missiles - distribute among targets, prioritizing nearest
    for (int i = 0; i < numMissiles; ++i)
    {
        // Target nearest living enemy
        Creature* target = nullptr;
        for (Creature* t : targets)
        {
            if (t->destructible && !t->destructible->is_dead())
            {
                target = t;
                break;
            }
        }

        if (!target) break;

        animate_magic_missile(caster.position, target->position, i);

        int damage = ctx.dice->roll(1, 4) + 1;
        totalDamage += damage;
        damagePerTarget[target] += damage;

        target->destructible->take_damage(*target, damage, ctx);
    }

    // Message
    ctx.message_system->append_message_part(CYAN_BLACK_PAIR, std::format("Magic Missile ({})! ", numMissiles));
    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "Total ");
    ctx.message_system->append_message_part(RED_BLACK_PAIR, std::format("{} damage!", totalDamage));
    ctx.message_system->finalize_message();

    ctx.creature_manager->cleanup_dead_creatures(*ctx.creatures);

    return true;
}

bool SpellSystem::cast_shield(Creature& caster, GameContext& ctx)
{
    // TODO: Add buff system for temporary AC bonus
    ctx.message_system->append_message_part(CYAN_BLACK_PAIR, "Shield! ");
    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "+4 AC for 5 turns.");
    ctx.message_system->finalize_message();
    return true;
}

bool SpellSystem::cast_sleep(Creature& caster, GameContext& ctx)
{
    int affected = 0;
    int hdAffected = ctx.dice->roll(2, 8); // 2d8 HD worth of creatures

    for (const auto& creature : *ctx.creatures)
    {
        if (creature && creature->destructible && !creature->destructible->is_dead())
        {
            if (ctx.map->is_in_fov(creature->position))
            {
                // Simple: affect creatures with low HP (simulating HD)
                if (creature->destructible->get_max_hp() <= hdAffected * 4)
                {
                    // Put to sleep = instant kill for simplicity
                    creature->destructible->take_damage(*creature, 9999, ctx);
                    affected++;
                    hdAffected -= creature->destructible->get_max_hp() / 4;
                    if (hdAffected <= 0) break;
                }
            }
        }
    }

    if (affected > 0)
    {
        ctx.message_system->append_message_part(CYAN_BLACK_PAIR, "Sleep! ");
        ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format("{} creatures fall into eternal slumber.", affected));
        ctx.message_system->finalize_message();
        ctx.creature_manager->cleanup_dead_creatures(*ctx.creatures);
    }
    else
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "Sleep spell has no effect on these creatures.", true);
    }

    return true;
}

bool SpellSystem::cast_invisibility(Creature& caster, GameContext& ctx)
{
    caster.set_invisible(20);
    ctx.message_system->append_message_part(CYAN_BLACK_PAIR, "Invisibility! ");
    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "You fade from view for 20 turns.");
    ctx.message_system->finalize_message();
    return true;
}

void SpellSystem::show_memorization_menu(Player& player, GameContext& ctx)
{
    CasterClass casterClass = to_caster_class(player.playerClassState);
    auto slots = get_spell_slots(casterClass, player.get_player_level());
    if (slots.empty())
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "You cannot cast spells.", true);
        return;
    }

    int maxSpellLevel = static_cast<int>(slots.size());
    auto available = get_available_spells(casterClass, maxSpellLevel);

    // Clear current memorized spells
    player.memorizedSpells.clear();

    // Auto-memorize spells to fill slots (simplified)
    for (int level = 1; level <= maxSpellLevel; ++level)
    {
        int slotsAtLevel = slots[level - 1];
        for (SpellId id : available)
        {
            if (get_spell_level(id) == level && slotsAtLevel > 0)
            {
                player.memorizedSpells.push_back(id);
                --slotsAtLevel;
            }
        }
    }

    ctx.message_system->append_message_part(CYAN_BLACK_PAIR, "Spells memorized: ");
    for (size_t i = 0; i < player.memorizedSpells.size(); ++i)
    {
        if (i > 0) ctx.message_system->append_message_part(WHITE_BLACK_PAIR, ", ");
        ctx.message_system->append_message_part(GREEN_BLACK_PAIR, get_spell_name(player.memorizedSpells[i]));
    }
    ctx.message_system->finalize_message();
}

void SpellSystem::show_casting_menu(Player& player, GameContext& ctx)
{
    if (player.memorizedSpells.empty())
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "No spells memorized. Rest to memorize spells.", true);
        return;
    }

    // Create spell selection window
    int height = static_cast<int>(player.memorizedSpells.size()) + 4;
    int width = 40;
    int startY = (LINES - height) / 2;
    int startX = (COLS - width) / 2;

    WINDOW* spellWin = newwin(height, width, startY, startX);
    box(spellWin, 0, 0);
    mvwprintw(spellWin, 1, 2, "Cast Spell (ESC to cancel):");

    for (size_t i = 0; i < player.memorizedSpells.size(); ++i)
    {
        const auto& def = get_spell_definition(player.memorizedSpells[i]);
        mvwprintw(spellWin, static_cast<int>(i) + 2, 2, "%c) %s (L%d)",
            'a' + static_cast<int>(i), def.name.c_str(), def.level);
    }

    wrefresh(spellWin);
    int input = getch();
    delwin(spellWin);
    touchwin(stdscr);
    refresh();

    if (input == 27) // ESC
    {
        return;
    }

    int selection = input - 'a';
    if (selection >= 0 && selection < static_cast<int>(player.memorizedSpells.size()))
    {
        SpellId spell = player.memorizedSpells[selection];
        if (cast_spell(spell, player, ctx))
        {
            // Remove spell from memorized list
            player.memorizedSpells.erase(player.memorizedSpells.begin() + selection);
            *ctx.game_status = GameStatus::NEW_TURN;
        }
    }
}
