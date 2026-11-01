// file: SilenceSpellTest.cpp
// What casting Silence does to the creature the caster targets. The 2e spell
// stops all sound so that "spells cannot be cast" in its area, for "two
// rounds for each level of experience of the priest", and "can be centered
// upon a creature" (the 2e text extracted from the repo-root PDF, the
// "Silence, 15' Radius" entry). The game silences the one creature on the
// chosen tile, so the creature targeted ends silenced and cannot cast.
//
// The cursor is driven with scripted keys, and moving it draws, so each case
// runs in a HiddenRenderer child.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SilenceSpellTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string_view>
#include <utility>

#include "src/Actor.h"
#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InputSystem.h"
#include "src/Map.h"
#include "src/Player.h"
#include "src/SpellSystem.h"
#include "src/TileRef.h"
#include "src/TileType.h"
#include "src/Vector2D.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/MockGameContext.h"

namespace CppRL
{

	namespace
	{
		constexpr int CORRIDOR_ROW = 5;
		constexpr Vector2D CASTER_TILE{ 3, CORRIDOR_ROW };
		constexpr Vector2D ORC_TILE{ 5, CORRIDOR_ROW };

		// Casts the spell under key at the orc two tiles east, pressing Right
		// twice and Enter, and exits unless the orc's silence is as expected
		// and the casting was spent once.
		[[noreturn]] void cast_at_the_orc(
			std::string_view key,
			bool isOrcSilenced)
		{
			HiddenRenderer hidden{};
			MockGameContext mock{};
			GameContext ctx = mock.to_game_context();
			ctx.renderer = &hidden.renderer;

			mock.map.init_tiles();
			for (int col = 1; col < 10; ++col)
			{
				mock.map.set_tile(
					Vector2D{ col, CORRIDOR_ROW },
					TileType::FLOOR);
			}
			mock.player = std::make_unique<Player>(CASTER_TILE);
			mock.player->healthPool = std::make_unique<HealthPool>(20);
			mock.player->armorClass = std::make_unique<ArmorClass>(10);
			mock.player->experienceReward =
				std::make_unique<ExperienceReward>(0);
			mock.player->set_creature_level(3);
			auto orc = std::make_unique<Creature>(
				ORC_TILE,
				ActorData{ TileRef{}, "orc", ColorPairId::WHITE_BLACK });
			orc->healthPool = std::make_unique<HealthPool>(8);
			Creature& target = *orc;
			mock.creatures.push_back(std::move(orc));

			int castingsSpent = 0;
			const auto count_casting = [&castingsSpent](GameContext& castingCtx)
			{
				++castingsSpent;
			};
			SpellSystem::cast_spell_by_key(
				key,
				*mock.player,
				SpellSource::MEMORIZED,
				count_casting,
				ctx);
			exit_unless(
				mock.menus.size() == 1,
				"the spell opened no targeting cursor");

			for (
				const GameKey pressed :
				{ GameKey::RIGHT, GameKey::RIGHT, GameKey::ENTER })
			{
				ctx.inputSystem->queue_key(pressed, 0);
				// Inside a frame, which Game::tick opens and closes around
				// every menu.
				hidden.renderer.begin_frame();
				mock.menus.back()->menu(ctx);
				hidden.renderer.end_frame();
			}

			exit_unless(castingsSpent == 1, "the casting was not spent once");
			exit_unless(
				ctx.buffSystem->has_buff(target, BuffType::SILENCE) ==
					isOrcSilenced,
				isOrcSilenced ? "the orc was not silenced" :
								"the orc was silenced");
			exit_unless(
				target.has_state(ActorState::IS_SILENCED) == isOrcSilenced,
				isOrcSilenced ? "the orc can still cast" :
								"the orc cannot cast");
			finish_the_child();
		}

		[[noreturn]] void silence_silences_the_orc()
		{
			cast_at_the_orc("silence", true);
		}

		[[noreturn]] void web_does_not_silence_the_orc()
		{
			cast_at_the_orc("web", false);
		}
	} // namespace

	TEST(SilenceSpellTest, SilenceAtACreatureLeavesItUnableToCast)
	{
		const int childExitCode = exit_code_in_child(&silence_silences_the_orc);
		if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
		{
			GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
		}
		EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED);
	}

	// The same cursor and target with Web, the other targeted spell of its
	// level: no silence, so the silence above is Silence's own.
	TEST(SilenceSpellTest, WebAtTheSameCreatureLeavesItAbleToCast)
	{
		const int childExitCode =
			exit_code_in_child(&web_does_not_silence_the_orc);
		if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
		{
			GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
		}
		EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED);
	}

} // namespace CppRL
