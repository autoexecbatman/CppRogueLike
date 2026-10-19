// file: InvisibilitySpellTest.cpp
// How long the wizard's Invisibility lasts, by the Player's Handbook (PDF page 291):
// "The spell remains in effect until it is magically broken or dispelled, until the
// wizard or recipient cancels it, until the recipient attacks any creature, or until 24
// hours have passed." Breaking on an attack is tested with the other attack breaks; no
// dispel or cancel exists in the game, so the day is what is tested here.
//
// The day is counted from the book's own figures, 24 hours of 60 rounds ("A round is
// approximately one minute long", PDF page 184), computed here rather than read from the
// constant the spell uses, so a wrong constant cannot agree with itself.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=InvisibilitySpellTest.*

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/GameContext.h"
#include "src/LogMessage.h"
#include "src/SpellSystem.h"
#include "src/TurnSchedule.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// A day in rounds, from the book: 24 hours, and a round of about one minute.
constexpr int HOURS_IN_A_DAY = 24;
constexpr int MINUTES_IN_AN_HOUR = 60;
constexpr int ROUNDS_IN_A_DAY = HOURS_IN_A_DAY * MINUTES_IN_AN_HOUR;
} // namespace

class InvisibilitySpellTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
	}

	// The wizard's own casting, from memory; only a priest's spell can fizzle.
	void cast_invisibility()
	{
		const auto casting_spent = [](GameContext& completedCtx) {
		};
		SpellSystem::cast_spell_by_key("invisibility", wizard, SpellSource::MEMORIZED, casting_spent, ctx);
	}

	// Whether the wizard is still invisible once the clock reads `rounds` after the casting.
	bool is_invisible_after(int rounds)
	{
		ctx.buffSystem->update_creature_buffs(wizard, expiry_time(0, rounds));
		return wizard.is_invisible();
	}

	// Whether any stored message holds the given text, its coloured runs joined.
	bool said(std::string_view wanted) const
	{
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			std::string whole;
			for (const LogMessage& run : ctx.messageSystem->get_attack_message_at(index))
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
	GameContext ctx{};
	Creature wizard{ Vector2D{ 2, 2 }, ActorData{ TileRef{}, "wizard", ColorPairId::WHITE_BLACK } };
};

// The game's old twenty rounds is a moment of the book's day.
TEST_F(InvisibilitySpellTest, ItOutlastsTheOldTwentyRounds)
{
	cast_invisibility();
	ASSERT_TRUE(wizard.is_invisible());

	EXPECT_TRUE(is_invisible_after(21));
}

// "Until 24 hours have passed": in force through the day's last round, over at its end.
TEST_F(InvisibilitySpellTest, ItLastsADayAndNoLonger)
{
	cast_invisibility();

	EXPECT_TRUE(is_invisible_after(ROUNDS_IN_A_DAY - 1)) << "gone before the day was out";
	EXPECT_FALSE(is_invisible_after(ROUNDS_IN_A_DAY)) << "still invisible after a full day";
}

// Cast from memory it is the spell's own invisibility, which no ring's removal ends.
TEST_F(InvisibilitySpellTest, CastFromMemoryItIsTheSpellsOwn)
{
	cast_invisibility();

	EXPECT_TRUE(ctx.buffSystem->has_buff(wizard, BuffType::INVISIBILITY));
	EXPECT_FALSE(ctx.buffSystem->has_buff(wizard, BuffType::ITEM_INVISIBILITY));
}

// The line the caster reads no longer promises the old twenty.
TEST_F(InvisibilitySpellTest, TheCastingDoesNotNameTheOldDuration)
{
	cast_invisibility();

	EXPECT_TRUE(said("You fade from view."));
	EXPECT_FALSE(said("20")) << "the message still names the old twenty";
}

// end of file: InvisibilitySpellTest.cpp
