// file: Monsters.cpp
#include "Monsters.h"
#include "../Game.h"
#include "../Actor/Actor.h"
#include "../Colors/Colors.h"
#include "../Ai/AiMonster.h"
#include "../Ai/AiPlayer.h"
#include "../Random/RandomDice.h"
#include "../ActorTypes/Healer.h"

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
	destructible = std::make_unique<MonsterDestructible>(hp, 0, "dead goblin", 15, thaco, ac);

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
	const int hp = d.d8() + d.d4() + 2;
	const int thaco = 16;
	const int ac = 7;

	strength = d.d6() + d.d6() + 4;
	dexterity = d.d6() + 4; // Mimics are dexterous

	attacker = std::make_unique<Attacker>("D6");
	destructible = std::make_unique<MonsterDestructible>(hp, 1, "dead mimic", 50, thaco, ac);

	ai = std::make_unique<AiMonster>();

	// Randomly choose initial disguise
	changeDisguise();
}

void Mimic::consumeNearbyItems()
{
	// Only consuming items when revealed and active
	if (isDisguised || destructible->is_dead()) return;

	std::vector<size_t> itemsToRemove;

	// Check for items in a 1-tile radius
	for (size_t i = 0; i < game.container->inv.size(); i++)
	{
		auto& item = game.container->inv[i];
		if (item && get_tile_distance(item->position) <= 1)
		{
			// Found an item to consume!
			game.appendMessagePart(DRAGON_PAIR, "The mimic ");
			game.appendMessagePart(WHITE_PAIR, "consumes the ");
			game.appendMessagePart(item->actorData.color, item->actorData.name);
			game.appendMessagePart(WHITE_PAIR, "!");
			game.finalizeMessage();

			// Grow stronger based on item type
			itemsConsumed++;

			// Different bonuses based on item type
			if (item->actorData.name.find("potion") != std::string::npos)
			{
				// Health potions give the mimic more HP
				destructible->hpMax += 2;
				destructible->hp += 2;
			}
			else if (item->actorData.name.find("scroll") != std::string::npos)
			{
				// Scrolls make the mimic's confusion more potent
				confusionDuration += 1;
			}
			else if (item->actorData.name.find("gold") != std::string::npos)
			{
				// Gold makes the mimic more defensive
				destructible->dr += 1;
			}
			else if (item->actorData.name.find("weapon") != std::string::npos ||
				item->actorData.name.find("sword") != std::string::npos ||
				item->actorData.name.find("dagger") != std::string::npos)
			{
				// Weapons make the mimic hit harder
				int currentDamage = game.d.roll_from_string(attacker->roll);
				attacker->roll = "D" + std::to_string(currentDamage + 2);
			}

			// Mark item for removal
			itemsToRemove.push_back(i);

			// If mimic has consumed enough items, change appearance based on power level
			if (itemsConsumed >= 3)
			{
				actorData.ch = 'W'; // Different character for well-fed mimic
				actorData.color = FIREBALL_PAIR; // More dangerous color
				actorData.name = "greater mimic"; // New name
			}

			// Only consume one item per turn to give player a chance
			break;
		}
	}

	// Remove consumed items (in reverse order to avoid index issues)
	for (auto it = itemsToRemove.rbegin(); it != itemsToRemove.rend(); ++it)
	{
		game.container->inv[*it].reset();
		game.container->inv.erase(game.container->inv.begin() + *it);
	}
}

void Mimic::changeDisguise()
{
	if (!isDisguised) return; // Don't change if already revealed

	// Select a random disguise
	int index = game.d.roll(0, possibleDisguises.size() - 1);
	Disguise chosen = possibleDisguises[index];

	// Apply the disguise
	actorData.ch = chosen.ch;
	actorData.name = chosen.name;
	actorData.color = chosen.color;

	// Remove the BLOCKS state while disguised
	remove_state(ActorState::BLOCKS);
}

void Mimic::update()
{
	// Call the consumeNearbyItems method
	consumeNearbyItems();

	// Rest of the existing update logic...
	// Increment disguise change counter
	disguiseChangeCounter++;

	// Occasionally change disguise if still hidden
	if (isDisguised && disguiseChangeCounter >= disguiseChangeRate)
	{
		changeDisguise();
		disguiseChangeCounter = 0;
	}

	// If still disguised, check if player is close enough to reveal
	if (isDisguised)
	{
		int distance = get_tile_distance(game.player->position);

		if (distance <= revealDistance)
		{
			// Reveal true form!
			isDisguised = false;
			actorData.ch = 'M';
			actorData.name = "mimic";
			actorData.color = DRAGON_PAIR;
			add_state(ActorState::BLOCKS); // Now it's solid

			// Try to confuse the player
			if (game.d.d20() > game.player->wisdom)
			{
				game.appendMessagePart(CONFUSION_PAIR, "The ");
				game.appendMessagePart(DRAGON_PAIR, "mimic");
				game.appendMessagePart(CONFUSION_PAIR, " reveals itself and confuses you!");
				game.finalizeMessage();

				// Apply confusion to player
				game.player->add_state(ActorState::IS_CONFUSED);

				// Assuming we can cast to AiPlayer safely
				auto playerAi = dynamic_cast<AiPlayer*>(game.player->ai.get());
				if (playerAi)
				{
					playerAi->applyConfusion(confusionDuration);
				}
			}
			else
			{
				game.appendMessagePart(DRAGON_PAIR, "A mimic");
				game.appendMessagePart(WHITE_PAIR, " reveals itself but you resist its confusion!");
				game.finalizeMessage();
			}
		}
	}

	// If revealed, act like a normal monster
	if (!isDisguised)
	{
		Creature::update();
	}
}

void Mimic::render() const noexcept
{
	// If disguised, occasionally make it slightly shift its appearance
	// to give the player a subtle clue it's not a normal item
	if (isDisguised && is_visible())
	{
		attron(COLOR_PAIR(actorData.color));

		// Every so often, briefly flicker to a different symbol
		if (game.d.d20() == 1) {
			mvaddch(position.y, position.x, 'M'); // Brief flash of true form
		}
		else {
			mvaddch(position.y, position.x, actorData.ch);
		}

		attroff(COLOR_PAIR(actorData.color));
	}
	else
	{
		// Default rendering for non-disguised state
		Creature::render();
	}
}
// end of file: Goblin.cpp