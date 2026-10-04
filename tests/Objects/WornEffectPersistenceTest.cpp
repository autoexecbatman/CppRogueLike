// file: WornEffectPersistenceTest.cpp
// A worn item's magical effect survives a save, so reloading does not disarm it.
//
// What it is for. Five behaviours carry a MagicalEffect: MagicalHelm, MagicalRing,
// JewelryAmulet, Gauntlets and Girdle. Creature::wears_item_with reads that field off
// any of them, which is how a wearer crosses water and how a worn protection is found.
// The first two wrote the field to their save record; the other three went through
// save_stat_boost, which writes the six ability bonuses and neither the effect nor its
// bonus. So the gauntlets of swimming came back from a save as plain gauntlets, and a
// character who could cross water before saving could not after loading, with nothing
// printed.
//
// What is checked here is the round trip for every behaviour that carries an effect,
// rather than for the one item that exposed it, because the defect was a shared helper
// and the next item to carry an effect will use the same one.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=WornEffectPersistenceTest.*

#include <gtest/gtest.h>

#include <memory>
#include <variant>

#include <nlohmann/json.hpp>

#include "src/BodyPlanRegistry.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/Item.h"
#include "src/MagicalItemEffects.h"
#include "src/Pickable.h"
#include "tests/mocks/MockGameContext.h"

using json = nlohmann::json;

namespace
{
// A behaviour written out and read back, the way Item::save and Item::load do it.
ItemBehavior round_trip(const ItemBehavior& behavior)
{
	json record;
	save_behavior(behavior, record);
	const std::optional<ItemBehavior> loaded = load_behavior(record);
	EXPECT_TRUE(loaded.has_value()) << "the record did not read back as a behaviour at all";
	return loaded.value_or(behavior);
}
} // namespace

// The gauntlets of swimming and climbing, which is the item has_bypass names: its
// effect is the whole of what makes it worth wearing.
TEST(WornEffectPersistenceTest, GauntletsKeepTheirEffect)
{
	Gauntlets worn{};
	worn.effect = MagicalEffect::SWIMMING;
	worn.strBonus = 3;

	const ItemBehavior reloaded = round_trip(ItemBehavior{ worn });

	ASSERT_TRUE(std::holds_alternative<Gauntlets>(reloaded));
	EXPECT_EQ(std::get<Gauntlets>(reloaded).effect, MagicalEffect::SWIMMING)
		<< "the gauntlets of swimming came back as plain gauntlets";
	EXPECT_EQ(std::get<Gauntlets>(reloaded).strBonus, 3) << "the ability bonus was already saved and must stay saved";
}

// The amulet of ogre power carries protection in the data, so an amulet is the second
// of the three that loses something real.
TEST(WornEffectPersistenceTest, AnAmuletKeepsItsEffectAndItsBonus)
{
	JewelryAmulet worn{};
	worn.effect = MagicalEffect::PROTECTION;
	worn.bonus = 1;

	const ItemBehavior reloaded = round_trip(ItemBehavior{ worn });

	ASSERT_TRUE(std::holds_alternative<JewelryAmulet>(reloaded));
	EXPECT_EQ(std::get<JewelryAmulet>(reloaded).effect, MagicalEffect::PROTECTION);
	EXPECT_EQ(std::get<JewelryAmulet>(reloaded).bonus, 1) << "a protection of +1 came back as +0";
}

// No girdle in the data carries an effect today. It goes through the same helper, so
// the first one that does would have been born broken.
TEST(WornEffectPersistenceTest, AGirdleKeepsAnEffectNoItemCarriesYet)
{
	Girdle worn{};
	worn.effect = MagicalEffect::FREE_ACTION;
	worn.bonus = 2;

	const ItemBehavior reloaded = round_trip(ItemBehavior{ worn });

	ASSERT_TRUE(std::holds_alternative<Girdle>(reloaded));
	EXPECT_EQ(std::get<Girdle>(reloaded).effect, MagicalEffect::FREE_ACTION);
	EXPECT_EQ(std::get<Girdle>(reloaded).bonus, 2);
}

// The two that were already right, so a fix to the other three cannot quietly break
// them by sharing a helper the wrong way round.
TEST(WornEffectPersistenceTest, AHelmAndARingKeepWhatTheyAlreadyKept)
{
	MagicalHelm helm{};
	helm.effect = MagicalEffect::UNDERWATER_ACTION;
	const ItemBehavior reloadedHelm = round_trip(ItemBehavior{ helm });
	ASSERT_TRUE(std::holds_alternative<MagicalHelm>(reloadedHelm));
	EXPECT_EQ(std::get<MagicalHelm>(reloadedHelm).effect, MagicalEffect::UNDERWATER_ACTION);

	MagicalRing ring{};
	ring.effect = MagicalEffect::FREE_ACTION;
	ring.bonus = 1;
	const ItemBehavior reloadedRing = round_trip(ItemBehavior{ ring });
	ASSERT_TRUE(std::holds_alternative<MagicalRing>(reloadedRing));
	EXPECT_EQ(std::get<MagicalRing>(reloadedRing).effect, MagicalEffect::FREE_ACTION);
	EXPECT_EQ(std::get<MagicalRing>(reloadedRing).bonus, 1);
}

// What the defect cost a player, stated as the thing they would notice: a character
// who could cross water before saving could not after loading.
TEST(WornEffectPersistenceTest, AReloadedWearerStillCrossesWater)
{
	MockGameContext mock{};
	Creature wearer{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "swimmer", ColorPairId::WHITE_BLACK } };
	wearer.set_body_plan(mock.body_plans.get("humanoid"));

	Gauntlets worn{};
	worn.effect = MagicalEffect::SWIMMING;

	auto gauntlets = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "gauntlets", ColorPairId::WHITE_BLACK });
	gauntlets->behavior = round_trip(ItemBehavior{ worn });
	wearer.wear(std::move(gauntlets), EquipmentSlot::GAUNTLETS);

	EXPECT_TRUE(wearer.has_bypass(ActorState::CAN_SWIM))
		<< "the gauntlets of swimming stopped carrying their wearer across water after a save";
}
