#include <gtest/gtest.h>

#include "src/UniqueId.h"
#include <atomic>
#include <nlohmann/json.hpp>

// Test fixture for UniqueId tests
class UniqueIdTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset the ID generator before each test
        UniqueId::Generator::set_next_id(1);
    }
};

TEST_F(UniqueIdTest, SingleThreadedIdGeneration) {
    using namespace UniqueId;

    IdType id = Generator::generate();
    EXPECT_EQ(id, 1) << "Initial ID should be 1";

    id = Generator::generate();
    EXPECT_EQ(id, 2) << "Second call to generate should return 2";

    id = Generator::generate();
    EXPECT_EQ(id, 3) << "Third call to generate should return 3";
}

TEST_F(UniqueIdTest, IdsAreUnique) {
    using namespace UniqueId;

    IdType id1 = Generator::generate();
    IdType id2 = Generator::generate();
    IdType id3 = Generator::generate();

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
}

TEST_F(UniqueIdTest, IdsAreIncremental) {
    using namespace UniqueId;

    IdType id1 = Generator::generate();
    IdType id2 = Generator::generate();

    EXPECT_EQ(id2, id1 + 1) << "IDs should be incremental";
}

// The counter has to cross a save, or the next object created after a load takes an
// id that a loaded actor already holds. This is the defect issue 0015 describes,
// reduced to the two calls that carry the counter.
TEST_F(UniqueIdTest, TheCounterSurvivesASave)
{
	using namespace UniqueId;

	// A session that issued some ids, of which only the first two still exist -
	// whatever held the third was destroyed, which is what opens the gap.
	const IdType first = Generator::generate();
	const IdType second = Generator::generate();
	Generator::generate();

	nlohmann::json saved;
	save(saved);

	// A process restart: the static counter is back at its initial value.
	Generator::set_next_id(1);

	load(saved);

	const IdType afterLoad = Generator::generate();
	EXPECT_GT(afterLoad, first) << "a new object took an id a loaded actor already holds";
	EXPECT_GT(afterLoad, second) << "a new object took an id a loaded actor already holds";
	EXPECT_GE(afterLoad, 4u) << "the counter resumed below where the session left it";
}

// A save written before the counter was stored must not send the generator backwards.
// Loading nothing leaves it where it stands.
TEST_F(UniqueIdTest, ASaveWithoutTheCounterLeavesItAlone)
{
	using namespace UniqueId;

	Generator::generate();
	Generator::generate();
	const IdType before = Generator::peek_next_id();

	const nlohmann::json empty = nlohmann::json::object();
	load(empty);

	EXPECT_EQ(Generator::peek_next_id(), before) << "an older save reset the counter";
}

// Nothing may ever be issued the id that means "no id". This is why the counter
// starts at 1, and nothing checked it.
TEST_F(UniqueIdTest, AGeneratedIdIsNeverTheInvalidOne)
{
	using namespace UniqueId;

	for (int issued = 0; issued < 8; ++issued)
	{
		EXPECT_NE(Generator::generate(), INVALID_ID);
	}
}

// peek_next_id says what generate will return, which is the whole of what makes it
// safe to write into a save.
TEST_F(UniqueIdTest, PeekReportsWhatGenerateWillReturn)
{
	using namespace UniqueId;

	const IdType peeked = Generator::peek_next_id();

	EXPECT_EQ(Generator::generate(), peeked);
}

// Loading a save from inside a running game finds the counter already past what the
// file holds. Lowering it would hand out ids that objects still in memory are using,
// which is the same collision the fix exists to prevent, arriving from the other side.
TEST_F(UniqueIdTest, LoadingAnOlderSaveNeverLowersTheCounter)
{
	using namespace UniqueId;

	nlohmann::json early;
	save(early); // taken while the counter is still at 1

	for (int issued = 0; issued < 5; ++issued)
	{
		Generator::generate();
	}
	const IdType reached = Generator::peek_next_id();

	load(early);

	EXPECT_EQ(Generator::peek_next_id(), reached)
		<< "an older save sent the counter back over ids already in use";
}

// Setting the counter puts it exactly there. Every other case in this file either
// expects the counter to climb or happens to run first, so a set_next_id that quietly
// did nothing would satisfy all of them - and then no save could ever resume.
TEST_F(UniqueIdTest, SettingTheCounterPutsItExactlyThere)
{
	using namespace UniqueId;

	constexpr IdType somewhereElse = 500;
	Generator::set_next_id(somewhereElse);

	EXPECT_EQ(Generator::peek_next_id(), somewhereElse);
	EXPECT_EQ(Generator::generate(), somewhereElse) << "the counter reported one value and issued another";
}
