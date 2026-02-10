// file: MonsterCreator.cpp
#include <algorithm>

#include "MonsterCreator.h"
#include "../Actor/Actor.h"
#include "../Actor/Attacker.h"
#include "../Actor/Destructible.h"
#include "../Ai/AiMonster.h"
#include "../Ai/AiMonsterRanged.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"

namespace
{
    int roll_dice(RandomDice* dice, const DiceExpr& expr)
    {
        if (expr.num == 0)
            return 0;
        int total = 0;
        for (int i = 0; i < expr.num; ++i)
            total += dice->roll(1, expr.sides);
        return total + expr.bonus;
    }

    // AD&D 2e monster registry.
    // DiceExpr format: { num_dice, sides, bonus }  e.g. { 3, 6, 2 } = 3d6+2
    // Ability score guidelines:
    //   Exceptional  18+  e.g. dragon STR 5d6 avg 17
    //   High         15   e.g. 3d6+3
    //   Above avg    12   e.g. 3d6+1 or 2d6+4
    //   Average      10   e.g. 3d6
    //   Below avg     8   e.g. 2d6+2
    //   Low           6   e.g. 1d6+3 or 2d6+1
    //   Poor          4   e.g. 1d4+2
    //   Animal        2   e.g. 1d4
    //   Minimal       1   e.g. 1d2
    const std::unordered_map<MonsterId, MonsterParams> s_registry =
    {
        // === GOBLIN ===
        // Small humanoid: weak and dim but nimble. Low morale.
        // HD 1-1, AC 6, THAC0 20. XP 35 (x3 solo = 105).
        {
            MonsterId::GOBLIN,
            {
                .symbol = 'g',
                .name = "goblin",
                .color = YELLOW_BLACK_PAIR,
                .corpse_name = "dead goblin",
                .hp_dice = { 1, 8, -1 },
                .thaco = 20,
                .ac = 6,
                .xp = 105,
                .dr = 0,
                .str_dice = { 2, 6, 1 },   // avg  8 — weak
                .dex_dice = { 3, 6, 0 },   // avg 10 — average
                .con_dice = { 2, 6, 1 },   // avg  8 — average
                .int_dice = { 1, 6, 3 },   // avg  6 — dim
                .wis_dice = { 1, 6, 3 },   // avg  6 — poor judgement
                .cha_dice = { 1, 4, 1 },   // avg  3 — repellent
                .weapon_name = "Short Sword",
                .damage = DamageValues::ShortSword(),
                .ai_type = MonsterAiType::MELEE,
                .can_swim = true,
                .base_weight = 30,
                .level_minimum = 1,
                .level_maximum = 5,
                .level_scaling = -0.2f,
            }
        },
        // === ORC ===
        // Medium humanoid: brutish and aggressive. Stronger than goblin.
        // HD 1, AC 6, THAC0 19. XP 35 (x3 solo = 105).
        {
            MonsterId::ORC,
            {
                .symbol = 'o',
                .name = "orc",
                .color = RED_BLACK_PAIR,
                .corpse_name = "dead orc",
                .hp_dice = { 1, 10, 0 },
                .thaco = 19,
                .ac = 6,
                .xp = 105,
                .dr = 0,
                .str_dice = { 3, 6, 1 },   // avg 11 — above average
                .dex_dice = { 2, 6, 2 },   // avg  9 — slightly below
                .con_dice = { 3, 6, 0 },   // avg 10 — average
                .int_dice = { 2, 6, 1 },   // avg  8 — dim
                .wis_dice = { 2, 6, 1 },   // avg  8 — poor
                .cha_dice = { 1, 6, 2 },   // avg  5 — brutish
                .weapon_name = "Long Sword",
                .damage = DamageValues::LongSword(),
                .ai_type = MonsterAiType::MELEE,
                .base_weight = 15,
                .level_minimum = 2,
                .level_maximum = 0,
                .level_scaling = 0.0f,
            }
        },
        // === TROLL ===
        // Large regenerating predator. Strong, tough, stupid.
        // HD 6+6, AC 4, THAC0 13. XP 100 (x3 solo = 300).
        {
            MonsterId::TROLL,
            {
                .symbol = 'T',
                .name = "troll",
                .color = GREEN_BLACK_PAIR,
                .corpse_name = "dead troll",
                .hp_dice = { 1, 12, 0 },
                .thaco = 13,
                .ac = 4,
                .xp = 300,
                .dr = 1,
                .str_dice = { 4, 6, 0 },   // avg 14 — strong
                .dex_dice = { 3, 6, 0 },   // avg 10 — average
                .con_dice = { 4, 6, 0 },   // avg 14 — tough
                .int_dice = { 1, 4, 1 },   // avg  3 — very low
                .wis_dice = { 1, 4, 2 },   // avg  4 — animal-level
                .cha_dice = { 1, 4, 0 },   // avg  2 — terrifying
                .weapon_name = "Claws",
                .damage = DamageValues::GreatSword(),
                .ai_type = MonsterAiType::MELEE,
                .base_weight = 7,
                .level_minimum = 4,
                .level_maximum = 0,
                .level_scaling = 0.5f,
            }
        },
        // === DRAGON ===
        // Apex predator. Exceptional physical and mental stats.
        // HD 8-9, AC 1, THAC0 9. XP 200 (x3 solo = 600).
        {
            MonsterId::DRAGON,
            {
                .symbol = 'D',
                .name = "dragon",
                .color = RED_YELLOW_PAIR,
                .corpse_name = "dead dragon",
                .hp_dice = { 1, 12, 5 },
                .thaco = 9,
                .ac = 1,
                .xp = 600,
                .dr = 2,
                .str_dice = { 5, 6, 0 },   // avg 17 — exceptional
                .dex_dice = { 3, 6, 0 },   // avg 10 — average
                .con_dice = { 5, 6, 0 },   // avg 17 — exceptional
                .int_dice = { 3, 6, 3 },   // avg 13 — high
                .wis_dice = { 3, 6, 0 },   // avg 10 — average
                .cha_dice = { 4, 6, 0 },   // avg 14 — impressive
                .weapon_name = "Fiery Breath",
                .damage = { 1, 17, "1d12+5", DamageType::FIRE },
                .ai_type = MonsterAiType::MELEE,
                .base_weight = 3,
                .level_minimum = 5,
                .level_maximum = 0,
                .level_scaling = 1.0f,
            }
        },
        // === ARCHER ===
        // Human ranged fighter. Balanced human stats, good dexterity.
        // HD 1, AC 7, THAC0 18. XP 40 (x3 solo = 120).
        {
            MonsterId::ARCHER,
            {
                .symbol = 'a',
                .name = "archer",
                .color = RED_BLACK_PAIR,
                .corpse_name = "dead archer",
                .hp_dice = { 1, 8, 0 },
                .thaco = 18,
                .ac = 7,
                .xp = 120,
                .dr = 0,
                .str_dice = { 3, 6, 0 },   // avg 10 — average
                .dex_dice = { 3, 6, 2 },   // avg 12 — good (archer trait)
                .con_dice = { 3, 6, 0 },   // avg 10 — average
                .int_dice = { 3, 6, 0 },   // avg 10 — average
                .wis_dice = { 3, 6, 0 },   // avg 10 — average
                .cha_dice = { 3, 6, 0 },   // avg 10 — average
                .weapon_name = "Longbow",
                .damage = DamageValues::LongBow(),
                .ai_type = MonsterAiType::RANGED,
                .base_weight = 10,
                .level_minimum = 2,
                .level_maximum = 0,
                .level_scaling = 0.2f,
            }
        },
        // === MAGE ===
        // Human wizard. Frail body, exceptional mind.
        // HD 1, AC 9, THAC0 19. XP 60 (x3 solo = 180).
        {
            MonsterId::MAGE,
            {
                .symbol = 'm',
                .name = "mage",
                .color = WHITE_BLUE_PAIR,
                .corpse_name = "dead mage",
                .hp_dice = { 1, 6, 0 },
                .thaco = 19,
                .ac = 9,
                .xp = 180,
                .dr = 0,
                .str_dice = { 2, 6, 0 },   // avg  7 — weak
                .dex_dice = { 3, 6, 0 },   // avg 10 — average
                .con_dice = { 2, 6, 0 },   // avg  7 — frail
                .int_dice = { 3, 6, 3 },   // avg 13 — high (wizard trait)
                .wis_dice = { 3, 6, 2 },   // avg 12 — above average
                .cha_dice = { 3, 6, 0 },   // avg 10 — average
                .weapon_name = "Staff",
                .damage = DamageValues::Staff(),
                .ai_type = MonsterAiType::RANGED,
                .base_weight = 10,
                .level_minimum = 3,
                .level_maximum = 0,
                .level_scaling = 0.3f,
            }
        },
        // === WOLF ===
        // Natural predator. Quick and with good instincts. Low intelligence.
        // HD 2+2, AC 7, THAC0 19. XP 15 (x3 solo = 45).
        {
            MonsterId::WOLF,
            {
                .symbol = 'w',
                .name = "wolf",
                .color = BROWN_BLACK_PAIR,
                .corpse_name = "dead wolf",
                .hp_dice = { 2, 4, 0 },
                .thaco = 19,
                .ac = 7,
                .xp = 45,
                .dr = 0,
                .str_dice = { 3, 6, 0 },   // avg 10 — average
                .dex_dice = { 3, 6, 2 },   // avg 12 — quick
                .con_dice = { 3, 6, 0 },   // avg 10 — average
                .int_dice = { 1, 4, 1 },   // avg  3 — animal
                .wis_dice = { 2, 6, 2 },   // avg  9 — good pack instincts
                .cha_dice = { 1, 4, 0 },   // avg  2 — irrelevant
                .weapon_name = "Bite",
                .damage = DamageValues::Dagger(),
                .ai_type = MonsterAiType::MELEE,
                .base_weight = 20,
                .level_minimum = 1,
                .level_maximum = 5,
                .level_scaling = -0.1f,
            }
        },
        // === FIRE WOLF ===
        // Magical fire-infused wolf. Tougher than normal, slightly smarter.
        // HD 3+3, AC 6, THAC0 18. XP 40 (x3 solo = 120).
        {
            MonsterId::FIRE_WOLF,
            {
                .symbol = 'w',
                .name = "fire wolf",
                .color = RED_YELLOW_PAIR,
                .corpse_name = "charred wolf corpse",
                .hp_dice = { 2, 6, 0 },
                .thaco = 18,
                .ac = 6,
                .xp = 120,
                .dr = 0,
                .str_dice = { 3, 6, 2 },   // avg 12 — strong
                .dex_dice = { 3, 6, 2 },   // avg 12 — quick
                .con_dice = { 3, 6, 2 },   // avg 12 — tough
                .int_dice = { 1, 6, 2 },   // avg  5 — slightly above animal
                .wis_dice = { 2, 6, 2 },   // avg  9 — good instincts
                .cha_dice = { 1, 4, 1 },   // avg  3 — intimidating
                .weapon_name = "Flaming Bite",
                .damage = { 1, 6, "1d6", DamageType::FIRE },
                .ai_type = MonsterAiType::MELEE,
                .base_weight = 8,
                .level_minimum = 3,
                .level_maximum = 0,
                .level_scaling = 0.3f,
            }
        },
        // === ICE WOLF ===
        // Magical cold-infused wolf. Mirror of fire wolf with cold damage.
        // HD 3+3, AC 6, THAC0 18. XP 40 (x3 solo = 120).
        {
            MonsterId::ICE_WOLF,
            {
                .symbol = 'w',
                .name = "ice wolf",
                .color = CYAN_BLUE_PAIR,
                .corpse_name = "frozen wolf corpse",
                .hp_dice = { 2, 6, 0 },
                .thaco = 18,
                .ac = 6,
                .xp = 120,
                .dr = 0,
                .str_dice = { 3, 6, 2 },   // avg 12 — strong
                .dex_dice = { 3, 6, 2 },   // avg 12 — quick
                .con_dice = { 3, 6, 2 },   // avg 12 — tough
                .int_dice = { 1, 6, 2 },   // avg  5 — slightly above animal
                .wis_dice = { 2, 6, 2 },   // avg  9 — good instincts
                .cha_dice = { 1, 4, 1 },   // avg  3 — intimidating
                .weapon_name = "Freezing Bite",
                .damage = { 1, 6, "1d6", DamageType::COLD },
                .ai_type = MonsterAiType::MELEE,
                .base_weight = 8,
                .level_minimum = 3,
                .level_maximum = 0,
                .level_scaling = 0.3f,
            }
        },
        // === BAT ===
        // Tiny flying pest. Negligible threat but hard to hit. Near-zero INT.
        // HD 1-1, AC 8, THAC0 20. XP 10 (x3 solo = 30).
        {
            MonsterId::BAT,
            {
                .symbol = 'b',
                .name = "bat",
                .color = MAGENTA_BLACK_PAIR,
                .corpse_name = "dead bat",
                .hp_dice = { 1, 2, 0 },
                .thaco = 20,
                .ac = 8,
                .xp = 30,
                .dr = 0,
                .str_dice = { 1, 4, 0 },   // avg  2 — negligible
                .dex_dice = { 3, 6, 6 },   // avg 17 — exceptional agility
                .con_dice = { 1, 4, 1 },   // avg  3 — fragile
                .int_dice = { 1, 2, 0 },   // avg  1 — minimal
                .wis_dice = { 2, 6, 0 },   // avg  7 — echolocation instinct
                .cha_dice = { 1, 2, 0 },   // avg  1 — irrelevant
                .weapon_name = "Bite",
                .damage = DamageValues::Unarmed(),
                .ai_type = MonsterAiType::MELEE,
                .base_weight = 15,
                .level_minimum = 1,
                .level_maximum = 3,
                .level_scaling = -0.3f,
            }
        },
        // === KOBOLD ===
        // Tiny reptilian humanoid. Weaker than goblin but craftier in traps.
        // HD 1-4 HP, AC 7, THAC0 20. XP 15 (x3 solo = 45).
        {
            MonsterId::KOBOLD,
            {
                .symbol = 'k',
                .name = "kobold",
                .color = RED_BLACK_PAIR,
                .corpse_name = "dead kobold",
                .hp_dice = { 1, 4, 0 },
                .thaco = 20,
                .ac = 7,
                .xp = 45,
                .dr = 0,
                .str_dice = { 2, 6, 1 },   // avg  8 — weak
                .dex_dice = { 3, 6, 1 },   // avg 11 — nimble
                .con_dice = { 2, 6, 0 },   // avg  7 — fragile
                .int_dice = { 2, 6, 1 },   // avg  8 — low but trap-cunning
                .wis_dice = { 2, 6, 0 },   // avg  7 — poor
                .cha_dice = { 1, 6, 1 },   // avg  4 — unpleasant
                .weapon_name = "Dagger",
                .damage = DamageValues::Dagger(),
                .ai_type = MonsterAiType::MELEE,
                .base_weight = 25,
                .level_minimum = 1,
                .level_maximum = 4,
                .level_scaling = -0.2f,
            }
        },
    };
} // anonymous namespace

const std::unordered_map<MonsterId, MonsterParams>& MonsterCreator::get_registry()
{
    return s_registry;
}

std::unique_ptr<Creature> MonsterCreator::create_from_params(Vector2D pos, const MonsterParams& params, GameContext& ctx)
{
    auto c = std::make_unique<Creature>(pos, ActorData{ params.symbol, params.name, params.color });

    c->set_strength(std::max(1, roll_dice(ctx.dice, params.str_dice)));
    c->set_dexterity(std::max(1, roll_dice(ctx.dice, params.dex_dice)));
    c->set_constitution(std::max(1, roll_dice(ctx.dice, params.con_dice)));
    c->set_intelligence(std::max(1, roll_dice(ctx.dice, params.int_dice)));
    c->set_wisdom(std::max(1, roll_dice(ctx.dice, params.wis_dice)));
    c->set_charisma(std::max(1, roll_dice(ctx.dice, params.cha_dice)));

    c->set_weapon_equipped(params.weapon_name);

    const int hp = std::max(1, roll_dice(ctx.dice, params.hp_dice));

    c->attacker = std::make_unique<Attacker>(params.damage);
    c->destructible = std::make_unique<MonsterDestructible>(hp, params.dr, params.corpse_name, params.xp, params.thaco, params.ac);
    c->destructible->set_last_constitution(c->get_constitution());

    if (params.ai_type == MonsterAiType::RANGED)
    {
        c->ai = std::make_unique<AiMonsterRanged>();
        c->add_state(ActorState::IS_RANGED);
    }
    else
    {
        c->ai = std::make_unique<AiMonster>();
    }

    if (params.can_swim)
        c->add_state(ActorState::CAN_SWIM);

    return c;
}

std::unique_ptr<Creature> MonsterCreator::create(Vector2D pos, MonsterId id, GameContext& ctx)
{
    return create_from_params(pos, s_registry.at(id), ctx);
}
