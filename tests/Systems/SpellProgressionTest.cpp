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

// Wisdom 18 is used wherever a case is about the table's own reach rather than about
// the Wisdom gate: it satisfies both of Table 24's footnotes, so nothing is withheld.
class SpellProgressionTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		dataManager.load_all_data(messages);
	}

	static constexpr int UNGATED_WISDOM = 18;

	DataManager dataManager{};
	MessageSystem messages{};
};

TEST_F(SpellProgressionTest, TheWizardTableIsTableTwentyOneToTwentiethLevel)
{
	for (int level = 1; level <= 20; ++level)
	{
		EXPECT_EQ(SpellSystem::progression_slots(CasterClass::WIZARD, level),
			WIZARD_TABLE_21.at(static_cast<std::size_t>(level) - 1))
			<< "wizard level " << level;
	}
}

TEST_F(SpellProgressionTest, ThePriestTableIsTableTwentyFourToTwentiethLevel)
{
	for (int level = 1; level <= 20; ++level)
	{
		EXPECT_EQ(SpellSystem::progression_slots(CasterClass::CLERIC, level),
			PRIEST_TABLE_24.at(static_cast<std::size_t>(level) - 1))
			<< "priest level " << level;
	}
}

// The number a level-up announces is the number the table grants, so the two cannot
// come apart - which is the whole defect this replaces.
TEST_F(SpellProgressionTest, TheHighestSpellLevelIsWhatTheTableGrants)
{
	for (int level = 1; level <= 20; ++level)
	{
		EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, level, UNGATED_WISDOM, dataManager),
			static_cast<int>(WIZARD_TABLE_21.at(static_cast<std::size_t>(level) - 1).size()))
			<< "wizard level " << level;
		EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::CLERIC, level, UNGATED_WISDOM, dataManager),
			static_cast<int>(PRIEST_TABLE_24.at(static_cast<std::size_t>(level) - 1).size()))
			<< "priest level " << level;
	}
}

// The rows the old formula got wrong. (level + 1) / 2 promised a sixth spell level at
// 11th; Table 21 grants it at 12th, and the formula runs away from the table from
// there - a seventh at 13th where the book gives it at 14th, and so on.
TEST_F(SpellProgressionTest, AWizardReachesSixthLevelSpellsAtTwelfthNotEleventh)
{
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 10, UNGATED_WISDOM, dataManager), 5);
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 11, UNGATED_WISDOM, dataManager), 5) << "11th grants no new level";
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 12, UNGATED_WISDOM, dataManager), 6);
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 13, UNGATED_WISDOM, dataManager), 6) << "13th grants no new level";
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 14, UNGATED_WISDOM, dataManager), 7);
}

// A caster past the table's last row keeps what 20th level gave it rather than
// growing forever, and a non-caster has nothing at any level.
TEST_F(SpellProgressionTest, PastTwentiethTheTableHoldsAndANonCasterHasNothing)
{
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::WIZARD, 25, UNGATED_WISDOM, dataManager),
		SpellSystem::highest_spell_level(CasterClass::WIZARD, 20, UNGATED_WISDOM, dataManager));
	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::CLERIC, 25, UNGATED_WISDOM, dataManager),
		SpellSystem::highest_spell_level(CasterClass::CLERIC, 20, UNGATED_WISDOM, dataManager));

	EXPECT_EQ(SpellSystem::highest_spell_level(CasterClass::NONE, 10, UNGATED_WISDOM, dataManager), 0);
	EXPECT_TRUE(SpellSystem::progression_slots(CasterClass::NONE, 10).empty());
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
		return announced_level_at(creatureClass, newLevel, 18);
	}

	// Wisdom matters to a priest: Table 24's sixth and seventh rows are footnoted.
	int announced_level_at(CreatureClass creatureClass, int newLevel, int wisdom)
	{
		Creature caster{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "caster", ColorPairId::WHITE_BLACK } };
		caster.set_wisdom(wisdom);
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
				if (part.text.find("New spell level!") != std::string::npos)
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
				const std::string& text = part.text;
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
		caster.set_wisdom(18);
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
				if (part.text.find(fragment) != std::string::npos)
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

// A priest below the footnotes' Wisdom never reaches those rows, so it is never told
// about them however high its level goes.
TEST_F(SpellAnnouncementTest, APriestTooUnwiseForASpellLevelIsNotToldAboutIt)
{
	EXPECT_EQ(announced_level_at(CreatureClass::CLERIC, 11, 16), 0) << "the sixth needs Wisdom 17";
	EXPECT_EQ(announced_level_at(CreatureClass::CLERIC, 11, 17), 6);
	EXPECT_EQ(announced_level_at(CreatureClass::CLERIC, 14, 17), 0) << "the seventh needs Wisdom 18";
	EXPECT_EQ(announced_level_at(CreatureClass::CLERIC, 14, 18), 7);
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

// Table 5's Bonus Spells column, which is a list of spell levels and is cumulative:
// "a priest with a Wisdom of 15 is entitled to two 1st-level bonus spells and one
// 2nd-level bonus spell" (PHB, PDF page 36). The rows add 1st at 13, 1st at 14, 2nd
// at 15, 2nd at 16, 3rd at 17, 4th at 18, and 1st and 3rd at 19.
//
// The column used to be a single integer per row, which cannot say "1st, 3rd" - and
// row 19 held 2, which is neither the level it names nor a count of anything.
class PriestSpellsTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		dataManager.load_all_data(messages);
	}

	DataManager dataManager{};
	MessageSystem messages{};
};

// The book's own worked example is the case that can only pass if the rule is
// cumulative: at Wisdom 15 the two 1st-level bonuses come from rows 13 and 14.
TEST_F(PriestSpellsTest, WisdomFifteenGivesTwoFirstLevelBonusesAndOneSecond)
{
	const std::vector<int> bonus = SpellSystem::bonus_priest_spells(15, dataManager);

	ASSERT_GE(bonus.size(), 2u);
	EXPECT_EQ(bonus.at(0), 2) << "two 1st-level bonus spells";
	EXPECT_EQ(bonus.at(1), 1) << "one 2nd-level bonus spell";
}

TEST_F(PriestSpellsTest, BonusSpellsAccumulateRowByRow)
{
	EXPECT_TRUE(SpellSystem::bonus_priest_spells(12, dataManager).empty()) << "nothing below 13";
	EXPECT_EQ(SpellSystem::bonus_priest_spells(13, dataManager), (std::vector<int>{ 1 }));
	EXPECT_EQ(SpellSystem::bonus_priest_spells(14, dataManager), (std::vector<int>{ 2 }));
	EXPECT_EQ(SpellSystem::bonus_priest_spells(16, dataManager), (std::vector<int>{ 2, 2 }));
	EXPECT_EQ(SpellSystem::bonus_priest_spells(17, dataManager), (std::vector<int>{ 2, 2, 1 }));
	EXPECT_EQ(SpellSystem::bonus_priest_spells(18, dataManager), (std::vector<int>{ 2, 2, 1, 1 }));

	// Row 19 adds a 1st and a 3rd, which is the row a single integer could not carry.
	EXPECT_EQ(SpellSystem::bonus_priest_spells(19, dataManager), (std::vector<int>{ 3, 2, 2, 1 }));
}

// "these spells are available only when the priest is entitled to spells of the
// appropriate level": a 1st-level priest has one row of slots, so a 4th-level bonus
// has nowhere to go.
TEST_F(PriestSpellsTest, ABonusNeedsARowToLandOn)
{
	const std::vector<int> atFirst = SpellSystem::get_spell_slots(CasterClass::CLERIC, 1, 18, dataManager);

	ASSERT_EQ(atFirst.size(), 1u) << "1st level grants one row, whatever the Wisdom";
	EXPECT_EQ(atFirst.at(0), 1 + 2) << "one from Table 24, two 1st-level bonuses";
}

// Table 24's footnotes: the sixth column is usable only with Wisdom 17 or greater,
// the seventh only with 18 or greater.
TEST_F(PriestSpellsTest, TheSixthAndSeventhRowsNeedWisdomSeventeenAndEighteen)
{
	// 11th level is where Table 24 first prints a sixth-level slot.
	EXPECT_EQ(SpellSystem::get_spell_slots(CasterClass::CLERIC, 11, 16, dataManager).size(), 5u)
		<< "Wisdom 16 cannot reach the sixth";
	EXPECT_EQ(SpellSystem::get_spell_slots(CasterClass::CLERIC, 11, 17, dataManager).size(), 6u);

	// 14th is where the seventh first appears.
	EXPECT_EQ(SpellSystem::get_spell_slots(CasterClass::CLERIC, 14, 17, dataManager).size(), 6u)
		<< "Wisdom 17 cannot reach the seventh";
	EXPECT_EQ(SpellSystem::get_spell_slots(CasterClass::CLERIC, 14, 18, dataManager).size(), 7u);
}

// A wizard's spells are not a priest's: Table 5 says bonus spells are for "a priest
// (and only a priest)", and no footnote gates a wizard's rows.
TEST_F(PriestSpellsTest, AWizardTakesNoWisdomBonusAndNoWisdomGate)
{
	EXPECT_EQ(SpellSystem::get_spell_slots(CasterClass::WIZARD, 12, 18, dataManager),
		SpellSystem::progression_slots(CasterClass::WIZARD, 12));
	EXPECT_EQ(SpellSystem::get_spell_slots(CasterClass::WIZARD, 12, 9, dataManager),
		SpellSystem::progression_slots(CasterClass::WIZARD, 12));
}

// end of file: SpellProgressionTest.cpp
