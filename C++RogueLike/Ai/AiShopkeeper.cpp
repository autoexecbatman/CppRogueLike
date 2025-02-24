// file: AiShopkeeper.cpp
#include <curses.h>
#include "AiShopkeeper.h"
#include "../Game.h"
#include "../Random/RandomDice.h"
#include "../Actor/Actor.h"
#include "../Items.h"
#include "../MenuTrade.h"
#include "../ActorTypes/Player.h"

constexpr auto TRACKING_TURNS = 3; // Used in AiShopkeeper::update()
constexpr auto MIN_TRADE_DISTANCE = 2;  // Minimum distance to initiate a trade

AiShopkeeper::AiShopkeeper() : moveCount(0) {}

// If positionDifference > 0, return 1; otherwise, return -1
int AiShopkeeper::calculateStep(int positionDifference)
{
	return positionDifference > 0 ? 1 : -1;
}

void AiShopkeeper::moveToTarget(Actor& owner, int targetX, int targetY)
{
	// Calculate how many squares away the target is horizontally and vertically
	int moveX = targetX - owner.position.x; // Number of squares to move horizontally
	int moveY = targetY - owner.position.y; // Number of squares to move vertically

	// Decide in which direction to take one step horizontally and vertically
	int stepX = calculateStep(moveX); // Direction to move horizontally (left or right)
	int stepY = calculateStep(moveY); // Direction to move vertically (up or down)

	// List potential moves in order of priority
	std::vector<std::pair<int, int>> moves
	{
		{stepX, stepY},  // First priority: move diagonally
		{stepX, 0},      // Second priority: move horizontally only
		{0, stepY}       // Third priority: move vertically only
	};

	// Try each move in order of priority until one is successful
	for (const auto& move : moves)
	{
		int nextX = owner.position.x + move.first;
		int nextY = owner.position.y + move.second;
		if (game.map->can_walk(Vector2D{ nextY, nextX }))
		{
			owner.position.x = nextX;
			owner.position.y = nextY;
			break; // Exit the loop after a successful move
		}
	}
}

void AiShopkeeper::moveOrTrade(Creature& shopkeeper, int targetx, int targety)
{
	const double distance = sqrt(pow(targetx - shopkeeper.position.x, 2) + pow(targety - shopkeeper.position.y, 2));

	if (distance >= MIN_TRADE_DISTANCE)
	{
		moveToTarget(shopkeeper, targetx, targety);
	}
	else if (!shopkeeper.destructible->is_dead())
	{
		trade(shopkeeper, *game.player);
	}
}

void AiShopkeeper::trade(Creature& shopkeeper, Creature& player)
{
	game.menus.push_back(std::make_unique<MenuTrade>(shopkeeper, player));
	game.shouldInput = false;
}

void AiShopkeeper::update(Creature& owner)
{
	//game.log("Shopkeeper AI update");
	//if (owner.ai == nullptr || owner.destructible->is_dead()) // if the owner has no ai OR if the owner is dead
	//{
	//	return; // do nothing
	//}

	//if (game.map->is_in_fov(owner.position)) // if the owner is in the fov
	//{
	//	// move towards the player
	//	moveCount = TRACKING_TURNS;
	//}
	//else if (moveCount > 0) // if the move count is greater than 0
	//{
	//	moveCount--; // decrement the move count
	//}

	//if (moveCount > 0) // if the move count is greater than 0
	//{
	//	moveOrTrade(owner, game.player->position.x, game.player->position.y); // move or trade with the player
	//}
}

void AiShopkeeper::load(const json& j)
{
	moveCount = j.at("moveCount").get<int>();
}

void AiShopkeeper::save(json& j)
{
	j["type"] = static_cast<int>(AiType::SHOPKEEPER);
	j["moveCount"] = moveCount;
}
// file: AiShopkeeper.cpp
