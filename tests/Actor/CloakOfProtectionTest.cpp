// file: CloakOfProtectionTest.cpp A worn cloak of protection betters armour
// class by its plus, but only over leather armour or none, and never with
// magical armour or a shield.
//
// What it is for. The DMG (PDF page 951): "each plus of a cloak of protection
// betters Armor Class by one". Page 952: "This device can be combined with
// other items or worn with leather armor. It cannot function in conjunction
// with any sort of magical armor, normal armor not made of leather, or with a
// shield of any sort." The DMG names only "leather armor"; studded leather and
// hide count as made of leather by the PHB's own descriptions (PDF page 157):
// hide is "prepared from the extremely thick hide of a creature ... or from
// multiple layers of regular leather", and padded is "quilted layers of cloth",
// so padded does not. A ring of protection is "cumulative with all other
// magical items of protection" (DMG page 908), so the two stack.
//
// Armour class values are PHB Table 46: none 10, leather or padded 8, studded
// leather 7, chain mail 5, and a shield betters any of them by one.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CloakOfProtectionTest.*
#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "src/ArmorClass.h"
#include "src/ArmorMaterial.h"
#include "src/BuffSystem.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/ItemEditor.h"
#include "src/ItemEnhancements.h"
#include "src/ItemRegistry.h"
#include "src/MagicalItemEffects.h"
#include "src/MessageSystem.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace CppRL
{

namespace
{
	// PHB Table 46.
	constexpr int BARE_ARMOR_CLASS = 10;
	constexpr int LEATHER_ARMOR_CLASS = 8;
	constexpr int PADDED_ARMOR_CLASS = 8;
	constexpr int STUDDED_LEATHER_ARMOR_CLASS = 7;
	constexpr int CHAIN_MAIL_ARMOR_CLASS = 5;
	constexpr int LEATHER_AND_SHIELD_ARMOR_CLASS = 7;
	// The shipped cloak and ring are each +1.
	constexpr int CLOAK_PLUS = 1;
	constexpr int RING_PLUS = 1;
	// A creature built in a test has no Strength, and the pack is weight-gated
	// by it.
	constexpr int CARRIER_STRENGTH = 18;
	// Dexterity with no armour class adjustment (PHB Table 2), so an item's
	// Dexterity bonus moves nothing.
	constexpr int NEUTRAL_DEXTERITY = 10;
} // namespace

class CloakOfProtectionTest : public ::testing::Test
{
protected:
	MockGameContext mock{};
	GameContext ctx{};
	BuffSystem buffs{};

	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.buffSystem = &buffs;
		mock.player = std::make_unique<Player>(Vector2D{ 0, 0 });
		Player& wearer = *mock.player;
		wearer.experienceReward = std::make_unique<ExperienceReward>(0);
		wearer.armorClass = std::make_unique<ArmorClass>(BARE_ARMOR_CLASS);
		wearer.healthPool = std::make_unique<HealthPool>(20);
		wearer.set_strength(CARRIER_STRENGTH);
		wearer.set_dexterity(NEUTRAL_DEXTERITY);
		ctx.playerOwner = &mock.player;
	}

	Player& player() { return *mock.player; }

	// The item made from its record, type identified so it shows its real name,
	// in the player's pack.
	Item& carry(std::string_view key)
	{
		auto made = ItemCreator::create(key, player().position, ctx);
		made->identify_type();
		Item* carried = made.get();
		[[maybe_unused]] const auto added = InventoryOperations::add_item(
			player().inventoryData,
			std::move(made));
		return *carried;
	}

	// Carries the item and puts it on the way the inventory's use does.
	Item& wear(std::string_view key)
	{
		Item& item = carry(key);
		[[maybe_unused]] const bool used =
			use_item(*item.behavior, item, player(), ctx);
		return item;
	}

	int armor_class() { return player().get_armor_class(); }
};

TEST_F(CloakOfProtectionTest, ACloakAloneBettersBareArmourClassByItsPlus)
{
	wear("cloak_of_protection");
	ASSERT_NE(player().get_equipped_item(EquipmentSlot::CLOAK), nullptr)
		<< "the cloak did not go on";

	EXPECT_EQ(armor_class(), BARE_ARMOR_CLASS - CLOAK_PLUS);
}

TEST_F(CloakOfProtectionTest, ACloakOverLeatherBettersItByItsPlus)
{
	wear("leather_armor");
	ASSERT_EQ(armor_class(), LEATHER_ARMOR_CLASS);

	wear("cloak_of_protection");

	EXPECT_EQ(armor_class(), LEATHER_ARMOR_CLASS - CLOAK_PLUS);
}

TEST_F(CloakOfProtectionTest, ACloakOverStuddedLeatherBettersItByItsPlus)
{
	wear("studded_leather");
	ASSERT_EQ(armor_class(), STUDDED_LEATHER_ARMOR_CLASS);

	wear("cloak_of_protection");

	EXPECT_EQ(armor_class(), STUDDED_LEATHER_ARMOR_CLASS - CLOAK_PLUS);
}

// Relative to hide alone: Table 46 gives hide 6, and the shipped record gives
// 7, a data defect of its own that this test does not decide.
TEST_F(CloakOfProtectionTest, ACloakOverHideBettersItByItsPlus)
{
	wear("hide_armor");
	const int hideAlone = armor_class();

	wear("cloak_of_protection");

	EXPECT_EQ(armor_class(), hideAlone - CLOAK_PLUS);
}

TEST_F(CloakOfProtectionTest, ACloakOverPaddedArmourGivesNothing)
{
	wear("padded_armor");
	ASSERT_EQ(armor_class(), PADDED_ARMOR_CLASS);

	wear("cloak_of_protection");

	EXPECT_EQ(armor_class(), PADDED_ARMOR_CLASS)
		<< "padded is cloth, not leather";
}

TEST_F(CloakOfProtectionTest, ACloakOverChainMailGivesNothing)
{
	wear("chain_mail");
	ASSERT_EQ(armor_class(), CHAIN_MAIL_ARMOR_CLASS);

	wear("cloak_of_protection");

	EXPECT_EQ(armor_class(), CHAIN_MAIL_ARMOR_CLASS);
}

TEST_F(CloakOfProtectionTest, ACloakWithAShieldGivesNothing)
{
	wear("leather_armor");
	wear("medium_shield");
	ASSERT_EQ(armor_class(), LEATHER_AND_SHIELD_ARMOR_CLASS);

	wear("cloak_of_protection");

	EXPECT_EQ(armor_class(), LEATHER_AND_SHIELD_ARMOR_CLASS);
}

TEST_F(CloakOfProtectionTest, ACloakOverMagicalLeatherGivesNothing)
{
	Item& leather = carry("leather_armor");
	ItemEnhancement magical{};
	magical.isMagical = true;
	leather.apply_enhancement(magical);
	[[maybe_unused]] const bool used =
		use_item(*leather.behavior, leather, player(), ctx);
	const int magicalLeatherAlone = armor_class();

	wear("cloak_of_protection");

	EXPECT_EQ(armor_class(), magicalLeatherAlone);
}

TEST_F(CloakOfProtectionTest, ACloakAndARingOfProtectionStack)
{
	wear("leather_armor");
	wear("ring_of_protection_plus_1");
	ASSERT_EQ(armor_class(), LEATHER_ARMOR_CLASS - RING_PLUS);

	wear("cloak_of_protection");

	EXPECT_EQ(armor_class(), LEATHER_ARMOR_CLASS - RING_PLUS - CLOAK_PLUS);
}

TEST_F(CloakOfProtectionTest, TheBreakdownNamesTheCloakOnlyWhenItCounts)
{
	wear("leather_armor");
	wear("cloak_of_protection");

	const ArmorClassBreakdown counted =
		player().armorClass->update(player(), ctx);

	EXPECT_EQ(counted.cloak.name, "cloak of protection");
	EXPECT_EQ(counted.cloak.bonus, -CLOAK_PLUS);

	wear("chain_mail");
	const ArmorClassBreakdown refused =
		player().armorClass->update(player(), ctx);

	EXPECT_TRUE(refused.cloak.name.empty())
		<< "a cloak over chain mail was named as counting";
	EXPECT_EQ(refused.cloak.bonus, 0);
}

TEST_F(CloakOfProtectionTest, ACloakWithoutProtectionGivesNothingAndIsNotNamed)
{
	wear("leather_armor");
	wear("cloak_of_elvenkind");
	ASSERT_NE(player().get_equipped_item(EquipmentSlot::CLOAK), nullptr)
		<< "the cloak of elvenkind did not go on";

	const ArmorClassBreakdown breakdown =
		player().armorClass->update(player(), ctx);

	EXPECT_EQ(armor_class(), LEATHER_ARMOR_CLASS);
	EXPECT_TRUE(breakdown.cloak.name.empty())
		<< "a cloak with no protection was named as counting";
}

// The breakdown goes to the debug log, MessageSystem::log, which writes
// std::clog.
TEST_F(CloakOfProtectionTest, TheArmourClassLogNamesTheCloak)
{
	mock.messages.enable_debug_mode();
	wear("leather_armor");
	std::ostringstream captured{};
	std::streambuf* const original = std::clog.rdbuf(captured.rdbuf());

	wear("cloak_of_protection");

	std::clog.rdbuf(original);
	EXPECT_NE(
		captured.str().find("  -1 from cloak of protection\n"),
		std::string::npos)
		<< "the armour class log did not name the cloak: " << captured.str();
}

TEST_F(CloakOfProtectionTest, AnArmoursMaterialSurvivesASave)
{
	nlohmann::json saved{};
	save_behavior(ItemBehavior{ Armor{ -2, ArmorMaterial::LEATHER } }, saved);

	const ItemBehavior loaded = load_behavior(saved);

	EXPECT_EQ(saved.at("armorMaterial"), "leather");
	EXPECT_EQ(std::get<Armor>(loaded).material, ArmorMaterial::LEATHER);
}

TEST_F(CloakOfProtectionTest, TheItemEditorShowsAndCyclesTheMaterial)
{
	using Field = ItemEditorModel::FieldId;
	ItemEditorModel model{};
	model.load(ctx, "leather_armor");
	ASSERT_EQ(model.field_value(Field::ARMOR_MATERIAL), "leather");

	model.field_toggle(Field::ARMOR_MATERIAL);
	model.commit(ctx, "leather_armor");

	EXPECT_EQ(model.field_value(Field::ARMOR_MATERIAL), "metal");
	EXPECT_EQ(
		ctx.itemRegistry->get_params("leather_armor").armorMaterial,
		ArmorMaterial::METAL);
}

// Only protection is armour: a cloak with another effect gives nothing,
// whatever its plus.
TEST_F(CloakOfProtectionTest, ACloaksPlusCountsOnlyWhenItsEffectIsProtection)
{
	wear("leather_armor");
	Item& cloak = carry("cloak_of_elvenkind");
	Gauntlets& behavior = std::get<Gauntlets>(*cloak.behavior);
	ASSERT_NE(behavior.effect, MagicalEffect::PROTECTION);
	behavior.bonus = 2;

	[[maybe_unused]] const bool used =
		use_item(*cloak.behavior, cloak, player(), ctx);

	EXPECT_EQ(armor_class(), LEATHER_ARMOR_CLASS);
}

TEST_F(CloakOfProtectionTest, TheShippedArmoursCarryTheirMaterial)
{
	const auto material_of = [this](std::string_view key)
	{
		return ctx.itemRegistry->get_params(key).armorMaterial;
	};

	EXPECT_EQ(material_of("leather_armor"), ArmorMaterial::LEATHER);
	EXPECT_EQ(material_of("studded_leather"), ArmorMaterial::LEATHER);
	EXPECT_EQ(material_of("hide_armor"), ArmorMaterial::LEATHER);
	EXPECT_EQ(material_of("padded_armor"), ArmorMaterial::CLOTH);
	EXPECT_EQ(material_of("chain_mail"), ArmorMaterial::METAL);
	EXPECT_EQ(material_of("full_plate"), ArmorMaterial::METAL);
	EXPECT_EQ(material_of("cloak_of_protection"), ArmorMaterial::NONE);
}

TEST_F(CloakOfProtectionTest, MadeArmourCarriesItsRecordsMaterial)
{
	Item& leather = carry("leather_armor");

	EXPECT_EQ(
		std::get<Armor>(*leather.behavior).material,
		ArmorMaterial::LEATHER);
}

TEST(ArmorMaterialTest, NamesRoundTripAndAnUnknownNameIsRefused)
{
	for (const ArmorMaterialRecord& record : ARMOR_MATERIAL_TABLE)
	{
		EXPECT_EQ(
			parse_armor_material(encode_armor_material(record.material)),
			record.material);
	}
	EXPECT_EQ(encode_armor_material(ArmorMaterial::LEATHER), "leather");

	try
	{
		[[maybe_unused]] const ArmorMaterial parsed =
			parse_armor_material("bone");
		FAIL() << "an unknown material was accepted";
	}
	catch (const std::runtime_error& refusal)
	{
		EXPECT_STREQ(refusal.what(), "unknown armor_material 'bone'");
	}
}

TEST(ArmorMaterialTest, TheCycleVisitsEveryMaterialAndWraps)
{
	EXPECT_EQ(next_armor_material(ArmorMaterial::NONE), ArmorMaterial::CLOTH);
	EXPECT_EQ(
		next_armor_material(ArmorMaterial::CLOTH),
		ArmorMaterial::LEATHER);
	EXPECT_EQ(
		next_armor_material(ArmorMaterial::LEATHER),
		ArmorMaterial::METAL);
	EXPECT_EQ(next_armor_material(ArmorMaterial::METAL), ArmorMaterial::NONE);
}

} // namespace CppRL
