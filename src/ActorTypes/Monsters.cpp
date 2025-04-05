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

//==GOBLIN==
ActorData goblinData
{
	'g',
	"goblin",
	GOBLIN_PAIR
};

Goblin::Goblin(Vector2D position) : Creature(position, goblinData)
{
	RandomDice d;
	const int hp = d.d8();
	const int thaco = 20;
	const int ac = 6;
	strength = d.d6() + d.d6() + d.d6();

	weaponEquipped = "Short Sword";

	attacker = std::make_unique<Attacker>("D6");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead goblin", 1000, thaco, ac);

	ai = std::make_unique<AiMonster>();
	add_state(ActorState::CAN_SWIM);
}
//====

//==ORC==
ActorData orcData
{
	'o',
	"orc",
	ORC_PAIR
};

Orc::Orc(Vector2D position) : Creature(position, orcData)
{
	RandomDice d;
	const int damage = d.d10();
	const int hp = d.d10();
	const int thaco = 19;
	const int ac = 6;

	strength = d.d6() + d.d6() + d.d6();

	weaponEquipped = "Long Sword";

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead orc", 35, thaco, ac);

	ai = std::make_unique<AiMonster>();
}
//====

//==TROLL===
ActorData trollData
{
	'T',
	"troll",
	TROLL_PAIR
};

Troll::Troll(Vector2D position) : Creature(position, trollData)
{
	RandomDice d;
	const int damage = d.d10() + 3;
	const int dmgMin = 1;
	const int dmgMax = 13;
	const int hp = d.d12();
	const int thaco = 13;
	const int ac = 4;

	strength = d.d6() + d.d6() + d.d6();

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 1, "dead troll", 100, thaco, ac);

	ai = std::make_unique<AiMonster>();
}
//====

//==DRAGON===
ActorData dragonData
{
	'D',
	"dragon",
	DRAGON_PAIR
};

Dragon::Dragon(Vector2D position) : Creature(position, dragonData)
{
	RandomDice d;
	const int damage = d.d12() + 5;
	const int dmgMin = 1;
	const int dmgMax = 17;
	const int hp = d.d12() + 5;
	const int thaco = 9;
	const int ac = 1;

	strength = d.d6() + d.d6() + d.d6();

	attacker = std::make_unique<Attacker>("D10");
	destructible = std::make_unique<MonsterDestructible>(hp, 2, "dead dragon", 200, thaco, ac);

	ai = std::make_unique<AiMonster>();
}
//====

ActorData shopkeeperData
{
	'S',
	"shopkeeper",
	WHITE_PAIR
};

//==SHOPKEEPER==
Shopkeeper::Shopkeeper(Vector2D position) : Creature(position, shopkeeperData)
{
	destructible = std::make_unique<MonsterDestructible>(10, 0, "dead shopkeeper", 10, 10, 10);
	attacker = std::make_unique<Attacker>("D10");
	ai = std::make_unique<AiShopkeeper>();
	container = std::make_unique<Container>(10);
	auto healthPotion = std::make_unique<Item>(Vector2D{ 0,0 }, ActorData{ '!', "health potion", HPBARMISSING_PAIR });
	healthPotion->pickable = std::make_unique<Healer>(4);
	container->add(std::move(healthPotion));
	auto dagger = std::make_unique<Item>(Vector2D{ 0,0 }, ActorData{ '/', "dagger", 1 });
	dagger->pickable = std::make_unique<Dagger>();
	container->add(std::move(dagger));
	gold = 200;
}

//==ARCHER==
ActorData archerData
{
	'a',
	"archer",
	ORC_PAIR
};

Archer::Archer(Vector2D position) : Creature(position, archerData)
{
	RandomDice d;
	const int hp = d.d8();
	const int thaco = 18;
	const int ac = 7;

	strength = d.d6() + d.d6() + d.d6();
	dexterity = d.d6() + d.d6() + d.d6() + 2; // Archers have higher dexterity

	weaponEquipped = "Longbow";

	attacker = std::make_unique<Attacker>("D8");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead archer", 40, thaco, ac);

	ai = std::make_unique<AiMonster>();
	add_state(ActorState::IS_RANGED); // Mark as a ranged attacker
}

//==MAGE==
ActorData mageData
{
	'm',
	"mage",
	LIGHTNING_PAIR
};

Mage::Mage(Vector2D position) : Creature(position, mageData)
{
	RandomDice d;
	const int hp = d.d6();
	const int thaco = 19;
	const int ac = 9;

	strength = d.d6() + d.d6();
	intelligence = d.d6() + d.d6() + d.d6() + 4; // Mages have higher intelligence

	weaponEquipped = "Staff";

	attacker = std::make_unique<Attacker>("D6");
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead mage", 60, thaco, ac);

	ai = std::make_unique<AiMonster>();
	add_state(ActorState::IS_RANGED); // Mark as a ranged attacker
}

//==MIMIC==
ActorData mimicData
{
	'M',
	"mimic",
	DRAGON_PAIR
};

Mimic::Mimic(Vector2D position) : Creature(position, mimicData)
{
	RandomDice d;
	const int hp = d.d6() + d.d4(); // Reduced from d8+d4+2 to d6+d4
	const int thaco = 17; // Increased from 16 (making it harder to hit)
	const int ac = 7; // Kept the same

	strength = d.d6() + d.d6() + 2; // Reduced from +4 to +2
	dexterity = d.d6() + 2; // Reduced from +4 to +2

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
		{'$', "gold pile", GOLD_PAIR},
		{'!', "health potion", HPBARMISSING_PAIR},
		{'#', "scroll", LIGHTNING_PAIR},
		{'/', "weapon", WHITE_PAIR},
		{'%', "food", HPBARFULL_PAIR}
	};
}

std::vector<Disguise> Mimic::get_possible_disguises() const
{
	return possibleDisguises;
}

// end of file: Goblin.cpp