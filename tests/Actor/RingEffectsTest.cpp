// file: RingEffectsTest.cpp
// Magical rings worn on either hand. A ring of regeneration "restores one point of damage
// per turn" (Dungeon Master's Guide, PDF page 910 of the 2e archive), "the ring must be
// worn", and "in no case can the wearer's hit points exceed his usual maximum". Ten combat
// rounds are a turn (PDF page 24) and a game turn is a round, so the ring heals a point
// every tenth round.
//
// The book names no wound the ring cannot mend - only "total destruction of all living
// tissue by fire or acid" stops it - so fire and acid heal under it, where Constitution
// regeneration leaves them. A dead wearer stays dead.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RingEffectsTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string_view>
#include <vector>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/DamageInfo.h"
#include "src/DamageResolver.h"
#include "src/EquipmentSlot.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/ItemCreator.h"
#include "src/MagicalItemEffects.h"
#include "src/Player.h"
#include "src/SpellSystem.h"
#include "tests/mocks/MockGameContext.h"

class RingEffectsTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
	}

	// A creature of this class with maxHp hit points, down to hp, with a hand for each
	// ring. Constitution 10 regenerates nothing of its own.
	std::unique_ptr<Creature> make_wearer(CreatureClass creatureClass, int maxHp, int hp)
	{
		auto wearer = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "wearer", 0 });
		wearer->healthPool = std::make_unique<HealthPool>(maxHp);
		wearer->set_creature_class(creatureClass);
		wearer->set_constitution(10);
		wearer->set_hp(hp);
		wearer->set_body_plan({ EquipmentSlot::RIGHT_RING, EquipmentSlot::LEFT_RING });
		return wearer;
	}

	// Puts the ring from the item data on the given hand.
	void put_on_ring(Creature& wearer, std::string_view key, EquipmentSlot hand)
	{
		wearer.wear(ItemCreator::create(key, wearer.position, ctx), hand);
	}

	// Runs rounds 1 through `rounds` of the ring and returns the hit points gained.
	int regenerate_through(Creature& wearer, int rounds)
	{
		const int before = wearer.get_hp();
		for (int round = 1; round <= rounds; ++round)
		{
			wearer.regenerate_from_ring(round);
		}
		return wearer.get_hp() - before;
	}

	MockGameContext mock{};
	GameContext ctx{};
};

// The query behind every ring: this effect, on either hand.
TEST_F(RingEffectsTest, WearsRingOfAsksBothHandsForTheEffect)
{
	auto wearer = make_wearer(CreatureClass::FIGHTER, 20, 20);
	EXPECT_FALSE(wearer->wears_ring_of(MagicalEffect::FREE_ACTION)) << "bare hands";

	put_on_ring(*wearer, "ring_of_free_action", EquipmentSlot::LEFT_RING);

	EXPECT_TRUE(wearer->wears_ring_of(MagicalEffect::FREE_ACTION));
	EXPECT_FALSE(wearer->wears_ring_of(MagicalEffect::REGENERATION));
}

// A point every tenth round and not a round sooner: 12 in 120 rounds.
TEST_F(RingEffectsTest, ARingOfRegenerationHealsAPointEveryTurn)
{
	auto early = make_wearer(CreatureClass::FIGHTER, 200, 100);
	put_on_ring(*early, "ring_of_regeneration", EquipmentSlot::RIGHT_RING);
	EXPECT_EQ(regenerate_through(*early, 9), 0);

	auto watched = make_wearer(CreatureClass::FIGHTER, 200, 100);
	put_on_ring(*watched, "ring_of_regeneration", EquipmentSlot::RIGHT_RING);
	EXPECT_EQ(regenerate_through(*watched, 120), 12);
}

// It heals from the left hand as from the right.
TEST_F(RingEffectsTest, TheRingWorksOnEitherHand)
{
	auto wearer = make_wearer(CreatureClass::FIGHTER, 200, 100);
	put_on_ring(*wearer, "ring_of_regeneration", EquipmentSlot::LEFT_RING);

	EXPECT_EQ(regenerate_through(*wearer, 120), 12);
}

// Another ring heals nothing.
TEST_F(RingEffectsTest, WithoutTheRingNothingRegenerates)
{
	auto wearer = make_wearer(CreatureClass::FIGHTER, 200, 100);
	put_on_ring(*wearer, "ring_of_free_action", EquipmentSlot::RIGHT_RING);

	EXPECT_EQ(regenerate_through(*wearer, 120), 0);
}

// 6 acid and 4 fire on a 20-point wearer are gone after 100 rounds.
TEST_F(RingEffectsTest, TheRingMendsFireAndAcid)
{
	auto wearer = make_wearer(CreatureClass::FIGHTER, 20, 20);
	put_on_ring(*wearer, "ring_of_regeneration", EquipmentSlot::RIGHT_RING);
	wearer->take_damage(6, ctx, DamageType::ACID);
	wearer->take_damage(DamageResolver::reduce_dice({ 4 }, DamageType::FIRE, 0), ctx);
	ASSERT_EQ(wearer->get_hp(), 10);

	EXPECT_EQ(regenerate_through(*wearer, 100), 10);
	EXPECT_EQ(wearer->get_unregenerable_damage(), 0);
}

// One point short of the maximum, a hundred rounds give back that one point.
TEST_F(RingEffectsTest, TheRingNeverHealsPastTheMaximum)
{
	auto wearer = make_wearer(CreatureClass::FIGHTER, 20, 19);
	put_on_ring(*wearer, "ring_of_regeneration", EquipmentSlot::RIGHT_RING);

	EXPECT_EQ(regenerate_through(*wearer, 100), 1);
}

// A dead wearer stays dead.
TEST_F(RingEffectsTest, TheRingDoesNotRaiseTheDead)
{
	auto wearer = make_wearer(CreatureClass::FIGHTER, 20, 0);
	put_on_ring(*wearer, "ring_of_regeneration", EquipmentSlot::RIGHT_RING);

	EXPECT_EQ(regenerate_through(*wearer, 100), 0);
}

// The ring is its wearer's, a monster as much as a character; Constitution's
// regeneration is a character's alone.
TEST_F(RingEffectsTest, AMonsterWearingTheRingRegenerates)
{
	auto monster = make_wearer(CreatureClass::MONSTER, 200, 100);
	put_on_ring(*monster, "ring_of_regeneration", EquipmentSlot::RIGHT_RING);

	EXPECT_EQ(regenerate_through(*monster, 120), 12);
}

// A ring of invisibility puts invisibility in the spell list, from the left hand too.
TEST_F(RingEffectsTest, ARingOfInvisibilityGrantsTheSpell)
{
	auto player = std::make_unique<Player>(Vector2D{ 0, 0 });
	player->healthPool = std::make_unique<HealthPool>(20);
	player->armorClass = std::make_unique<ArmorClass>(10);
	player->set_strength(10);
	player->set_dexterity(10);
	ctx.playerOwner = &player;
	ASSERT_TRUE(player->equip_item(ItemCreator::create("ring_of_invisibility", player->position, ctx), EquipmentSlot::LEFT_RING, ctx));

	const std::vector<SpellSystem::ItemGrantedSpell> granted = SpellSystem::get_item_granted_spells(*player);

	ASSERT_EQ(granted.size(), 1u);
	EXPECT_EQ(granted.front().key, "invisibility");
	EXPECT_EQ(granted.front().source, "Ring");
}
