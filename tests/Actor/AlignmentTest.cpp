// file: AlignmentTest.cpp
// Verifies that alignment is two independent axes and that monsters authored
// before alignment existed load as true neutral rather than failing.

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "src/Actor/Alignment.h"
#include "src/Actor/Creature.h"
#include "src/Factories/MonsterCreator.h"
#include "src/Utils/Vector2D.h"

class AlignmentTest : public ::testing::Test
{
protected:
	Creature creature{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "goblin", 0 } };
};

// Nothing is aligned until its data says so.
TEST_F(AlignmentTest, DefaultsToTrueNeutral)
{
	EXPECT_EQ(creature.get_ethics(), Ethics::NEUTRAL);
	EXPECT_EQ(creature.get_morality(), Morality::NEUTRAL);
	EXPECT_FALSE(creature.is_evil());
}

// The axes are independent: setting one must not disturb the other.
TEST_F(AlignmentTest, AxesAreIndependent)
{
	creature.set_ethics(Ethics::CHAOTIC);

	EXPECT_EQ(creature.get_ethics(), Ethics::CHAOTIC);
	EXPECT_EQ(creature.get_morality(), Morality::NEUTRAL) << "morality moved with ethics";

	creature.set_morality(Morality::GOOD);

	EXPECT_EQ(creature.get_ethics(), Ethics::CHAOTIC) << "ethics moved with morality";
	EXPECT_EQ(creature.get_morality(), Morality::GOOD);
}

// is_evil reads only the morality axis, which is what alignment-keyed effects
// such as Protection from Evil test.
TEST_F(AlignmentTest, IsEvilReadsMoralityAlone)
{
	creature.set_ethics(Ethics::LAWFUL);
	creature.set_morality(Morality::EVIL);
	EXPECT_TRUE(creature.is_evil()) << "lawful evil is still evil";

	creature.set_ethics(Ethics::CHAOTIC);
	EXPECT_TRUE(creature.is_evil()) << "ethics must not affect is_evil";

	creature.set_morality(Morality::NEUTRAL);
	EXPECT_FALSE(creature.is_evil());
}

// Entries authored before alignment existed must still load, defaulting to true
// neutral. The 43 monsters in monsters.json carry no alignment keys: the
// Monstrous Manual is not on hand, so no per-monster alignment is asserted yet.
TEST(AlignmentDataTest, MonstersWithoutAlignmentKeysLoadAsTrueNeutral)
{
	MonsterCreator::load("data/content/monsters.json");

	const MonsterParams& goblin = MonsterCreator::get_params("goblin");

	EXPECT_EQ(goblin.ethics, Ethics::NEUTRAL);
	EXPECT_EQ(goblin.morality, Morality::NEUTRAL);
}

// The JSON stores alignment as the enum's integer value, so the ordering is part
// of the data format: reordering either enum would silently change the meaning of
// every authored monster. Saves are not kept compatible in this project, but
// data/content/monsters.json is authored content, not a save.
TEST(AlignmentDataTest, EnumOrderingIsPartOfTheDataFormat)
{
	EXPECT_EQ(static_cast<int>(Ethics::LAWFUL), 0);
	EXPECT_EQ(static_cast<int>(Ethics::NEUTRAL), 1);
	EXPECT_EQ(static_cast<int>(Ethics::CHAOTIC), 2);

	EXPECT_EQ(static_cast<int>(Morality::GOOD), 0);
	EXPECT_EQ(static_cast<int>(Morality::NEUTRAL), 1);
	EXPECT_EQ(static_cast<int>(Morality::EVIL), 2);
}
