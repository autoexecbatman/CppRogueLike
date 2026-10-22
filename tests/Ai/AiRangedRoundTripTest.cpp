// file: AiRangedRoundTripTest.cpp
// A ranged monster comes back from a save as a ranged monster, keeping the distances it
// shoots and keeps from. The save names the Ai it records and Ai::create builds that one,
// so a ranged Ai that saved under a melee name would come back as a melee one: an archer
// that walks up to swing, with its two distances dropped.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=AiRangedRoundTripTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Ai.h"
#include "src/AiMonsterRanged.h"
#include "src/Persistent.h"

namespace
{
// Not the defaults, so a load that ignored them could not pass by accident.
constexpr int MAX_RANGE = 7;
constexpr int OPTIMAL_DISTANCE = 4;
} // namespace

TEST(AiRangedRoundTripTest, ARangedMonsterLoadsAsTheSameAiItSaved)
{
	AiMonsterRanged archer{};
	archer.load(json{
		{ "maxRangeDistance", MAX_RANGE },
		{ "optimalDistance", OPTIMAL_DISTANCE },
	});
	json saved;
	archer.save(saved);

	const std::unique_ptr<Ai> loaded = Ai::create(saved);
	json resaved;
	loaded->save(resaved);

	EXPECT_EQ(archer.get_ai_type(), AiType::RANGED_MONSTER);
	EXPECT_EQ(loaded->get_ai_type(), archer.get_ai_type());
	EXPECT_EQ(resaved, saved) << "the loaded Ai saved something else: " << resaved.dump();
}

// end of file: AiRangedRoundTripTest.cpp
