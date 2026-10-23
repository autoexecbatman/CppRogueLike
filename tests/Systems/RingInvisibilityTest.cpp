// file: RingInvisibilityTest.cpp
// What taking off a ring of invisibility ends. The ring's invisibility "is exactly the
// same as the wizard invisibility spell" (Dungeon Master's Guide, PDF page 908), granted
// at will through the ring, so taking the ring off ends that invisibility and no other:
// not a potion's, and not a spell cast from memory.
//
// The ring is put on and taken off the way the inventory screen does it, with use_item on
// the ring, and its invisibility is cast the way the spell menu casts an item's spell.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RingInvisibilityTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/SpellSystem.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// An hour of rounds, from the book's round of about a minute (PHB, PDF page 184).
constexpr int ROUNDS_IN_AN_HOUR = 60;
} // namespace

class RingInvisibilityTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		mock.player = std::make_unique<Player>(Vector2D{ 2, 2 });
		mock.player->experienceReward = std::make_unique<ExperienceReward>(0);
		mock.player->healthPool = std::make_unique<HealthPool>(10);
		mock.player->armorClass = std::make_unique<ArmorClass>(10);
	}

	// A ring of invisibility from the data, carried in the pack.
	Item& carry_a_ring()
	{
		auto ring =
			ItemCreator::create("ring_of_invisibility", wearer().position, ctx);
		Item* carried = ring.get();
		[[maybe_unused]] const auto added =
			InventoryOperations::add_item_to_inventory(
				wearer().inventoryData,
				std::move(ring),
				wearer(),
				*ctx.dataManager);
		EXPECT_TRUE(added.has_value()) << "the ring did not fit in the pack";
		return *carried;
	}

	// Puts the ring on, or takes it off if it is worn, as the inventory screen does.
	void use(Item& ring) { use_item(*ring.behavior, ring, wearer(), ctx); }

	// Casts invisibility the way the spell menu casts it from the given source.
	void cast_invisibility(SpellSource source)
	{
		const auto casting_spent = [](GameContext& completedCtx) {
		};
		SpellSystem::cast_spell_by_key(
			"invisibility",
			wearer(),
			source,
			casting_spent,
			ctx);
	}

	Player& wearer() { return ctx.player_concrete(); }

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
};

// A spell's invisibility is not the ring's to end.
TEST_F(RingInvisibilityTest, TakingTheRingOffLeavesASpellsInvisibility)
{
	Item& ring = carry_a_ring();
	use(ring);
	ctx.buffSystem->add_buff(
		wearer(),
		BuffType::INVISIBILITY,
		0,
		10,
		BuffApplication::ADD,
		ctx.gameState->get_time());

	use(ring);

	EXPECT_TRUE(wearer().is_invisible())
		<< "taking the ring off ended invisibility it did not grant";
}

// The ring's invisibility is its own grant, apart from the spell's.
TEST_F(RingInvisibilityTest, CastingThroughTheRingIsNotTheSpellFromMemory)
{
	Item& ring = carry_a_ring();
	use(ring);

	cast_invisibility(SpellSource::ITEM);

	ASSERT_TRUE(wearer().is_invisible());
	EXPECT_FALSE(ctx.buffSystem->has_buff(wearer(), BuffType::INVISIBILITY))
		<< "the ring's invisibility is the spell's buff, so taking the ring off cannot tell them apart";
}

// While a second ring of invisibility is worn, taking one off ends nothing.
TEST_F(RingInvisibilityTest, ASecondRingStillWornKeepsTheInvisibility)
{
	Item& first = carry_a_ring();
	Item& second = carry_a_ring();
	use(first);
	use(second);
	cast_invisibility(SpellSource::ITEM);

	use(first);

	EXPECT_TRUE(wearer().is_invisible())
		<< "one ring came off and the other still grants it";
}

// The ring's own invisibility ends when the ring comes off.
TEST_F(RingInvisibilityTest, TakingTheRingOffEndsItsOwnInvisibility)
{
	Item& ring = carry_a_ring();
	use(ring);
	cast_invisibility(SpellSource::ITEM);

	use(ring);

	EXPECT_FALSE(wearer().is_invisible());
}

// Removal is the end, not the clock: worn past an hour the ring keeps its wearer unseen,
// and taking it off then ends it at once.
TEST_F(RingInvisibilityTest, WornPastAnHourItLastsUntilTheRingComesOff)
{
	Item& ring = carry_a_ring();
	use(ring);
	cast_invisibility(SpellSource::ITEM);

	ctx.buffSystem->update_creature_buffs(
		wearer(),
		expiry_time(0, ROUNDS_IN_AN_HOUR + 1));
	ASSERT_TRUE(wearer().is_invisible())
		<< "worn past an hour, the ring's invisibility ran out";

	use(ring);

	EXPECT_FALSE(wearer().is_invisible());
}

// Like the spell, the ring's invisibility ends when its wearer attacks.
TEST_F(RingInvisibilityTest, AttackingEndsTheRingsInvisibility)
{
	Item& ring = carry_a_ring();
	use(ring);
	cast_invisibility(SpellSource::ITEM);

	ctx.buffSystem->remove_buffs_broken_by_attacking(wearer());

	EXPECT_FALSE(wearer().is_invisible());
}

// end of file: RingInvisibilityTest.cpp
