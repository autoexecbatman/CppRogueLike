// file: ItemRegistryTest.cpp
// An ItemRegistry is a value: each instance holds its own items, so an edit made
// through one - the item editor, a test - never reaches another. Game owns the one the
// game plays with and hands it out as ctx.itemRegistry.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemRegistryTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "src/ItemRegistry.h"
#include "src/Paths.h"

// Raising the health potion's value in one registry leaves another's at the file's.
TEST(ItemRegistryTest, AnEditToOneRegistryDoesNotReachAnother)
{
	ItemRegistry first{};
	ItemRegistry second{};
	first.load(Paths::ITEMS);
	second.load(Paths::ITEMS);
	const int shippedValue = second.get_params("health_potion").value;

	ItemParams dearer = first.get_params("health_potion");
	dearer.value = shippedValue + 500;
	first.set_params("health_potion", dearer);

	EXPECT_EQ(first.get_params("health_potion").value, shippedValue + 500);
	EXPECT_EQ(second.get_params("health_potion").value, shippedValue)
		<< "an edit to one registry reached another";
}

// An item added to one registry is listed by that registry alone, under its own name.
TEST(ItemRegistryTest, ACustomItemBelongsToTheRegistryItWasAddedTo)
{
	ItemRegistry first{};
	ItemRegistry second{};
	first.load(Paths::ITEMS);
	second.load(Paths::ITEMS);

	const std::string key =
		first.add_custom("Registry Trinket", "potion", ItemParams{});

	EXPECT_EQ(key, "registry_trinket")
		<< "the key is the name, lower case, spaces as underscores";
	EXPECT_EQ(first.get_params(key).name, "Registry Trinket");
	EXPECT_EQ(first.get_params(key).category, "potion");
	EXPECT_TRUE(std::ranges::contains(first.get_all_keys(), key));
	EXPECT_FALSE(std::ranges::contains(second.get_all_keys(), key))
		<< "a custom item reached another registry";
}

// A name already taken - by a custom item or a shipped one - is numbered from 2.
TEST(ItemRegistryTest, ATakenNameIsNumbered)
{
	ItemRegistry items{};
	items.load(Paths::ITEMS);

	EXPECT_EQ(
		items.add_custom("Registry Trinket", "potion", ItemParams{}),
		"registry_trinket");
	EXPECT_EQ(
		items.add_custom("Registry Trinket", "potion", ItemParams{}),
		"registry_trinket_2");
	EXPECT_EQ(
		items.add_custom("Health Potion", "potion", ItemParams{}),
		"health_potion_2");
}

// Only a custom item can be removed; a key nobody holds is an error too.
TEST(ItemRegistryTest, AShippedItemCannotBeRemoved)
{
	ItemRegistry items{};
	items.load(Paths::ITEMS);

	EXPECT_THROW(items.remove_custom("health_potion"), std::logic_error);
	EXPECT_THROW(items.remove_custom("no_such_item"), std::out_of_range);
}

// Loading replaces the custom items a registry held with the file's.
TEST(ItemRegistryTest, ALoadReplacesTheCustomItemsItHeld)
{
	ItemRegistry items{};
	items.load(Paths::ITEMS);
	const std::string key =
		items.add_custom("Registry Trinket", "potion", ItemParams{});

	items.load(Paths::ITEMS);

	EXPECT_FALSE(std::ranges::contains(items.get_all_keys(), key))
		<< "a custom item survived a load";
}

// An item's name and category are strings its params own, so a copy of them outlives the
// registry entry it was read from.
TEST(ItemRegistryTest, ItemParamsOwnTheirStrings)
{
	EXPECT_TRUE((std::is_same_v<decltype(ItemParams::name), std::string>));
	EXPECT_TRUE((std::is_same_v<decltype(ItemParams::category), std::string>));
}

// A params copied out of the registry keeps its name and category after the load that
// removes its entry. While they were views into the entry, this read freed memory.
TEST(ItemRegistryTest, AParamsCopyKeepsItsNameAfterAReload)
{
	ItemRegistry items{};
	items.load(Paths::ITEMS);
	// Longer than any short-string buffer, so the name lives on the heap the load frees.
	const std::string key = items.add_custom(
		"Registry Trinket Of Some Length",
		"potion",
		ItemParams{});
	const ItemParams copied = items.get_params(key);

	items.load(Paths::ITEMS);

	EXPECT_EQ(copied.name, "Registry Trinket Of Some Length");
	EXPECT_EQ(copied.category, "potion");
}

// Replacing an item's params leaves its name and category to set_name_category.
TEST(ItemRegistryTest, SetParamsKeepsTheItemsOwnNameAndCategory)
{
	ItemRegistry items{};
	items.load(Paths::ITEMS);
	const ItemParams shipped = items.get_params("health_potion");

	ItemParams renamed = shipped;
	renamed.name = "Some Other Name";
	renamed.category = "weapon";
	renamed.value = shipped.value + 1;
	items.set_params("health_potion", renamed);

	EXPECT_EQ(items.get_params("health_potion").value, shipped.value + 1);
	EXPECT_EQ(items.get_params("health_potion").name, shipped.name);
	EXPECT_EQ(items.get_params("health_potion").category, shipped.category);
}
