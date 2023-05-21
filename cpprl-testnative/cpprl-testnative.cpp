#include "pch.h"
#include "CppUnitTest.h"
#define _CRT_SECURE_NO_WARNINGS
#include "../C++RogueLike/Map.h"
#include "../C++RogueLike/Player.h"
#include "../C++RogueLike/Game.h"
#include "../C++RogueLike/Menu.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// https://putridparrot.com/blog/unit-testing-native-c-code-using-visual-studio/
namespace Microsoft::VisualStudio::CppUnitTestFramework {
	template<> inline std::wstring ToString<Game::GameStatus>(const Game::GameStatus& t) {
		switch (t) {
		case Game::GameStatus::STARTUP: return L"STARTUP";
		case Game::GameStatus::IDLE: return L"IDLE";
		case Game::GameStatus::NEW_TURN: return L"NEW_TURN";
		case Game::GameStatus::VICTORY: return L"VICTORY";
		case Game::GameStatus::DEFEAT: return L"DEFEAT";
		default: return L"Unknown GameStatus";
		}
	}
}

namespace cpprltestnative
{
	TEST_CLASS(cpprltestnative)
	{
	public:
		TEST_METHOD(TestMethod1)
		{
			int x = 1;
			int y = 1;

			int actual = x + y;
			int expected = 2;
			Assert::AreEqual(expected, actual);
		}
	};
}

namespace CPPRogueLike
{
	TEST_CLASS(GameTest)
	{
		TEST_METHOD(GameConstructor)
		{
			Game game;
			Assert::AreEqual(22, game.map->map_height);
			Assert::AreEqual(119, game.map->map_width);
		}

		TEST_METHOD(GameCreateMonsters)
		{
			Game game;
			int y = 1;
			int x = 1;
			auto monster = game.map->create_monster<Goblin>(y, x);
			Assert::AreEqual(y, monster->get_posY());
			Assert::AreEqual(x, monster->get_posX());
		}

		TEST_METHOD(GameInit)
		{
			Game game;
			game.init();
			Assert::AreEqual(true, game.stairs->blocks);
			Assert::AreEqual(true, game.stairs->fovOnly);
			Assert::AreEqual(static_cast<int>(Game::GameStatus::STARTUP), static_cast<int>(game.gameStatus));
			Assert::AreEqual(Game::GameStatus::STARTUP, game.gameStatus);
		}
	};

	TEST_CLASS(MenuTest)
	{
		TEST_METHOD(menu)
		{

		}
	};

	TEST_CLASS(MapTest)
	{
		TEST_METHOD(MapConstructor)
		{
			int map_height = 30 - 8;
			int map_width = 119;
			Map map(map_height, map_width);
			Assert::AreEqual(map_height, map.map_height);
			Assert::AreEqual(map_width, map.map_width);
		}

		TEST_METHOD(MapInit)
		{
			Game game;
			Map map(22, 119);
			map.init(true);
			Assert::AreEqual(true, game.stairs->blocks);
		}
		TEST_METHOD(MapCreateMonster)
		{
			int map_height = 30 - 8;
			int map_width = 119;
			Map map(map_height, map_width);
			int y = 1;
			int x = 1;
			auto monster = map.create_monster<Goblin>(y, x);
			Assert::AreEqual(y, monster->get_posY());
			Assert::AreEqual(x, monster->get_posX());
		}

	};

	TEST_CLASS(PlayerTest)
	{
		TEST_METHOD(PlayerConstructor)
		{
			int y = 1;
			int x = 1;
			int maxHp = 1;
			int dr = 1;
			std::string corpseName = "1";
			int xp = 1;
			int dmg = 1;
			bool canSwim = true;
			Player player(y, x, maxHp, dr, corpseName, xp, dmg, canSwim);
			Assert::AreEqual(y, player.getPosY());
			Assert::AreEqual(x, player.getPosX());
			Assert::AreEqual(canSwim, player.canSwim);
		}

		TEST_METHOD(PlayerIsDead)
		{
			int y = 1;
			int x = 1;
			int maxHp = 1;
			int dr = 1;
			std::string corpseName = "1";
			int xp = 1;
			int dmg = 1;
			bool canSwim = true;
			Player player(y, x, maxHp, dr, corpseName, xp, dmg, canSwim);
			Assert::AreEqual(false, player.player_is_dead());
		}
	};


}