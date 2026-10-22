// file: SpiderStatBlockTest.cpp
// The three spiders in monsters.json are the Monstrous Manual's hairy, huge and giant
// spiders (Spider, PDF pages 1876-1877 of the local archive), by the owner's mapping:
// spider_small is the hairy spider, spider_giant the huge spider and spider_weaver the
// giant spider. Each expectation is that column's, read from the page:
//
//                 hairy        huge          giant
//   Armor Class:  8            6             4
//   Hit Dice:     1-1          2+2           3+3 or 4+4 (the 3+3 column is taken)
//   THAC0:        20           19            3+3 HD: 17
//   Damage:       1            1-6           1-8
//   Morale:       Average (10) Average (8)   Elite (13)
//   XP Value:     65           270           3+3 HD: 420
//   Size:         T            M             L
//   Intelligence: Low (5-7)    Animal (1)    Low (5-7)
//
// Hit dice are eight-sided with the bonus added once to the total, so 1-1 is 1d8-1.
// Their poisons live in their Ai (AiSpider, AiGiantSpider, AiWebSpinner) and are not
// restated here.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SpiderStatBlockTest.*

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "src/Ai.h"
#include "src/Creature.h"
#include "src/CreatureSize.h"
#include "src/DamageInfo.h"
#include "src/IntelligenceRating.h"
#include "src/MonsterCreator.h"
#include "src/MonsterHitDice.h"
#include "src/MonsterRegistry.h"
#include "src/Paths.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
struct BookSpider
{
	std::string_view key{};
	int armorClass{ 0 };
	int hitDiceCount{ 0 };
	int hitDiceBonus{ 0 };
	int thaco{ 0 };
	std::string_view damage{};
	int morale{ 0 };
	int xp{ 0 };
	CreatureSize size{ CreatureSize::MEDIUM };
	IntelligenceRating rating{ IntelligenceRating::AVERAGE };
	AiType ai{ AiType::SPIDER };
};

constexpr BookSpider BOOK_SPIDERS[] = {
	{
		"spider_small",
		8,
		1,
		-1,
		20,
		"1",
		10,
		65,
		CreatureSize::TINY,
		IntelligenceRating::LOW,
		AiType::SPIDER,
	},
	{
		"spider_giant",
		6,
		2,
		2,
		19,
		"1d6",
		8,
		270,
		CreatureSize::MEDIUM,
		IntelligenceRating::ANIMAL,
		AiType::GIANT_SPIDER,
	},
	{
		"spider_weaver",
		4,
		3,
		3,
		17,
		"1d8",
		13,
		420,
		CreatureSize::LARGE,
		IntelligenceRating::LOW,
		AiType::WEB_SPINNER,
	},
};
} // namespace

class SpiderStatBlockTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		monsters.load(Paths::MONSTERS);
		ctx = mock.to_game_context();
	}

	MonsterRegistry monsters{};
	MockGameContext mock{};
	GameContext ctx{};
};

TEST_F(SpiderStatBlockTest, EachSpiderCarriesItsColumnOfTheBook)
{
	for (const BookSpider& book : BOOK_SPIDERS)
	{
		const MonsterParams& spider = monsters.get_params(std::string{ book.key });
		EXPECT_EQ(spider.ac, book.armorClass) << book.key;
		ASSERT_EQ(spider.hitDice.counts.size(), 1u) << book.key;
		EXPECT_EQ(spider.hitDice.sides, 8) << book.key;
		EXPECT_EQ(spider.hitDice.counts.front().count, book.hitDiceCount) << book.key;
		EXPECT_EQ(spider.hitDice.bonus, book.hitDiceBonus) << book.key;
		EXPECT_EQ(spider.thaco, book.thaco) << book.key;
		EXPECT_EQ(spider.damage.displayRoll, book.damage) << book.key;
		EXPECT_EQ(spider.morale, book.morale) << book.key;
		EXPECT_EQ(spider.hitDice.counts.front().xp, book.xp) << book.key;
		EXPECT_EQ(spider.size, book.size) << book.key;
		EXPECT_EQ(spider.intelligence, book.rating) << book.key;
	}
}

// The hairy spider's "Damage/Attack: 1" is a fixed point, not dice: no die is rolled
// and the bite is worth exactly 1 before Strength and damage reduction.
TEST_F(SpiderStatBlockTest, TheHairySpidersBiteIsExactlyOne)
{
	const DamageInfo& bite = monsters.get_params("spider_small").damage;

	EXPECT_EQ(bite.dice.num, 0) << "the bite rolls a die";
	EXPECT_EQ(bite.minDamage, 1);
	EXPECT_EQ(bite.maxDamage, 1);
}

// Spiders walk on water and cross webs, theirs or another's, and each kind is built with
// the Ai that carries its poison.
TEST_F(SpiderStatBlockTest, ASpiderBuiltFromDataWalksWebsSwimsAndCarriesItsPoison)
{
	for (const BookSpider& book : BOOK_SPIDERS)
	{
		const auto spider = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, monsters.get_params(std::string{ book.key }), ctx);

		EXPECT_TRUE(spider->has_state(ActorState::CAN_WALK_WEBS)) << book.key;
		EXPECT_TRUE(spider->has_state(ActorState::CAN_SWIM)) << book.key;
		ASSERT_TRUE(spider->ai) << book.key;
		EXPECT_EQ(spider->ai->get_ai_type(), book.ai) << book.key;
	}
}

// end of file: SpiderStatBlockTest.cpp
