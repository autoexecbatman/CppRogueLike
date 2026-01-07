#include <gtest/gtest.h>
#include <atomic>
#include "src/Utils/UniqueId.h"

// Test fixture for UniqueId tests
class UniqueIdTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset the ID generator before each test
        UniqueId::Generator::nextId = 1;
    }
};

TEST_F(UniqueIdTest, SingleThreadedIdGeneration) {
    using namespace UniqueId;

    IdType id = Generator::getNextId();
    EXPECT_EQ(id, 1) << "Initial ID should be 1";

    id = Generator::getNextId();
    EXPECT_EQ(id, 2) << "Second call to getNextId should return 2";

    id = Generator::getNextId();
    EXPECT_EQ(id, 3) << "Third call to getNextId should return 3";
}

TEST_F(UniqueIdTest, IdsAreUnique) {
    using namespace UniqueId;

    IdType id1 = Generator::getNextId();
    IdType id2 = Generator::getNextId();
    IdType id3 = Generator::getNextId();

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
}

TEST_F(UniqueIdTest, IdsAreIncremental) {
    using namespace UniqueId;

    IdType id1 = Generator::getNextId();
    IdType id2 = Generator::getNextId();

    EXPECT_EQ(id2, id1 + 1) << "IDs should be incremental";
}
