// file: AiMimicTest.cpp
// Verifies AiMimic serialization correctness after the MIMIC AiType fix.
// Previously AiMimic::save() wrote AiType::MONSTER (0) via AiMonster::save(),
// causing loaded mimics to reconstruct as plain AiMonster and lose all state.
#include <gtest/gtest.h>
#include <memory>
#include <nlohmann/json.hpp>

#include "src/Ai/AiMimic.h"
#include "src/Ai/Ai.h"

using json = nlohmann::json;

TEST(AiMimicTest, Save_WritesMimicType_NotMonsterType)
{
    AiMimic mimic;
    json j;
    mimic.save(j);

    ASSERT_TRUE(j.contains("type")) << "save() must write a type field";
    // AiType::MIMIC = 4. Must NOT be AiType::MONSTER = 0.
    EXPECT_EQ(j["type"], 4) << "AiMimic must save as MIMIC (4), not MONSTER (0)";
}

TEST(AiMimicTest, Save_PreservesIsDisguisedState)
{
    AiMimic mimic;
    json j;
    mimic.save(j);

    ASSERT_TRUE(j.contains("isDisguised"));
    EXPECT_EQ(j["isDisguised"], true) << "freshly constructed mimic starts disguised";
}

TEST(AiMimicTest, SaveLoad_RoundTrip_PreservesType)
{
    AiMimic original;
    json j;
    original.save(j);

    // Reconstruct via Ai::create -- must yield AiMimic, not AiMonster.
    auto loaded = Ai::create(j);
    ASSERT_NE(loaded, nullptr);

    // Verify the round-trip preserves type by saving again and checking.
    json j2;
    loaded->save(j2);
    EXPECT_EQ(j2["type"], 4) << "loaded AI must still be MIMIC type";
}
