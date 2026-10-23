// file: SpellcastingTest.cpp
// What a creature casts from, and the one rule that fills it for the player and a monster.
//
// The book these are derived from, PDF pages of the local archive:
// - PHB 165: "Upon casting, the energy of the spell is spent", and "he can memorize the same
//   spell more than once, but each memorization counts as one spell".
// - Table 21 (PHB 66), the wizard's slots: 1st level 1; 18th level 5 5 5 5 5 3 3 2 1.
// - Table 24 (PHB 72), the priest's: 5th level 3 3 1. Table 5's bonus spells at Wisdom 18 are
//   two 1st, two 2nd, one 3rd and one 4th, and a 5th-level priest has no 4th-level row.
// - The fill takes each spell once per level in the registry's order: cure light wounds,
//   bless, sanctuary, protection from evil, hold person, silence; magic missile, shield,
//   sleep, invisibility, web, fireball, teleport, knock. Spell levels are spells.json's.
// - Monstrous Manual: a lich "originally wizards of at least 18th level" (1634); a pit fiend
//   uses fireball "Once per round" (Baatezu, 1103); the mage has no entry and is a 1st-level
//   wizard by its record.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=SpellcastingTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/ArmorClass.h"
#include "src/Controls.h"
#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/InputSystem.h"
#include "src/MenuManager.h"
#include "src/MenuSpellCast.h"
#include "src/MonsterCreator.h"
#include "src/MonsterRegistry.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "src/SpellSystem.h"
#include "src/Spellcasting.h"
#include "src/TurnSchedule.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// One day of the clock in time units, the helm's recharge.
constexpr int DAY_IN_TIME_UNITS = ROUNDS_PER_DAY * TIME_UNITS_PER_ROUND;

using Keys = std::vector<std::string>;
} // namespace

class SpellcastingTest : public ::testing::Test
{
protected:
	void SetUp() override { ctx = mock.to_game_context(); }

	void TearDown() override { std::filesystem::remove(edited); }

	// The real monsters.json with one record changed by edit, written beside the temp files.
	template <typename Edit>
	void write_edited_monsters(Edit edit)
	{
		std::ifstream source(Paths::resolve(Paths::MONSTERS));
		nlohmann::json root = nlohmann::json::parse(source);
		edit(root);
		std::ofstream out(edited);
		out << root.dump();
	}

	std::unique_ptr<Creature> build(const std::string& key)
	{
		return MonsterCreator::create_from_params(
			Vector2D{ 0, 0 },
			mock.monsterRegistry.get_params(key),
			ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::filesystem::path edited{ std::filesystem::temp_directory_path() /
		"monsters_spellcasting.json" };
};

TEST_F(
	SpellcastingTest,
	FrequenciesAndCasterClassesAreWrittenByNameAndUnknownNamesAreRefused)
{
	EXPECT_EQ(encode_power_frequency(PowerFrequency::AT_WILL), "at_will");
	EXPECT_EQ(
		encode_power_frequency(PowerFrequency::ONCE_PER_DAY),
		"once_per_day");
	EXPECT_EQ(parse_power_frequency("at_will"), PowerFrequency::AT_WILL);
	EXPECT_EQ(
		parse_power_frequency("once_per_day"),
		PowerFrequency::ONCE_PER_DAY);
	EXPECT_THROW((void)parse_power_frequency("weekly"), std::runtime_error);

	EXPECT_EQ(encode_caster_class(CasterClass::WIZARD), "wizard");
	EXPECT_EQ(encode_caster_class(CasterClass::CLERIC), "cleric");
	EXPECT_EQ(parse_caster_class("wizard"), CasterClass::WIZARD);
	EXPECT_EQ(parse_caster_class("cleric"), CasterClass::CLERIC);
	EXPECT_THROW((void)parse_caster_class("none"), std::runtime_error);
	EXPECT_THROW((void)parse_caster_class("bard"), std::runtime_error);
}

// Each memorization is one casting, so casting one of two copies leaves the other.
TEST_F(SpellcastingTest, SpendingASpellRemovesOneCopy)
{
	Spellcasting lich{};
	lich.memorizedSpells = { "magic_missile", "fireball", "magic_missile" };

	spend_memorized(lich, "magic_missile");

	EXPECT_EQ(lich.memorizedSpells, (Keys{ "fireball", "magic_missile" }));
}

// An at-will power is never spent; a once-a-day power is ready again a day after its use,
// and not a time unit before.
TEST_F(SpellcastingTest, AnAtWillPowerIsNeverSpentAndADailyOneWaitsADay)
{
	InnatePower atWill{ "fireball", PowerFrequency::AT_WILL, 0 };
	record_power_use(atWill, 600);
	EXPECT_TRUE(is_power_ready(atWill, 600));
	EXPECT_EQ(atWill.readyAgainAt, 0);

	InnatePower daily{ "fireball", PowerFrequency::ONCE_PER_DAY, 0 };
	EXPECT_TRUE(is_power_ready(daily, 600)) << "a daily power starts ready";
	record_power_use(daily, 600);
	EXPECT_FALSE(is_power_ready(daily, 600));
	EXPECT_FALSE(is_power_ready(daily, 600 + DAY_IN_TIME_UNITS - 1));
	EXPECT_TRUE(is_power_ready(daily, 600 + DAY_IN_TIME_UNITS));
}

TEST_F(
	SpellcastingTest,
	ASaveKeepsTheMemorizedCopiesTheTrainingAndEachPowersClock)
{
	Spellcasting original{};
	original.memorizedSpells = { "fireball", "fireball" };
	original.training = SpellcasterTraining{ CasterClass::WIZARD, 18 };
	original.innatePowers = {
		InnatePower{ "fireball", PowerFrequency::ONCE_PER_DAY, 5000 }
	};

	const Spellcasting loaded =
		parse_spellcasting(encode_spellcasting(original));

	EXPECT_EQ(loaded.memorizedSpells, original.memorizedSpells);
	ASSERT_TRUE(loaded.training.has_value());
	EXPECT_EQ(loaded.training->casterClass, CasterClass::WIZARD);
	EXPECT_EQ(loaded.training->casterLevel, 18);
	ASSERT_EQ(loaded.innatePowers.size(), 1u);
	EXPECT_EQ(loaded.innatePowers.front().spell, "fireball");
	EXPECT_EQ(
		loaded.innatePowers.front().frequency,
		PowerFrequency::ONCE_PER_DAY);
	EXPECT_EQ(loaded.innatePowers.front().readyAgainAt, 5000);

	const Spellcasting none =
		parse_spellcasting(encode_spellcasting(Spellcasting{}));
	EXPECT_TRUE(none.memorizedSpells.empty());
	EXPECT_FALSE(none.training.has_value());
	EXPECT_TRUE(none.innatePowers.empty());
}

TEST_F(SpellcastingTest, ASaveMissingAnyPartIsRefused)
{
	const nlohmann::json whole = encode_spellcasting(Spellcasting{});
	for (const char* field : { "memorizedSpells", "training", "innatePowers" })
	{
		nlohmann::json damaged = whole;
		damaged.erase(field);
		EXPECT_ANY_THROW((void)parse_spellcasting(damaged))
			<< "a save without " << field << " loaded";
	}
}

// Table 21 and today's fill: one slot, the first 1st-level wizard spell.
TEST_F(SpellcastingTest, AFirstLevelWizardMemorizesMagicMissile)
{
	EXPECT_EQ(
		SpellSystem::spells_to_memorize(
			CasterClass::WIZARD,
			1,
			10,
			mock.spellRegistry,
			mock.data_manager),
		(Keys{ "magic_missile" }));
}

// An 18th-level wizard has more slots at every level than the game has spells, so each of
// the eight is memorized once, lowest level first.
TEST_F(SpellcastingTest, AnEighteenthLevelWizardMemorizesEveryWizardSpellOnce)
{
	EXPECT_EQ(
		SpellSystem::spells_to_memorize(
			CasterClass::WIZARD,
			18,
			10,
			mock.spellRegistry,
			mock.data_manager),
		(Keys{
			"magic_missile",
			"shield",
			"sleep",
			"invisibility",
			"web",
			"knock",
			"fireball",
			"teleport",
		}));
}

// A 5th-level priest has three 1st-level slots, so it holds three of the four 1st-level spells;
// Wisdom 18's two bonus 1st-level slots reach the fourth.
TEST_F(SpellcastingTest, APriestsWisdomAddsTheBonusSlotsToTheFill)
{
	EXPECT_EQ(
		SpellSystem::spells_to_memorize(
			CasterClass::CLERIC,
			5,
			10,
			mock.spellRegistry,
			mock.data_manager),
		(Keys{
			"cure_light_wounds",
			"bless",
			"sanctuary",
			"hold_person",
			"silence",
		}));
	EXPECT_EQ(
		SpellSystem::spells_to_memorize(
			CasterClass::CLERIC,
			5,
			18,
			mock.spellRegistry,
			mock.data_manager),
		(Keys{
			"cure_light_wounds",
			"bless",
			"sanctuary",
			"protection_from_evil",
			"hold_person",
			"silence",
		}));
	EXPECT_TRUE(
		SpellSystem::spells_to_memorize(
			CasterClass::NONE,
			9,
			18,
			mock.spellRegistry,
			mock.data_manager)
			.empty());
}

// The player's class and level are its own; a monster's training overrides its Hit Dice.
TEST_F(
	SpellcastingTest,
	ACasterCastsAtItsTrainingWhenItHasOneAndItsLevelOtherwise)
{
	Player wizard{ Vector2D{ 1, 1 } };
	wizard.set_creature_class(CreatureClass::WIZARD);
	wizard.set_creature_level(7);
	EXPECT_EQ(SpellSystem::caster_class(wizard), CasterClass::WIZARD);
	EXPECT_EQ(SpellSystem::caster_level(wizard), 7);

	Creature lich{
		Vector2D{ 2, 2 },
		ActorData{ TileRef{}, "lich", ColorPairId::WHITE_BLACK },
	};
	lich.set_creature_level(11);
	lich.spellcasting.training = SpellcasterTraining{ CasterClass::WIZARD, 18 };
	EXPECT_EQ(SpellSystem::caster_class(lich), CasterClass::WIZARD);
	EXPECT_EQ(SpellSystem::caster_level(lich), 18);

	Creature pitFiend{
		Vector2D{ 3, 3 },
		ActorData{ TileRef{}, "pit fiend", ColorPairId::WHITE_BLACK },
	};
	pitFiend.set_creature_level(13);
	EXPECT_EQ(SpellSystem::caster_class(pitFiend), CasterClass::NONE);
	EXPECT_EQ(SpellSystem::caster_level(pitFiend), 13);
}

// Table 5's fizzle is a priest's, and a monster trained as a cleric is one: Wisdom 9 is 20
// percent, as it is for the player's priest. A wizard-trained monster risks none.
TEST_F(SpellcastingTest, AClericTrainedMonsterFizzlesByTableFive)
{
	Creature shaman{
		Vector2D{ 2, 2 },
		ActorData{ TileRef{}, "shaman", ColorPairId::WHITE_BLACK },
	};
	shaman.set_wisdom(9);
	shaman.spellcasting.training =
		SpellcasterTraining{ CasterClass::CLERIC, 5 };
	EXPECT_EQ(SpellSystem::spell_failure_chance(shaman, mock.data_manager), 20);

	shaman.spellcasting.training =
		SpellcasterTraining{ CasterClass::WIZARD, 5 };
	EXPECT_EQ(SpellSystem::spell_failure_chance(shaman, mock.data_manager), 0);
}

TEST_F(SpellcastingTest, MemorizingWritesTheFillForTheCreaturesOwnClassAndLevel)
{
	Player wizard{ Vector2D{ 1, 1 } };
	wizard.set_creature_class(CreatureClass::WIZARD);
	wizard.set_creature_level(1);
	wizard.spellcasting.memorizedSpells = { "teleport" };

	SpellSystem::memorize_spells(wizard, mock.spellRegistry, mock.data_manager);

	EXPECT_EQ(wizard.spellcasting.memorizedSpells, (Keys{ "magic_missile" }));
}

// The fill reads the caster level: a creature of level 5 trained as a 1st-level wizard has
// Table 21's one slot, where its own level would give it seven spells.
TEST_F(SpellcastingTest, AMonsterMemorizesAtItsTrainingLevelAndNotItsHitDice)
{
	Creature conjurer{
		Vector2D{ 2, 2 },
		ActorData{ TileRef{}, "conjurer", ColorPairId::WHITE_BLACK },
	};
	conjurer.set_creature_level(5);
	conjurer.spellcasting.training =
		SpellcasterTraining{ CasterClass::WIZARD, 1 };

	SpellSystem::memorize_spells(
		conjurer,
		mock.spellRegistry,
		mock.data_manager);

	EXPECT_EQ(conjurer.spellcasting.memorizedSpells, (Keys{ "magic_missile" }));
}

TEST_F(SpellcastingTest, TheRecordsStateTheLichTheMageAndThePitFiend)
{
	const MonsterParams& lich = mock.monsterRegistry.get_params("lich");
	ASSERT_TRUE(lich.spellcasterTraining.has_value());
	EXPECT_EQ(lich.spellcasterTraining->casterClass, CasterClass::WIZARD);
	EXPECT_EQ(lich.spellcasterTraining->casterLevel, 18);
	EXPECT_TRUE(lich.innatePowers.empty());

	const MonsterParams& mage = mock.monsterRegistry.get_params("mage");
	ASSERT_TRUE(mage.spellcasterTraining.has_value());
	EXPECT_EQ(mage.spellcasterTraining->casterClass, CasterClass::WIZARD);
	EXPECT_EQ(mage.spellcasterTraining->casterLevel, 1);

	const MonsterParams& pitFiend =
		mock.monsterRegistry.get_params("pit_fiend");
	EXPECT_FALSE(pitFiend.spellcasterTraining.has_value());
	ASSERT_EQ(pitFiend.innatePowers.size(), 1u);
	EXPECT_EQ(pitFiend.innatePowers.front().spell, "fireball");
	EXPECT_EQ(pitFiend.innatePowers.front().frequency, PowerFrequency::AT_WILL);

	const MonsterParams& goblin = mock.monsterRegistry.get_params("goblin");
	EXPECT_FALSE(goblin.spellcasterTraining.has_value());
	EXPECT_TRUE(goblin.innatePowers.empty());
}

// A monster is built with its record's casting, and memorizes by the player's fill at spawn.
TEST_F(SpellcastingTest, AMonsterIsBuiltWithItsCastingAndMemorizesAtSpawn)
{
	const std::unique_ptr<Creature> lich = build("lich");
	EXPECT_EQ(SpellSystem::caster_level(*lich), 18);
	EXPECT_EQ(
		lich->spellcasting.memorizedSpells,
		(Keys{
			"magic_missile",
			"shield",
			"sleep",
			"invisibility",
			"web",
			"knock",
			"fireball",
			"teleport",
		}));

	const std::unique_ptr<Creature> pitFiend = build("pit_fiend");
	EXPECT_TRUE(pitFiend->spellcasting.memorizedSpells.empty());
	ASSERT_EQ(pitFiend->spellcasting.innatePowers.size(), 1u);
	EXPECT_EQ(pitFiend->spellcasting.innatePowers.front().spell, "fireball");

	const std::unique_ptr<Creature> goblin = build("goblin");
	EXPECT_TRUE(goblin->spellcasting.memorizedSpells.empty());
	EXPECT_TRUE(goblin->spellcasting.innatePowers.empty());
}

TEST_F(SpellcastingTest, ARecordMissingItsCastingFieldsIsRefused)
{
	MonsterRegistry monsters{};
	for (const char* field : { "spellcasting", "innate_powers" })
	{
		write_edited_monsters(
			[field](nlohmann::json& root) { root.at("goblin").erase(field); });
		EXPECT_ANY_THROW(monsters.load(edited.string()))
			<< "a goblin with no " << field << " loaded";
	}
}

TEST_F(SpellcastingTest, ARecordNamingAnUnknownFrequencyOrCasterClassIsRefused)
{
	MonsterRegistry monsters{};
	write_edited_monsters(
		[](nlohmann::json& root)
		{
			root.at("pit_fiend").at("innate_powers").at(0)["frequency"] =
				"weekly";
		});
	EXPECT_THROW(monsters.load(edited.string()), std::runtime_error);

	write_edited_monsters([](nlohmann::json& root)
		{ root.at("lich").at("spellcasting")["caster_class"] = "none"; });
	EXPECT_THROW(monsters.load(edited.string()), std::runtime_error);
}

// Every power in the shipped data names a spell the game has; a renamed one is refused.
TEST_F(
	SpellcastingTest,
	APowerNamingAnUnknownSpellIsRefusedOnceBothRegistriesLoad)
{
	EXPECT_NO_THROW(
		mock.monsterRegistry.refuse_unknown_spells(mock.spellRegistry));

	MonsterRegistry monsters{};
	write_edited_monsters(
		[](nlohmann::json& root)
		{
			root.at("pit_fiend").at("innate_powers").at(0)["spell"] =
				"hellfire";
		});
	monsters.load(edited.string());
	EXPECT_THROW(
		monsters.refuse_unknown_spells(mock.spellRegistry),
		std::runtime_error);
}

// The creature's save carries its casting, so a spent copy stays spent across a load.
TEST_F(SpellcastingTest, ACreatureSavesItsCasting)
{
	Creature lich{
		Vector2D{ 2, 2 },
		ActorData{ TileRef{}, "lich", ColorPairId::WHITE_BLACK },
	};
	lich.healthPool = std::make_unique<HealthPool>(5);
	lich.armorClass = std::make_unique<ArmorClass>(10);
	lich.spellcasting.memorizedSpells = { "fireball" };
	lich.spellcasting.training = SpellcasterTraining{ CasterClass::WIZARD, 18 };
	nlohmann::json saved;
	lich.save(saved);

	Creature loaded{
		Vector2D{ 0, 0 },
		ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK },
	};
	loaded.load(saved);

	EXPECT_EQ(loaded.spellcasting.memorizedSpells, (Keys{ "fireball" }));
	ASSERT_TRUE(loaded.spellcasting.training.has_value());
	EXPECT_EQ(loaded.spellcasting.training->casterLevel, 18);
}

namespace
{

// Casting one of two memorized shields from the cast menu leaves the other.
[[noreturn]] void casting_from_the_menu_spends_one_copy()
{
	HiddenRenderer hidden{};
	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	ctx.renderer = &hidden.renderer;
	mock.player = std::make_unique<Player>(Vector2D{ 2, 2 });
	Player& wizard = *mock.player;
	wizard.experienceReward = std::make_unique<ExperienceReward>(0);
	wizard.armorClass = std::make_unique<ArmorClass>(10);
	wizard.healthPool = std::make_unique<HealthPool>(20);
	ctx.playerOwner = &mock.player;
	wizard.spellcasting.memorizedSpells = { "shield", "shield" };

	mock.menus.push_back(std::make_unique<MenuSpellCast>(wizard, ctx));
	ctx.inputSystem->queue_key(GameKey::NONE, 'a');
	hidden.renderer.begin_frame();
	ctx.menuManager->handle_menus(*ctx.menus, ctx);
	hidden.renderer.end_frame();

	exit_unless(
		wizard.spellcasting.memorizedSpells == Keys{ "shield" },
		"casting one shield did not leave exactly the other");
	finish_the_child();
}

} // namespace

TEST(SpellcastingMenuTest, CastingFromTheMenuSpendsOneCopy)
{
	const int childExitCode =
		exit_code_in_child(&casting_from_the_menu_spends_one_copy);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

// end of file: SpellcastingTest.cpp
