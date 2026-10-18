// file: DragonStatBlockTest.cpp
// The dragon in monsters.json is a juvenile red dragon, age category 4, and every
// number its record carries is the Monstrous Manual's for that age. Page numbers are
// the local archive's as a PDF viewer shows them, counting from 1 - one above the
// 0-based index pypdf reports.
//
// Where each value comes from:
//   - Hit Dice 15, AC -3 base: Red Dragon, p1240. Hit Dice are 8-sided unless the
//     entry says otherwise (Monstrous Manual introduction, p1083), and the Dragon Table
//     (p1227) gives a juvenile a hit die modifier of Nil, so 15d8 with no bonus.
//   - AC -3 and XP 12,000: the juvenile row of the red dragon's age table, p1242.
//   - THAC0 5: the Dungeon Master's Guide's Table 39, Creature THAC0 (p692), at 15 Hit
//     Dice. The entry prints "THAC0: 7 (at 9 HD)", but Table 39 gives 11 at 9 HD and 7
//     at 13 HD, so that line matches no row; at the dragon's own 15 HD the table gives 5.
//   - Bite 3d10+4: the entry's third attack, 3-30 (3d10), plus the juvenile's combat
//     modifier of +4 from the Dragon Table, which is added to each physical attack.
//   - Morale 18: the entry prints Fanatic (17-18); 18 is inside that band.
//   - Gargantuan: Size G, p1240. Depth 9, beside the pit fiend, the strongest creatures
//     in the game.
//
// What the record does not model, and so a dragon here does not do: its breath weapon
// (8d10+4 fire, the dragon's defining attack - issues/0206), the claw/claw pair, its
// one wizard spell and the innate powers of its age, magic resistance, immunity to
// fire, and the snatch and plummet attacks of a flying dragon.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=DragonStatBlockTest.*

#include <gtest/gtest.h>

#include "src/CreatureSize.h"
#include "src/DamageInfo.h"
#include "src/MonsterRegistry.h"
#include "src/Paths.h"

namespace
{
// The dragon's record as the monster editor reads it from the data file.
MonsterParams dragon_from_data()
{
	MonsterRegistry monsters{};
	monsters.load(Paths::MONSTERS);
	return monsters.get_params("dragon");
}
} // namespace

// 15 Hit Dice of eight sides, with nothing added: a juvenile's modifier is Nil.
TEST(DragonStatBlockTest, AJuvenileHasFifteenEightSidedHitDice)
{
	const MonsterParams dragon = dragon_from_data();

	ASSERT_EQ(dragon.hitDice.counts.size(), 1u);
	EXPECT_EQ(dragon.hitDice.counts.front().count, 15);
	EXPECT_EQ(dragon.hitDice.sides, 8);
	EXPECT_EQ(dragon.hitDice.bonus, 0);
}

// Table 39 at 15 Hit Dice, not the entry's "7 (at 9 HD)".
TEST(DragonStatBlockTest, ItsThac0IsTableThirtyNineAtFifteenHitDice)
{
	EXPECT_EQ(dragon_from_data().thaco, 5);
}

TEST(DragonStatBlockTest, AJuvenileHasArmorClassMinusThree)
{
	EXPECT_EQ(dragon_from_data().ac, -3);
}

TEST(DragonStatBlockTest, AJuvenileIsWorthTwelveThousandExperience)
{
	EXPECT_EQ(highest_count(dragon_from_data().hitDice).xp, 12000);
}

// Fanatic is 17-18.
TEST(DragonStatBlockTest, ItsMoraleIsFanatic)
{
	EXPECT_EQ(dragon_from_data().morale, 18);
}

TEST(DragonStatBlockTest, ItIsGargantuan)
{
	EXPECT_EQ(dragon_from_data().size, CreatureSize::GARGANTUAN);
}

// First drawn on the ninth level, and on every level below it.
TEST(DragonStatBlockTest, ItIsFirstMetOnTheNinthLevel)
{
	const MonsterParams dragon = dragon_from_data();

	EXPECT_EQ(dragon.levelMinimum, 9);
	EXPECT_EQ(dragon.levelMaximum, 0);
}

// One attack, the bite: 3d10 plus the juvenile's +4, physical. The breath is not a
// melee hit and is not this attack.
TEST(DragonStatBlockTest, ItsOneAttackIsAPhysicalBite)
{
	const MonsterParams dragon = dragon_from_data();

	EXPECT_EQ(dragon.naturalAttack, "bite");
	EXPECT_EQ(dragon.damage.dice.num, 3);
	EXPECT_EQ(dragon.damage.dice.sides, 10);
	EXPECT_EQ(dragon.damage.dice.bonus, 4);
	EXPECT_EQ(dragon.damage.damageType, DamageType::PHYSICAL);
}

// end of file: DragonStatBlockTest.cpp
