// file: SpellProgressionTest.cpp
// The spell progression tables, and the one number a level-up may announce from them.
//
// Every row below is read off the Player's Handbook: Table 21, Wizard Spell
// Progression (PDF page 66), and Table 24, Priest Spell Progression (PDF page 72).
// Both run to 20th level. The game's tables stopped at 10th and clamped there, while
// the level-up message worked the castable spell level out for itself as
// (level + 1) / 2 - so from 11th a wizard was told it could cast spells the table
// would never hand it.
//
// highest_spell_level is what both now read, the same shape as the cleric's
// highest_turnable_hit_dice: one place that answers "what can this caster reach",
// asked rather than recomputed.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SpellProgressionTest.*

#include <gtest/gtest.h>

#include <array>
#include <vector>

#include <cctype>
#include <memory>
#include <string>
#include <string_view>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/DataManager.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/LevelUpSystem.h"
#include "src/MessageSystem.h"
#include "src/SpellSystem.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// Table 21, one row per wizard level from 1 to 20, in slots per spell level.
const std::vector<std::vector<int>> WIZARD_TABLE_21 = {
	{ 1 },
	{ 2 },
	{ 2, 1 },
	{ 3, 2 },
	{ 4, 2, 1 },
	{ 4, 2, 2 },
	{ 4, 3, 2, 1 },
	{ 4, 3, 3, 2 },
	{ 4, 3, 3, 2, 1 },
	{ 4, 4, 3, 2, 2 },
	{ 4, 4, 4, 3, 3 },
	{ 4, 4, 4, 4, 4, 1 },
	{ 5, 5, 5, 4, 4, 2 },
	{ 5, 5, 5, 4, 4, 2, 1 },
	{ 5, 5, 5, 5, 5, 2, 1 },
	{ 5, 5, 5, 5, 5, 3, 2, 1 },
	{ 5, 5, 5, 5, 5, 3, 3, 2 },
	{ 5, 5, 5, 5, 5, 3, 3, 2, 1 },
	{ 5, 5, 5, 5, 5, 3, 3, 3, 1 },
	{ 5, 5, 5, 5, 5, 4, 3, 3, 2 },
};

// Table 24, one row per priest level from 1 to 20.
const std::vector<std::vector<int>> PRIEST_TABLE_24 = {
	{ 1 },
	{ 2 },
	{ 2, 1 },
	{ 3, 2 },
	{ 3, 3, 1 },
	{ 3, 3, 2 },
	{ 3, 3, 2, 1 },
	{ 3, 3, 3, 2 },
	{ 4, 4, 3, 2, 1 },
	{ 4, 4, 3, 3, 2 },
	{ 5, 4, 4, 3, 2, 1 },
	{ 6, 5, 5, 3, 2, 2 },
	{ 6, 6, 6, 4, 2, 2 },
	{ 6, 6, 6, 5, 3, 2, 1 },
	{ 6, 6, 6, 6, 4, 2, 1 },
	{ 7, 7, 7, 6, 4, 3, 1 },
	{ 7, 7, 7, 7, 5, 3, 2 },
	{ 8, 8, 8, 8, 6, 4, 2 },
	{ 9, 9, 8, 8, 6, 4, 2 },
	{ 9, 9, 9, 8, 7, 5, 2 },
};
} // namespace

TEST(SpellProgressionTest, TheWizardTableIsTableTwentyOneToTwentiethLevel)
{
	for (int level = 1; level <= 20; ++level)
	{
		EXPECT_EQ(SpellSystem::get_spell_slots(CasterClass::WIZARD, level),
			WIZARD_TABLE_21.at(static_cast<std::size_t>(level) - 1))
			<< "wizard level " << level;
	}
}

TEST(SpellProgressionTest, ThePriestTableIsTableTwentyFourToTwentiethLevel)
{
	for (int level = 1; level <= 20; ++level)
	{
		EXPECT_EQ(SpellSystem::get_spell_slots(CasterClass::CLERIC, level),
			PRIEST_TABLE_24.at(static_cast<std::size_t>(level) - 1))
			<< "priest level " << level;
	}
}

// The number a level-up announces is the number the table grants, so the two cannot
// come apart - which is the whole defect this replaces.
TEST(SpellProgressionTest, TheHighestSpellLevelIsWhatTheTableGrants)
{
	for (int level = 1; level <= 20; ++level)
	{
		EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, level),
			static_cast<int>(WIZARD_TABLE_21.at(static_cast<std::size_t>(level) - 1).size()))
			<< "wizard level " << level;
		EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::CLERIC, level),
			static_cast<int>(PRIEST_TABLE_24.at(static_cast<std::size_t>(level) - 1).size()))
			<< "priest level " << level;
	}
}

// The rows the old formula got wrong. (level + 1) / 2 promised a sixth spell level at
// 11th; Table 21 grants it at 12th, and the formula runs away from the table from
// there - a seventh at 13th where the book gives it at 14th, and so on.
TEST(SpellProgressionTest, AWizardReachesSixthLevelSpellsAtTwelfthNotEleventh)
{
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 10), 5);
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 11), 5) << "11th grants no new level";
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 12), 6);
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 13), 6) << "13th grants no new level";
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 14), 7);
}

// A caster past the table's last row keeps what 20th level gave it rather than
// growing forever, and a non-caster has nothing at any level.
TEST(SpellProgressionTest, PastTwentiethTheTableHoldsAndANonCasterHasNothing)
{
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 25),
		SpellSystem::highest_spell_level(CasterClass::WIZARD, 20));
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::CLERIC, 25),
		SpellSystem::highest_spell_level(CasterClass::CLERIC, 20));

	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::NONE, 10), 0);
	EXPECT_TRUE(SpellSystem::get_spell_slots(CasterClass::NONE, 10).empty());
}

// The announcement itself: a level-up says so only when the table opened something.
class SpellAnnouncementTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.dataManager = &dataManager;
		dataManager.load_all_data(messages);
	}

	// Levels a fresh caster and reports what spell level it was told it had reached,
	// or 0 for no such message.
	int announced_level_at(CreatureClass creatureClass, int newLevel)
	{
		Creature caster{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "caster", ColorPairId::WHITE_BLACK } };
		caster.healthPool = std::make_unique<HealthPool>(50);
		caster.armorClass = std::make_unique<ArmorClass>(10);
		caster.experienceReward = std::make_unique<ExperienceReward>(0);
		caster.set_creature_class(creatureClass);
		caster.set_creature_level(newLevel);

		const std::size_t before = mock.messages.get_stored_message_count();
		LevelUpSystem::apply_level_up_benefits(caster, newLevel, &ctx);

		for (std::size_t index = before; index < mock.messages.get_stored_message_count(); ++index)
		{
			const auto& parts = mock.messages.get_attack_message_at(index);
			bool isSpellMessage = false;
			for (const auto& part : parts)
			{
				if (part.logMessageText.find("New spell level!") != std::string::npos)
				{
					isSpellMessage = true;
				}
			}
			if (!isSpellMessage)
			{
				continue;
			}
			for (const auto& part : parts)
			{
				const std::string& text = part.logMessageText;
				if (!text.empty() && std::isdigit(static_cast<unsigned char>(text.front())))
				{
					return std::stoi(text);
				}
			}
		}
		return 0;
	}

	// Whether any message this level-up produced contains that text.
	bool said(CreatureClass creatureClass, int newLevel, std::string_view fragment)
	{
		Creature caster{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "caster", ColorPairId::WHITE_BLACK } };
		caster.healthPool = std::make_unique<HealthPool>(50);
		caster.armorClass = std::make_unique<ArmorClass>(10);
		caster.experienceReward = std::make_unique<ExperienceReward>(0);
		caster.set_creature_class(creatureClass);
		caster.set_creature_level(newLevel);

		const std::size_t before = mock.messages.get_stored_message_count();
		LevelUpSystem::apply_level_up_benefits(caster, newLevel, &ctx);

		for (std::size_t index = before; index < mock.messages.get_stored_message_count(); ++index)
		{
			for (const auto& part : mock.messages.get_attack_message_at(index))
			{
				if (part.logMessageText.find(fragment) != std::string::npos)
				{
					return true;
				}
			}
		}
		return false;
	}

	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
	MessageSystem messages{};
};

// Table 21 opens a sixth spell level at 12th, and nothing at 11th or 13th. The
// formula this replaced announced one at every odd level for ever.
TEST_F(SpellAnnouncementTest, AWizardIsToldOnlyWhenTheTableOpensALevel)
{
	EXPECT_EQ(announced_level_at(CreatureClass::WIZARD, 11), 0) << "11th opens nothing";
	EXPECT_EQ(announced_level_at(CreatureClass::WIZARD, 12), 6);
	EXPECT_EQ(announced_level_at(CreatureClass::WIZARD, 13), 0) << "13th opens nothing";
	EXPECT_EQ(announced_level_at(CreatureClass::WIZARD, 14), 7);
}

// The cleric gains spell levels on the same terms and was never told at all.
TEST_F(SpellAnnouncementTest, AClericIsToldOnTheLevelsTableTwentyFourOpens)
{
	EXPECT_EQ(announced_level_at(CreatureClass::CLERIC, 11), 6);
	EXPECT_EQ(announced_level_at(CreatureClass::CLERIC, 12), 0) << "12th opens nothing";
	EXPECT_EQ(announced_level_at(CreatureClass::CLERIC, 14), 7);
}

// A fighter has no spell table, so it is never told about one - and the levels that
// used to carry "Your martial prowess improves!" now carry nothing of their own.
TEST_F(SpellAnnouncementTest, AFighterIsNeverToldAboutSpells)
{
	for (const int level : { 3, 6, 9, 12 })
	{
		EXPECT_EQ(announced_level_at(CreatureClass::FIGHTER, level), 0) << "level " << level;
	}
}

// Three lines used to fire on schedules of their own - every third fighter level,
// every cleric level from 2nd, every wizard level - and claimed an improvement that
// had not happened. Their real gains announce themselves where they are applied.
TEST_F(SpellAnnouncementTest, NoLevelClaimsAnImprovementThatDidNotHappen)
{
	for (const int level : { 2, 3, 6, 9, 12 })
	{
		EXPECT_FALSE(said(CreatureClass::FIGHTER, level, "martial prowess")) << "level " << level;
		EXPECT_FALSE(said(CreatureClass::CLERIC, level, "divine power")) << "level " << level;
		EXPECT_FALSE(said(CreatureClass::WIZARD, level, "arcane knowledge")) << "level " << level;
	}
}
// end of file: SpellProgressionTest.cpp
