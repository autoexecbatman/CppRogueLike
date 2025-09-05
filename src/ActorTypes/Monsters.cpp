// file: Monsters.cpp
/*
 * SOLO AD&D 2E XP BONUS SYSTEM
 * ============================
 * 
 * All monster XP values have been TRIPLED from standard AD&D 2e values to compensate
 * for solo play. In traditional AD&D 2e, XP is shared among party members (typically 4-6),
 * making solo play extremely slow for character progression.
 * 
 * STANDARD vs SOLO XP VALUES:
 * - Goblin:    35 XP  → 105 XP  (3x multiplier)
 * - Orc:       35 XP  → 105 XP  (3x multiplier) 
 * - Archer:    40 XP  → 120 XP  (3x multiplier)
 * - Mage:      60 XP  → 180 XP  (3x multiplier)
 * - Mimic:     50 XP  → 150 XP  (3x multiplier)
 * - Troll:    100 XP  → 300 XP  (3x multiplier)
 * - Dragon:   200 XP  → 600 XP  (3x multiplier)
 * 
 * DESIGN RATIONALE:
 * 1. Solo characters face the same challenges as a full party
 * 2. No XP sharing means all experience goes to one character
 * 3. Maintains engaging progression pace without grinding
 * 4. Preserves AD&D 2e level progression curves and thresholds
 * 5. Balances risk vs reward for solo adventuring
 * 
 * This system ensures solo players can enjoy meaningful character progression
 * while maintaining the tactical challenge of AD&D 2e combat.
 */
#include "Monsters.h"
#include "../Game.h"
#include "../Actor/Actor.h"
#include "../Colors/Colors.h"
#include "../Ai/AiMonster.h"
#include "../Ai/AiPlayer.h"
#include "../Ai/AiShopkeeper.h"
#include "../Random/RandomDice.h"
#include "../ActorTypes/Healer.h"
#include "../Ai/AiMimic.h"

Goblin::Goblin(Vector2D position) : Creature(position, ActorData{ 'g',"goblin",YELLOW_BLACK_PAIR })
{
	const int hp = game.d.roll(1, 8 - 1);
	const int thaco = 20;
	const int ac = 6;
	const int xp = 105; // TRIPLED from 35 for solo play bonus
	set_strength(game.d.d6() + game.d.d6() + game.d.d6());
	set_dexterity(game.d.d6() + game.d.d6() + game.d.d6());
	set_constitution(game.d.d6() + game.d.d6() + game.d.d6());

	set_weapon_equipped("Short Sword");

	attacker = std::make_unique<Attacker>("D6");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead goblin", 105, thaco, ac);

	ai = std::make_unique<AiMonster>();
	add_state(ActorState::CAN_SWIM);
}

Orc::Orc(Vector2D position) : Creature(position, ActorData{ 'o',"orc",RED_BLACK_PAIR })
{
	const int damage = game.d.d10();
	const int hp = game.d.d10();
	const int thaco = 19;
	const int ac = 6;

	set_strength(game.d.d6() + game.d.d6() + game.d.d6());
	set_dexterity(game.d.d6() + game.d.d6() + game.d.d6());

	set_weapon_equipped("Long Sword");

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead orc", 105, thaco, ac); // TRIPLED from 35 for solo play bonus

	ai = std::make_unique<AiMonster>();
}

Troll::Troll(Vector2D position) : Creature(position, ActorData{ 'T', "troll",GREEN_BLACK_PAIR })
{
	const int damage = game.d.d10() + 3;
	const int dmgMin = 1;
	const int dmgMax = 13;
	const int hp = game.d.d12();
	const int thaco = 13;
	const int ac = 4;

	set_strength(game.d.d6() + game.d.d6() + game.d.d6());
	set_dexterity(game.d.d6() + game.d.d6() + game.d.d6());

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 1, "dead troll", 300, thaco, ac); // TRIPLED from 100 for solo play bonus

	ai = std::make_unique<AiMonster>();
}

Dragon::Dragon(Vector2D position) : Creature(position, ActorData{ 'D',"dragon",RED_YELLOW_PAIR })
{
	const int damage = game.d.d12() + 5;
	const int dmgMin = 1;
	const int dmgMax = 17;
	const int hp = game.d.d12() + 5;
	const int thaco = 9;
	const int ac = 1;

	set_strength(game.d.d6() + game.d.d6() + game.d.d6());
	set_dexterity(game.d.d6() + game.d.d6() + game.d.d6());

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 2, "dead dragon", 600, thaco, ac); // TRIPLED from 200 for solo play bonus

	ai = std::make_unique<AiMonster>();
}

Shopkeeper::Shopkeeper(Vector2D position) : Creature(position, ActorData{ 'S',"shopkeeper",WHITE_BLACK_PAIR })
{
	destructible = std::make_unique<MonsterDestructible>(10, 0, "dead shopkeeper", 10, 10, 10);
	attacker = std::make_unique<Attacker>("D10");
	ai = std::make_unique<AiShopkeeper>();
	container = std::make_unique<Container>(10);
	
	// NOTE: Inventory is populated in MonsterFactory.cpp - do not add items here
	// This ensures single source of truth for shopkeeper inventory
	
	set_gold(200);
}

Archer::Archer(Vector2D position) : Creature(position, ActorData{ 'a',"archer",RED_BLACK_PAIR })
{
	const int hp = game.d.d8();
	const int thaco = 18;
	const int ac = 7;

	set_strength(game.d.d6() + game.d.d6() + game.d.d6());
	set_dexterity(game.d.d6() + game.d.d6() + game.d.d6());

	set_weapon_equipped("Longbow");

	attacker = std::make_unique<Attacker>("D8");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead archer", 120, thaco, ac); // TRIPLED from 40 for solo play bonus

	ai = std::make_unique<AiMonster>();
	add_state(ActorState::IS_RANGED); // Mark as a ranged attacker
}

Mage::Mage(Vector2D position) : Creature(position, ActorData{ 'm',"mage",WHITE_BLUE_PAIR })
{
	const int hp = game.d.d6();
	const int thaco = 19;
	const int ac = 9;

	set_strength(game.d.d6() + game.d.d6() + game.d.d6());
	set_dexterity(game.d.d6() + game.d.d6() + game.d.d6());
	set_intelligence(game.d.d6() + game.d.d6() + game.d.d6()); // Mages have higher intelligence

	set_weapon_equipped("Staff");

	attacker = std::make_unique<Attacker>("D6");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead mage", 180, thaco, ac); // TRIPLED from 60 for solo play bonus

	ai = std::make_unique<AiMonster>();
	add_state(ActorState::IS_RANGED); // Mark as a ranged attacker
}

Mimic::Mimic(Vector2D position) : Creature(position, ActorData{ 'M',"mimic",RED_YELLOW_PAIR })
{
	const int hp = game.d.d6() + game.d.d4(); // Reduced from d8+d4+2 to d6+d4
	const int thaco = 17; // Increased from 16 (making it harder to hit)
	const int ac = 7; // Kept the same

	set_strength(game.d.d6() + game.d.d6() + game.d.d6());
	set_dexterity(game.d.d6() + game.d.d6() + game.d.d6());

	attacker = std::make_unique<Attacker>("D4"); // Reduced from D6
	destructible = std::make_unique<MonsterDestructible>(hp, 1, "dead mimic", 150, thaco, ac); // TRIPLED from 50 for solo play bonus

	// Create a specialized AI for this mimic
	ai = std::make_unique<AiMimic>();

	// Initialize the disguises list
	initDisguises();

	// Randomly choose initial disguise
	int index = game.d.roll(0, possibleDisguises.size() - 1);
	const auto& chosen = possibleDisguises[index];

	// Apply initial disguise
	actorData.ch = chosen.ch;
	actorData.name = chosen.name;
	actorData.color = chosen.color;

	// Remove BLOCKS state while disguised
	remove_state(ActorState::BLOCKS);
}

void Mimic::initDisguises()
{
	// Define possible disguises
	possibleDisguises = {
		{'$', "gold pile", YELLOW_BLACK_PAIR},
		{'!', "health potion", WHITE_RED_PAIR},
		{'#', "scroll", WHITE_BLUE_PAIR},
		{'/', "weapon", WHITE_BLACK_PAIR},
		{'%', "food", WHITE_GREEN_PAIR}
	};
}

std::vector<Disguise> Mimic::get_possible_disguises() const
{
	return possibleDisguises;
}

// end of file: Goblin.cpp