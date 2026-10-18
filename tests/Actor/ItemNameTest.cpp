// file: ItemNameTest.cpp
// Pins the name an item shows the player: identified enhancement parts by name,
// unknown ones as "?", a known curse appended, and every name its own string, so
// two names held at once can differ. The expected words are get_prefix_name's
// table: SHARP is "Sharp", FLAMING is "Flaming".

#include <format>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "src/Actor.h"
#include "src/Colors.h"
#include "src/Item.h"
#include "src/ItemEnhancements.h"
#include "src/Vector2D.h"

namespace
{
// A long sword carrying one prefix, its enhancement identified.
std::unique_ptr<Item> identified_sword(PrefixType prefix)
{
	auto sword = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "long sword", ColorPairId::WHITE_BLACK });
	ItemEnhancement enhancement{};
	enhancement.prefix = prefix;
	sword->apply_enhancement(enhancement);
	sword->identify_enhancement();
	return sword;
}
} // namespace

// Both names are taken before either is formatted, so each must be its own string.
TEST(ItemNameTest, TwoEnhancedNamesHeldAtOnceDiffer)
{
	const auto sharp = identified_sword(PrefixType::SHARP);
	const auto flaming = identified_sword(PrefixType::FLAMING);

	EXPECT_EQ(std::format("{} and {}", sharp->get_name(), flaming->get_name()), "Sharp long sword and Flaming long sword");
}

// A name held by the caller is not rewritten by the next item's name.
TEST(ItemNameTest, ANameOutlivesTheNextCall)
{
	const auto sharp = identified_sword(PrefixType::SHARP);
	const auto flaming = identified_sword(PrefixType::FLAMING);

	const std::string& sharpName = sharp->get_name();
	[[maybe_unused]] const std::string flamingName = flaming->get_name();

	EXPECT_EQ(sharpName, "Sharp long sword");
}

// An enhancement the player has not identified shows as "?" in place of its prefix.
TEST(ItemNameTest, AnUnidentifiedPrefixShowsAsAQuestionMark)
{
	Item sword{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "long sword", ColorPairId::WHITE_BLACK } };
	ItemEnhancement enhancement{};
	enhancement.prefix = PrefixType::SHARP;
	sword.apply_enhancement(enhancement);

	EXPECT_EQ(sword.get_name(), "? long sword");
}

// A curse the player has identified is appended to the name.
TEST(ItemNameTest, AnIdentifiedCurseIsAppended)
{
	Item sword{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "long sword", ColorPairId::WHITE_BLACK } };
	ItemEnhancement enhancement{};
	enhancement.prefix = PrefixType::SHARP;
	enhancement.blessing = BlessingStatus::CURSED;
	sword.apply_enhancement(enhancement);
	sword.identify_enhancement();
	sword.identify_buc();

	EXPECT_EQ(sword.get_name(), "Sharp long sword (cursed)");
}
