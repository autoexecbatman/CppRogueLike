// file: Monsters.cpp
#include "Monsters.h"
#include "../Game.h"
#include "../Actor/Actor.h"
#include "../Colors/Colors.h"
#include "../Ai/AiMonster.h"
#include "../Ai/AiPlayer.h"
#include "../Random/RandomDice.h"
#include "../ActorTypes/Healer.h"
#include "../AiMimic.h"

Goblin::Goblin(Vector2D position) : Creature(position, ActorData{ 'g',"goblin",YELLOW_BLACK_PAIR })
{
	RandomDice d;
	const int hp = d.roll(1, 8 - 1);
	const int thaco = 20;
	const int ac = 6;
	const int xp = 35;
	strength = d.d6() + d.d6() + d.d6();
	dexterity = d.d6() + d.d6() + d.d6();
	constitution = d.d6() + d.d6() + d.d6();

	weaponEquipped = "Short Sword";

	attacker = std::make_unique<Attacker>("D6");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead goblin", 35, thaco, ac);

	ai = std::make_unique<AiMonster>();
	add_state(ActorState::CAN_SWIM);
}

Orc::Orc(Vector2D position) : Creature(position, ActorData{ 'o',"orc",RED_BLACK_PAIR })
{
	RandomDice d;
	const int damage = d.d10();
	const int hp = d.d10();
	const int thaco = 19;
	const int ac = 6;

	strength = d.d6() + d.d6() + d.d6();
	dexterity = d.d6() + d.d6() + d.d6();

	weaponEquipped = "Long Sword";

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead orc", 35, thaco, ac);

	ai = std::make_unique<AiMonster>();
}

Troll::Troll(Vector2D position) : Creature(position, ActorData{ 'T', "troll",GREEN_BLACK_PAIR })
{
	RandomDice d;
	const int damage = d.d10() + 3;
	const int dmgMin = 1;
	const int dmgMax = 13;
	const int hp = d.d12();
	const int thaco = 13;
	const int ac = 4;

	strength = d.d6() + d.d6() + d.d6();
	dexterity = d.d6() + d.d6() + d.d6();

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 1, "dead troll", 100, thaco, ac);

	ai = std::make_unique<AiMonster>();
}

Dragon::Dragon(Vector2D position) : Creature(position, ActorData{ 'D',"dragon",RED_YELLOW_PAIR })
{
	RandomDice d;
	const int damage = d.d12() + 5;
	const int dmgMin = 1;
	const int dmgMax = 17;
	const int hp = d.d12() + 5;
	const int thaco = 9;
	const int ac = 1;

	strength = d.d6() + d.d6() + d.d6();
	dexterity = d.d6() + d.d6() + d.d6();

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 2, "dead dragon", 200, thaco, ac);

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
	
	gold = 200;
}

Archer::Archer(Vector2D position) : Creature(position, ActorData{ 'a',"archer",RED_BLACK_PAIR })
{
	RandomDice d;
	const int hp = d.d8();
	const int thaco = 18;
	const int ac = 7;

	strength = d.d6() + d.d6() + d.d6();
	dexterity = d.d6() + d.d6() + d.d6();

	weaponEquipped = "Longbow";

	attacker = std::make_unique<Attacker>("D8");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead archer", 40, thaco, ac);

	ai = std::make_unique<AiMonster>();
	add_state(ActorState::IS_RANGED); // Mark as a ranged attacker
}

Mage::Mage(Vector2D position) : Creature(position, ActorData{ 'm',"mage",WHITE_BLUE_PAIR })
{
	RandomDice d;
	const int hp = d.d6();
	const int thaco = 19;
	const int ac = 9;

	strength = d.d6() + d.d6() + d.d6();
	dexterity = d.d6() + d.d6() + d.d6();
	intelligence = d.d6() + d.d6() + d.d6(); // Mages have higher intelligence

	weaponEquipped = "Staff";

	attacker = std::make_unique<Attacker>("D6");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead mage", 60, thaco, ac);

	ai = std::make_unique<AiMonster>();
	add_state(ActorState::IS_RANGED); // Mark as a ranged attacker
}

Mimic::Mimic(Vector2D position) : Creature(position, ActorData{ 'M',"mimic",RED_YELLOW_PAIR })
{
	RandomDice d;
	const int hp = d.d6() + d.d4(); // Reduced from d8+d4+2 to d6+d4
	const int thaco = 17; // Increased from 16 (making it harder to hit)
	const int ac = 7; // Kept the same

	strength = d.d6() + d.d6() + d.d6();
	dexterity = d.d6() + d.d6() + d.d6();

	attacker = std::make_unique<Attacker>("D4"); // Reduced from D6
	destructible = std::make_unique<MonsterDestructible>(hp, 1, "dead mimic", 50, thaco, ac);

	// Create a specialized AI for this mimic
	ai = std::make_unique<AiMimic>();

	// Initialize the disguises list
	initDisguises();

	// Randomly choose initial disguise
	int index = d.roll(0, possibleDisguises.size() - 1);
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