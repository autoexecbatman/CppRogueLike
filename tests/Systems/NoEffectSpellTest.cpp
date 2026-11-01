// file: NoEffectSpellTest.cpp
// Casting a spell whose effect is NONE. The spell editor adds every custom
// spell with no effect, and casting does not refuse one, so the case is
// reachable from play: the caster is told the spell is not implemented and
// the casting is not spent, so the slot stays for a spell that does
// something.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=NoEffectSpellTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>

#include "src/ArmorClass.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/LogMessage.h"
#include "src/MessageSystem.h"
#include "src/Player.h"
#include "src/SpellRegistry.h"
#include "src/SpellSystem.h"
#include "tests/mocks/MockGameContext.h"

namespace CppRL
{

	class NoEffectSpellTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			mock.player = std::make_unique<Player>(Vector2D{ 3, 3 });
			mock.player->healthPool = std::make_unique<HealthPool>(20);
			mock.player->armorClass = std::make_unique<ArmorClass>(10);
			mock.player->experienceReward =
				std::make_unique<ExperienceReward>(0);
			mock.player->set_creature_level(3);
		}

		// Whether any stored message holds the given text, its coloured runs
		// joined.
		bool said_anything_about(std::string_view wanted) const
		{
			for (
				std::size_t index = 0;
				index < ctx.messageSystem->get_stored_message_count();
				++index)
			{
				std::string whole;
				for (
					const LogMessage& run :
					ctx.messageSystem->get_attack_message_at(index))
				{
					whole += run.text;
				}
				if (whole.find(wanted) != std::string::npos)
				{
					return true;
				}
			}
			return false;
		}

		MockGameContext mock{};
		GameContext ctx{ mock.to_game_context() };
	};

	TEST_F(NoEffectSpellTest, ASpellWithNoEffectSaysSoAndIsNotSpent)
	{
		const std::string key = ctx.spellRegistry->add_custom(
			SpellDefinition{ .name = "Blank Page" });
		ASSERT_EQ(
			ctx.spellRegistry->get_by_key(key).effect_type,
			SpellEffectType::NONE);
		int castingsSpent = 0;
		const auto count_casting = [&castingsSpent](GameContext& completedCtx)
		{
			++castingsSpent;
		};

		SpellSystem::cast_spell_by_key(
			key,
			*mock.player,
			SpellSource::MEMORIZED,
			count_casting,
			ctx);

		EXPECT_EQ(castingsSpent, 0) << "a spell that did nothing was spent";
		EXPECT_TRUE(said_anything_about("Spell not implemented yet."))
			<< "the caster was not told";
	}

} // namespace CppRL
