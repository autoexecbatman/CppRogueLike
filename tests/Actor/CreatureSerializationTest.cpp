#include "src/Actor.h"
#include "src/Ai.h"
#include "src/AiMonster.h"
#include "src/AiWebSpinner.h"
#include "src/Creature.h"
#include "src/ExperienceReward.h"
#include "src/MonsterAttacker.h"
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================================================
// CREATURE SERIALIZATION TESTS
// Ensures creatures save/load with all components intact
// ============================================================================

class CreatureSerializationTest : public ::testing::Test {
protected:
    std::unique_ptr<Creature> create_test_creature() {
        auto creature = std::make_unique<Creature>(
            Vector2D{ 20, 10 },
            ActorData{TileRef{}, "goblin", 1}
        );
        creature->set_strength(14);
        creature->set_dexterity(12);
        creature->set_constitution(10);
        creature->set_intelligence(8);
        creature->set_wisdom(7);
        creature->set_charisma(6);
        creature->set_gold(50);
        creature->set_natural_attack("Short Sword");

        creature->experienceReward = std::make_unique<ExperienceReward>(35);
        creature->set_dr(1);
        creature->set_thaco(19);
        creature->armorClass = std::make_unique<ArmorClass>(6);
        creature->healthPool = std::make_unique<HealthPool>(20);
        creature->attacker = std::make_unique<MonsterAttacker>(*creature, DamageInfo{"1d6", DamageType::PHYSICAL});
        creature->ai = std::make_unique<AiMonster>();

        return creature;
    }
};

TEST_F(CreatureSerializationTest, FullCreature_SaveLoad_RoundTrip) {
    auto original = create_test_creature();

    // Save
    json j;
    original->save(j);

    // Load into new creature
    auto loaded = std::make_unique<Creature>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    // Verify position (Vector2D is {y, x})
    EXPECT_EQ(loaded->position.x, 20);
    EXPECT_EQ(loaded->position.y, 10);

    // Verify stats
    EXPECT_EQ(loaded->get_strength(), 14);
    EXPECT_EQ(loaded->get_dexterity(), 12);
    EXPECT_EQ(loaded->get_constitution(), 10);
    EXPECT_EQ(loaded->get_gold(), 50);
    EXPECT_EQ(loaded->get_natural_attack(), "Short Sword");

    // Verify components exist
    ASSERT_NE(loaded->attacker, nullptr) << "Attacker not loaded";
    ASSERT_NE(loaded->ai, nullptr) << "AI not loaded";

    // Verify destructible values
    EXPECT_EQ(loaded->get_max_hp(), 20);
    EXPECT_EQ(loaded->get_dr(), 1);
    EXPECT_EQ(loaded->get_xp(), 35);
}

TEST_F(CreatureSerializationTest, Creature_WithDamage_PreserveHP) {
    auto original = create_test_creature();
    original->set_hp(15); // HP: 20 -> 15

    json j;
    original->save(j);

    auto loaded = std::make_unique<Creature>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    EXPECT_EQ(loaded->get_hp(), 15);
}

// Exceptional Strength is part of the saved creature: an 18/76 loads as 18/76.
TEST_F(CreatureSerializationTest, ExceptionalStrength_Preserved) {
    auto original = create_test_creature();
    original->set_strength(18);
    original->set_exceptional_strength(76);

    json j;
    original->save(j);

    auto loaded = std::make_unique<Creature>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    EXPECT_EQ(loaded->get_exceptional_strength(), 76);
}

TEST_F(CreatureSerializationTest, Creature_Dead_PreservesState) {
    auto original = create_test_creature();
    original->set_hp(-5); // Kill it

    ASSERT_TRUE(original->is_dead());

    json j;
    original->save(j);

    auto loaded = std::make_unique<Creature>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    EXPECT_TRUE(loaded->is_dead());
}

TEST_F(CreatureSerializationTest, Creature_NoHealthPool_HandledGracefully) {
    // Create creature without healthPool
    auto original = std::make_unique<Creature>(Vector2D{5, 5}, ActorData{TileRef{}, "mystery", 1});
    // Don't add healthPool

    json j;
    original->save(j);

    // Verify no healthPool key when not initialized
    EXPECT_FALSE(j.contains("healthPool"));
    // But constitutionTracker should still exist
    EXPECT_TRUE(j.contains("constitutionTracker"));

    auto loaded = std::make_unique<Creature>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
	EXPECT_NO_THROW(loaded->load(j)) << "a creature saved with no pool was refused on load";
	EXPECT_EQ(loaded->healthPool, nullptr) << "a pool appeared that was never saved";
}

TEST_F(CreatureSerializationTest, AttackerDamage_Preserved) {
    auto original = create_test_creature();

    json j;
    original->save(j);

    auto loaded = std::make_unique<Creature>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    ASSERT_NE(loaded->attacker, nullptr);

    // Get damage info and verify
    const auto& damageInfo = loaded->attacker->get_damage_info();
    EXPECT_EQ(damageInfo.minDamage, 1);
    EXPECT_EQ(damageInfo.maxDamage, 6);
}

// Loaded the way the game loads a level's monsters: a fresh Creature, then load(), with
// nothing built on it first. Every test above builds the pool by hand before loading, which
// is how a load that dropped hit points on the floor passed all of them.
TEST_F(CreatureSerializationTest, AMonsterLoadedIntoAFreshObjectKeepsItsHitPoints)
{
	auto original = create_test_creature();
	original->healthPool->set_hp(7);
	original->healthPool->set_temp_hp(3);

	json j;
	original->save(j);

	auto loaded = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "Unnamed", 0 });
	loaded->load(j);

	ASSERT_NE(loaded->healthPool, nullptr) << "the saved hit points were dropped: no pool on the loaded monster";
	EXPECT_EQ(loaded->get_max_hp(), 20);
	EXPECT_EQ(loaded->get_hp(), 7);
	EXPECT_EQ(loaded->get_temp_hp(), 3);
}

// Every field Creature::save writes on every creature is required on load: the owner's
// ruling is that no save fallbacks exist. The unconditional fields are found by saving a
// bare creature, which has none of the optional components, and subtracting what
// Actor::save writes - so nothing here is a list kept by hand.
// The save says which Ai a creature carries by name, so a record written by one build
// and read by another cannot quietly hand it a different one.
TEST_F(CreatureSerializationTest, TheAiKindIsSavedByName)
{
	json saved;
	AiWebSpinner{ 0 }.save(saved);

	EXPECT_EQ(saved.at("type"), "web_spinner");

	const std::unique_ptr<Ai> loaded = Ai::create(saved);
	json savedAgain;
	loaded->save(savedAgain);
	EXPECT_EQ(savedAgain.at("type"), "web_spinner") << "the Ai came back as something else";
}

// A record with no Ai name at all is refused too, by the field read itself.
TEST_F(CreatureSerializationTest, ARecordWithNoAiKindIsRefused)
{
	EXPECT_THROW((void)Ai::create(json::object()), json::out_of_range);
}

// An Ai this build does not know is refused, rather than becoming whichever one the
// number happens to land on.
TEST_F(CreatureSerializationTest, AnUnknownAiKindIsRefused)
{
	const json saved = json{ { "type", "sorcerer_of_the_ninth_circle" } };

	EXPECT_THROW((void)Ai::create(saved), std::runtime_error);
}

// A creature's alignment and class go into the record as names, so a record written by
// one build and read by another cannot quietly change what it says.
TEST_F(CreatureSerializationTest, TheAlignmentAndClassAreSavedByName)
{
	auto creature = create_test_creature();
	creature->set_ethics(Ethics::LAWFUL);
	creature->set_morality(Morality::EVIL);
	creature->set_creature_class(CreatureClass::ROGUE);

	json saved;
	creature->save(saved);

	EXPECT_EQ(saved.at("ethics"), "lawful");
	EXPECT_EQ(saved.at("morality"), "evil");
	EXPECT_EQ(saved.at("creatureClass"), "rogue");

	auto loaded = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
	loaded->load(saved);

	EXPECT_EQ(loaded->get_ethics(), Ethics::LAWFUL);
	EXPECT_EQ(loaded->get_morality(), Morality::EVIL);
	EXPECT_EQ(loaded->get_creature_class(), CreatureClass::ROGUE);
}

// And so does every buff the creature is carrying.
TEST_F(CreatureSerializationTest, ARunningBuffIsSavedByName)
{
	auto creature = create_test_creature();
	creature->activeBuffs.push_back(Buff{ BuffType::BLESS, 1, 30, false, {} });

	json saved;
	creature->save(saved);

	EXPECT_EQ(saved.at("activeBuffs").at(0).at("type"), "bless");

	auto loaded = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
	loaded->load(saved);

	ASSERT_EQ(loaded->activeBuffs.size(), 1u);
	EXPECT_EQ(loaded->activeBuffs.front().type, BuffType::BLESS);
}

// The damage an attacker deals carries its type by name, for the same reason: a record
// numbering it means something else the moment a type is inserted into the list.
TEST_F(CreatureSerializationTest, AnAttackersDamageTypeIsSavedByName)
{
	auto creature = create_test_creature();
	creature->attacker = std::make_unique<MonsterAttacker>(*creature, DamageInfo{ "2d4", DamageType::ACID });

	json saved;
	creature->save(saved);

	EXPECT_EQ(saved.at("attacker").at("damageInfo").at("type"), "acid");

	auto loaded = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
	loaded->load(saved);

	ASSERT_NE(loaded->attacker, nullptr);
	EXPECT_EQ(loaded->attacker->get_damage_info().damageType, DamageType::ACID);
}

// A damage type this build does not know is refused rather than cast to whatever the
// number lands on, and so is a record still numbering it.
TEST_F(CreatureSerializationTest, AnUnknownDamageTypeIsRefused)
{
	auto creature = create_test_creature();
	json saved;
	creature->save(saved);

	json withUnknownName = saved;
	withUnknownName["attacker"]["damageInfo"]["type"] = "sonic";
	auto loaded = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
	EXPECT_THROW(loaded->load(withUnknownName), std::runtime_error);

	json withNumber = saved;
	withNumber["attacker"]["damageInfo"]["type"] = 3;
	auto second = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
	EXPECT_ANY_THROW(second->load(withNumber));
}

// A name this build cannot place is refused, rather than becoming whichever value the
// number happens to land on.
TEST_F(CreatureSerializationTest, AnAlignmentNameThisBuildDoesNotKnowIsRefused)
{
	auto creature = create_test_creature();
	json saved;
	creature->save(saved);
	saved["ethics"] = "scrupulous";

	auto loaded = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
	EXPECT_THROW(loaded->load(saved), std::runtime_error);
}

// And so is a record still numbering them: saves are not kept compatible here, so the
// old shape fails loudly instead of being read as an index into today's enum.
TEST_F(CreatureSerializationTest, ARecordNumberingItsEnumsIsRefused)
{
	auto creature = create_test_creature();
	json saved;
	creature->save(saved);
	saved["creatureClass"] = 1;

	auto loaded = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
	EXPECT_ANY_THROW(loaded->load(saved));
}

TEST_F(CreatureSerializationTest, ACreatureRecordMissingAFieldItsSaverAlwaysWritesIsRefused)
{
	Creature bare{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "bare", 1 } };
	json bareRecord;
	bare.save(bareRecord);

	Actor plainActor{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "bare", 1 } };
	json actorRecord;
	plainActor.save(actorRecord);

	for (const auto& [key, value] : bareRecord.items())
	{
		if (actorRecord.contains(key))
		{
			continue;
		}
		json missingOne = bareRecord;
		missingOne.erase(key);
		auto loaded = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
		EXPECT_ANY_THROW(loaded->load(missingOne))
			<< "a creature record without \"" << key << "\" loaded quietly";
	}
}
