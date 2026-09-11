// file: AssertProbeTest.cpp
//
// Drives six invariants past their bound and checks that the assertion guarding
// each one aborts the process.
//
// An assertion nobody has watched fire has not been shown to exist. The rest of
// the suite is blind to these, because an abort is not an exception: no
// EXPECT_THROW reaches one, and no ordinary test survives one. Each probe
// therefore runs in a child process through GoogleTest's death-test mechanism,
// and the text it expects is the assertion's own message - which is what
// separates "the process died" from "this assertion is what stopped it".
//
// What this deliberately does not do: it never checks that the guarded code
// works. Every call below is a violation. Correct behaviour for the same
// functions is covered by BodyPlanTest, EquipmentStorageTest and
// MonsterEquipmentTest.
//
// Where the expectations come from: each string is copied out of the assertion
// it targets, and carries enough of that assertion's own wording to identify it
// - two of the six would otherwise match each other's message. A probe that
// stops matching is a probe whose assertion was reworded, moved or deleted, and
// that is the finding.
//
// Run it:
//
//   cmake --build build --config Debug --target test_exe
//   cd build/bin/Debug && ./test_exe.exe --gtest_filter=AssertProbeDeathTest.*
//
//   [==========] Running 6 tests from 1 test suite.
//   [ RUN      ] AssertProbeDeathTest.WearingNothingAborts
//   [       OK ] AssertProbeDeathTest.WearingNothingAborts (69 ms)
//   ...
//   [  PASSED  ] 6 tests.
//
// A green run here says every probe died with the right message. It does not
// say the probes have teeth - for that, tests/assert_probes.ps1 deletes each
// targeted assertion in turn, rebuilds, and confirms the matching probe fails.

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>

#include "src/Actor/Creature.h"
#include "src/Actor/EquipmentSlot.h"
#include "src/Actor/Item.h"
#include "src/Colors/Colors.h"
#include "src/Combat/ExperienceReward.h"
#include "src/Core/Paths.h"
#include "src/Factories/ItemCreator.h"
#include "src/Factories/MonsterCreator.h"
#include "src/Systems/Shopkeepers/ShopkeeperFactory.h"
#include "tests/mocks/MockGameContext.h"

// GoogleTest runs any suite whose name ends in DeathTest before the others, so
// the abort happens before anything else has had a chance to start a thread.
class AssertProbeDeathTest : public ::testing::Test
{
protected:
	MockGameContext mock{};
	GameContext ctx{};

	void SetUp() override
	{
#ifdef NDEBUG
		// Said out loud rather than passed silently: with the assertions
		// compiled out there is nothing here to fire, and a green run would
		// claim otherwise.
		GTEST_SKIP() << "assertions are compiled out under NDEBUG; these probes can prove nothing in this build";
#endif
		// Content is loaded so a probe dies on the invariant it names rather
		// than on a missing file, which aborts with the wrong message.
		MonsterCreator::load(Paths::MONSTERS);
		ItemCreator::load(Paths::ITEMS);

		ctx = mock.to_game_context();
	}

	static std::unique_ptr<Creature> make_creature()
	{
		return std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "orc", WHITE_BLACK_PAIR });
	}

	static std::unique_ptr<Item> make_item(std::string_view name)
	{
		return std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, std::string(name), WHITE_BLACK_PAIR });
	}

	// The goblin carries a short sword, so this one call reaches both of
	// create_from_params' registry assertions. The creature it would return is
	// discarded because neither probe lets the call get that far.
	void build_a_goblin()
	{
		[[maybe_unused]] const std::unique_ptr<Creature> built =
			MonsterCreator::create_from_params(Vector2D{ 0, 0 }, MonsterCreator::get_params("goblin"), ctx);
	}
};

// wear() takes ownership, so a caller passing nothing has already lost whatever
// it meant to hand over. There is no state to record for equipping nothing.
TEST_F(AssertProbeDeathTest, WearingNothingAborts)
{
	std::unique_ptr<Creature> creature = make_creature();
	creature->set_body_plan({ EquipmentSlot::RIGHT_HAND });

	EXPECT_DEATH(creature->wear(nullptr, EquipmentSlot::RIGHT_HAND), "wear called with no item");
}

// A body plan is the set of slots a creature has at all. Wearing into one it
// does not have puts an item where no lookup will ever find it again.
TEST_F(AssertProbeDeathTest, WearingIntoASlotTheBodyLacksAborts)
{
	std::unique_ptr<Creature> creature = make_creature();
	creature->set_body_plan({ EquipmentSlot::RIGHT_HAND });

	EXPECT_DEATH(creature->wear(make_item("chain mail"), EquipmentSlot::BODY), "a slot this body does not have");
}

// Every monster's body comes from the registry. Without one it would be built
// with no slots at all and quietly wear nothing it was authored to carry.
TEST_F(AssertProbeDeathTest, BuildingAMonsterWithNoBodyPlanRegistryAborts)
{
	ctx.bodyPlanRegistry = nullptr;

	EXPECT_DEATH(build_a_goblin(), "create_from_params called without a bodyPlanRegistry");
}

// What a monster carries is a real item, made through the same registry the
// player's gear comes from. A goblin's short sword has to exist to be worn.
TEST_F(AssertProbeDeathTest, BuildingAMonsterWithNoContentRegistryAborts)
{
	ctx.contentRegistry = nullptr;

	EXPECT_DEATH(build_a_goblin(), "create_from_params called without a contentRegistry");
}

// A shopkeeper is a person with a knife, and the dagger is a real item in a
// real hand rather than a name on the creature.
TEST_F(AssertProbeDeathTest, ConfiguringAShopkeeperWithNoContentRegistryAborts)
{
	std::unique_ptr<Creature> shopkeeper = make_creature();
	ctx.contentRegistry = nullptr;

	EXPECT_DEATH(ShopkeeperFactory::configure_shopkeeper(*shopkeeper, 1, ctx),
		"configure_shopkeeper called without a contentRegistry");
}

// Death pays experience to the player, so a context holding no player has
// nobody to pay. This path runs on every kill in the game.
TEST_F(AssertProbeDeathTest, KillingACreatureWithNoPlayerInContextAborts)
{
	std::unique_ptr<Creature> creature = make_creature();
	creature->experienceReward = std::make_unique<ExperienceReward>(10);

	EXPECT_DEATH(creature->die(ctx), "requires a live player in context");
}
