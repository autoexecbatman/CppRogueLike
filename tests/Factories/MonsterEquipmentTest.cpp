#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "src/Factories/MonsterCreator.h"
#include "src/Factories/ItemCreator.h"
#include "src/Systems/BodyPlanRegistry.h"
#include "src/Actor/EquipmentSlot.h"

// A monster either wields an item or fights with its body. The two are
// separate fields because only one of them names something in items.json.
class MonsterEquipmentTest : public ::testing::Test
{
protected:
	BodyPlanRegistry bodyPlans;

	void SetUp() override
	{
		MonsterCreator::load("data/content/monsters.json");
		ItemCreator::load("data/content/items.json");
		bodyPlans.load("data/content/body_plans.json");
	}

	static const MonsterParams::StartingItem* find_slot(const MonsterParams& params, EquipmentSlot slot)
	{
		auto match = std::ranges::find_if(params.equipment,
			[slot](const MonsterParams::StartingItem& entry) { return entry.slot == slot; });
		return match == params.equipment.end() ? nullptr : &*match;
	}
};

// The orc's Long Sword is a real item, so it names the items.json key rather
// than a display string.
TEST_F(MonsterEquipmentTest, WieldingMonsterCarriesAnItemKey)
{
	const MonsterParams& orc = MonsterCreator::get_params("orc");

	const MonsterParams::StartingItem* mainHand = find_slot(orc, EquipmentSlot::RIGHT_HAND);
	ASSERT_NE(mainHand, nullptr);
	EXPECT_EQ(mainHand->itemKey, "long_sword");
	EXPECT_TRUE(orc.naturalAttack.empty());
}

// A troll's claws are part of it. Nothing in items.json corresponds, so no
// slot is filled.
TEST_F(MonsterEquipmentTest, ClawedMonsterCarriesNothing)
{
	const MonsterParams& troll = MonsterCreator::get_params("troll");

	EXPECT_TRUE(troll.equipment.empty());
	EXPECT_EQ(troll.naturalAttack, "Claws");
}

// The archer's Longbow and the warden's Longsword were misspelled against
// items.json, which is why neither resolved.
TEST_F(MonsterEquipmentTest, MisspelledWeaponsResolveToRealKeys)
{
	const MonsterParams::StartingItem* bow =
		find_slot(MonsterCreator::get_params("archer"), EquipmentSlot::MISSILE_WEAPON);
	ASSERT_NE(bow, nullptr);
	EXPECT_EQ(bow->itemKey, "long_bow");

	const MonsterParams::StartingItem* blade =
		find_slot(MonsterCreator::get_params("dungeon_warden"), EquipmentSlot::RIGHT_HAND);
	ASSERT_NE(blade, nullptr);
	EXPECT_EQ(blade->itemKey, "long_sword");
}

// Every key a monster carries must exist in items.json. This is the guard the
// display-name field never had.
TEST_F(MonsterEquipmentTest, EveryStartingItemKeyIsKnown)
{
	const std::vector<std::string> allItemKeys = ItemCreator::get_all_keys();
	const std::set<std::string> knownItemKeys(allItemKeys.begin(), allItemKeys.end());

	for (const std::string& key : MonsterCreator::get_all_keys())
	{
		const MonsterParams& params = MonsterCreator::get_params(key);
		for (const MonsterParams::StartingItem& entry : params.equipment)
		{
			EXPECT_TRUE(knownItemKeys.contains(entry.itemKey))
				<< key << " carries unknown item key " << entry.itemKey;
			EXPECT_NE(entry.slot, EquipmentSlot::NONE)
				<< key << " carries " << entry.itemKey << " in no slot";
		}
	}
}

// A creature that carries gear has somewhere to put it; one that fights with
// its body has no slots at all.
TEST_F(MonsterEquipmentTest, BodyPlanCoversEveryCarriedSlot)
{
	for (const std::string& key : MonsterCreator::get_all_keys())
	{
		const MonsterParams& params = MonsterCreator::get_params(key);
		const std::vector<EquipmentSlot>& plan = bodyPlans.get(params.bodyPlanName);
		for (const MonsterParams::StartingItem& carried : params.equipment)
		{
			const bool planned = std::ranges::find(plan, carried.slot) != plan.end();
			EXPECT_TRUE(planned)
				<< key << " carries " << carried.itemKey << " in a slot its body does not have";
		}
	}
}

// The wolf wears nothing, and nothing in the data pretends otherwise.
TEST_F(MonsterEquipmentTest, BeastHasNoSlots)
{
	EXPECT_TRUE(bodyPlans.get(MonsterCreator::get_params("wolf").bodyPlanName).empty());
	EXPECT_FALSE(bodyPlans.get(MonsterCreator::get_params("orc").bodyPlanName).empty());
}

// Eleven monsters share one humanoid body rather than eleven copies of the
// same fifteen slot names.
TEST_F(MonsterEquipmentTest, WieldersShareOneBodyTemplate)
{
	EXPECT_EQ(MonsterCreator::get_params("orc").bodyPlanName, "humanoid");
	EXPECT_EQ(MonsterCreator::get_params("kobold").bodyPlanName, "humanoid");
	EXPECT_TRUE(MonsterCreator::get_params("wolf").bodyPlanName.empty());
}

// A misspelled template is a data error and must not read as a creature that
// simply wears nothing.
TEST_F(MonsterEquipmentTest, UnknownBodyTemplateIsRefused)
{
	EXPECT_THROW(bodyPlans.get("humaniod"), std::runtime_error);
	EXPECT_NO_THROW(bodyPlans.get(""));
}

// The saver rebuilds the file from scratch, so anything it does not write is
// destroyed by the first save from the editor. Every creature's body name
// survives the trip.
TEST_F(MonsterEquipmentTest, SavePreservesEveryBodyName)
{
	const std::filesystem::path roundTrip =
		std::filesystem::temp_directory_path() / "monsters_roundtrip.json";

	std::map<std::string, std::string> before;
	for (const std::string& key : MonsterCreator::get_all_keys())
	{
		before.emplace(key, MonsterCreator::get_params(key).bodyPlanName);
	}

	MonsterCreator::save(roundTrip.string());
	MonsterCreator::load(roundTrip.string());

	for (const auto& [key, bodyPlanName] : before)
	{
		EXPECT_EQ(MonsterCreator::get_params(key).bodyPlanName, bodyPlanName)
			<< key << " lost its body across a save";
	}
	EXPECT_EQ(before.at("orc"), "humanoid");

	// Leave the shared registry holding the real file, not the temporary one.
	MonsterCreator::load("data/content/monsters.json");
	std::filesystem::remove(roundTrip);
}

// Body composition follows the Monstrous Manual rather than a single humanoid
// default. A medusa's snakes leave no room for a helmet.
TEST_F(MonsterEquipmentTest, MedusaHasNoHeadSlot)
{
	const std::vector<EquipmentSlot>& plan =
		bodyPlans.get(MonsterCreator::get_params("medusa").bodyPlanName);

	EXPECT_EQ(std::ranges::find(plan, EquipmentSlot::HEAD), plan.end());
	EXPECT_NE(std::ranges::find(plan, EquipmentSlot::BODY), plan.end());
}

// The stone golem entry says it is weaponless and wears nothing, so it gets no
// slots at all rather than a body nobody fills.
TEST_F(MonsterEquipmentTest, StoneGolemWearsNothing)
{
	EXPECT_TRUE(MonsterCreator::get_params("golem_stone").bodyPlanName.empty());
	EXPECT_TRUE(bodyPlans.get(
		MonsterCreator::get_params("golem_stone").bodyPlanName).empty());
}

// A harpy uses a club and wears nothing, so it has hands and no wardrobe.
TEST_F(MonsterEquipmentTest, HarpyHasHandsAndNothingElse)
{
	const std::vector<EquipmentSlot>& plan =
		bodyPlans.get(MonsterCreator::get_params("harpy").bodyPlanName);

	EXPECT_EQ(plan.size(), 2u);
	EXPECT_NE(std::ranges::find(plan, EquipmentSlot::RIGHT_HAND), plan.end());
	EXPECT_EQ(std::ranges::find(plan, EquipmentSlot::BODY), plan.end());
}
