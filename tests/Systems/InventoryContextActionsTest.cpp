// file: InventoryContextActionsTest.cpp
//
// What a right-click offers on an inventory row.
//
// What it is for. ContextMenu is general - a list of labels, an anchor and a callback -
// but it was constructed in exactly one place, PlayerController's right-click on the
// map. There was no right-click on an item, so the inventory's two actions were reachable
// only by knowing that Enter uses and d drops.
//
// Every label here is something the keyboard already does. The menu moves the cursor to
// the clicked row and calls the same handler, so this file pins which actions apply to a
// row and nothing about what they mean.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=InventoryContextActionsTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "src/InventoryUI.h"

namespace
{
bool offers(const std::vector<std::string>& labels, const std::string& wanted)
{
	return std::ranges::find(labels, wanted) != labels.end();
}
} // namespace

TEST(InventoryContextActionsTest, ABackpackItemCanBeUsedOrDropped)
{
	const auto labels = InventoryActions::labels_for_row(InventoryScreen::BACKPACK, true);

	EXPECT_TRUE(offers(labels, "Use"));
	EXPECT_TRUE(offers(labels, "Drop"));
}

TEST(InventoryContextActionsTest, AWornItemCanBeTakenOffOrDropped)
{
	const auto labels = InventoryActions::labels_for_row(InventoryScreen::EQUIPMENT, true);

	EXPECT_TRUE(offers(labels, "Unequip"));
	EXPECT_TRUE(offers(labels, "Drop"));
	EXPECT_FALSE(offers(labels, "Use")) << "a worn item was offered Use, which equips it again";
}

TEST(InventoryContextActionsTest, AnEmptySlotOffersToBrowseWhatFitsIt)
{
	const auto labels = InventoryActions::labels_for_row(InventoryScreen::EQUIPMENT, false);

	EXPECT_TRUE(offers(labels, "Browse"));
	EXPECT_FALSE(offers(labels, "Drop")) << "an empty slot offered to drop what it does not hold";
	EXPECT_FALSE(offers(labels, "Unequip"));
}

TEST(InventoryContextActionsTest, ARowHoldingNothingOffersNothingAtAll)
{
	// A category heading in the backpack list. The map's context menu skips opening when
	// there is nothing actionable, and this follows it: empty means no menu.
	EXPECT_TRUE(InventoryActions::labels_for_row(InventoryScreen::BACKPACK, false).empty());
	EXPECT_TRUE(InventoryActions::labels_for_row(InventoryScreen::USABLES, false).empty());
}

TEST(InventoryContextActionsTest, EveryMenuOfferedCanBeDismissed)
{
	// Without Cancel the only way out of an opened menu is ESC, which a player who
	// reached it with the mouse has no reason to look for.
	for (const auto screen : { InventoryScreen::EQUIPMENT, InventoryScreen::BACKPACK, InventoryScreen::USABLES })
	{
		for (const bool holdsItem : { true, false })
		{
			const auto labels = InventoryActions::labels_for_row(screen, holdsItem);
			if (!labels.empty())
			{
				EXPECT_EQ(labels.back(), "Cancel") << "a menu was offered with no way to dismiss it";
			}
		}
	}
}

TEST(InventoryContextActionsTest, AUsableIsUsedTheSameWayAsABackpackItem)
{
	EXPECT_EQ(
		InventoryActions::labels_for_row(InventoryScreen::USABLES, true),
		InventoryActions::labels_for_row(InventoryScreen::BACKPACK, true));
}
