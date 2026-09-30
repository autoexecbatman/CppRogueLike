#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Colors.h"
#include "GameContext.h"
#include "InventoryData.h"
#include "InventoryOperations.h"
#include "Item.h"
#include "ItemCreator.h" // SINGLE SOURCE OF TRUTH
#include "LevelManager.h"
#include "MessageSystem.h"
#include "Persistent.h"
#include "Player.h"
#include "RandomDice.h"
#include "ShopKeeper.h"
#include "Vector2D.h"

using namespace InventoryOperations;

ShopKeeper::ShopKeeper(ShopType type, ShopQuality quality, RandomDice& dice)
	: shopType(type), shopQuality(quality)
{
	// Set pricing based on quality
	switch (quality)
	{
	case ShopQuality::POOR:
	{
		markupPercent = 110;
		sellbackPercent = 50;
		break;
	}
	case ShopQuality::AVERAGE:
	{
		markupPercent = 120;
		sellbackPercent = 60;
		break;
	}
	case ShopQuality::GOOD:
	{
		markupPercent = 130;
		sellbackPercent = 70;
		break;
	}
	case ShopQuality::EXCELLENT:
	{
		markupPercent = 150;
		sellbackPercent = 80;
		break;
	}
	}

	generate_shop_name(dice);
	// Note: generate_initial_inventory(ctx) must be called separately after construction
}

int ShopKeeper::get_buy_price(const Item& item) const
{
	int base_price = item.get_value() > 0 ? item.get_value() : 10;
	return (base_price * markupPercent) / 100;
}

int ShopKeeper::get_sell_price(const Item& item) const
{
	int base_price = item.get_value() > 0 ? item.get_value() : 10;
	return (base_price * sellbackPercent) / 100;
}

void ShopKeeper::generate_initial_inventory(int dungeonLevel, GameContext& ctx)
{
	shopInventory.items.clear();

	// Generate 3-7 random items based on shop type
	int item_count = ctx.dice->roll(3, 7);

	for (int i = 0; i < item_count; i++)
	{
		std::unique_ptr<Item> item = generate_random_item_by_type(dungeonLevel, ctx);
		if (item)
		{
			// At most seven items into fifty slots, so stocking cannot overflow.
			[[maybe_unused]] const auto stocked = add_item(shopInventory, std::move(item));
			assert(stocked.has_value());
		}
	}
}

std::unique_ptr<Item> ShopKeeper::generate_random_item_by_type(int dungeonLevel, GameContext& ctx)
{
	switch (shopType)
	{
	case ShopType::WEAPON_SHOP:
	{
		return generate_random_weapon(dungeonLevel, ctx);
	}
	case ShopType::ARMOR_SHOP:
	{
		return generate_random_armor(dungeonLevel, ctx);
	}
	case ShopType::POTION_SHOP:
	{
		return generate_random_potion(dungeonLevel, ctx);
	}
	case ShopType::SCROLL_SHOP:
	{
		return generate_random_scroll(dungeonLevel, ctx);
	}
	case ShopType::GENERAL_STORE:
	{
		switch (ctx.dice->roll(0, 3))
		{
		case 0:
		{
			return generate_random_weapon(dungeonLevel, ctx);
		}
		case 1:
		{
			return generate_random_armor(dungeonLevel, ctx);
		}
		case 2:
		{
			return generate_random_potion(dungeonLevel, ctx);
		}
		case 3:
		{
			return generate_random_scroll(dungeonLevel, ctx);
		}
		}
		break;
	}
	default:
	{
		return generate_random_misc_item(dungeonLevel, ctx);
	}
	}
	return nullptr;
}

std::unique_ptr<Item> ShopKeeper::generate_random_weapon(int dungeonLevel, GameContext& ctx)
{
	auto item = ItemCreator::create_random_of_category("weapon", { 0, 0 }, ctx, dungeonLevel);

	if (item && ctx.dice->roll(1, 100) <= 40)
	{
		item->generate_random_enhancement(MagicalPrefixes::ALLOWED, *ctx.dice);
	}

	return item;
}

std::unique_ptr<Item> ShopKeeper::generate_random_armor(int dungeonLevel, GameContext& ctx)
{
	auto item = ItemCreator::create_random_of_category("armor", { 0, 0 }, ctx, dungeonLevel);

	if (item && ctx.dice->roll(1, 100) <= 35)
	{
		item->generate_random_enhancement(MagicalPrefixes::ALLOWED, *ctx.dice);
	}

	return item;
}

std::unique_ptr<Item> ShopKeeper::generate_random_potion(int dungeonLevel, GameContext& ctx)
{
	return ItemCreator::create_random_of_category("potion", { 0, 0 }, ctx, dungeonLevel);
}

std::unique_ptr<Item> ShopKeeper::generate_random_scroll(int dungeonLevel, GameContext& ctx)
{
	return ItemCreator::create_random_of_category("scroll", { 0, 0 }, ctx, dungeonLevel);
}

std::unique_ptr<Item> ShopKeeper::generate_random_misc_item(int dungeonLevel, GameContext& ctx)
{
	Vector2D shop_pos{ 0, 0 };

	std::unique_ptr<Item> item;
	const int category = ctx.dice->roll(0, 3);
	switch (category)
	{
	case 0:
	{
		item = ItemCreator::create_random_of_category("weapon", shop_pos, ctx, dungeonLevel);
		break;
	}
	case 1:
	{
		item = ItemCreator::create_random_of_category("armor", shop_pos, ctx, dungeonLevel);
		break;
	}
	case 2:
	{
		item = ItemCreator::create_random_of_category("potion", shop_pos, ctx, dungeonLevel);
		break;
	}
	case 3:
	{
		item = ItemCreator::create("food_ration", shop_pos, ctx);
		break;
	}
	}

	// Apply price variation (+-5)
	item->set_value(std::max(1, item->get_value() + ctx.dice->roll(-5, 5)));

	// 15% chance for enhancement (only for equipment)
	if (item->is_weapon() || item->is_armor())
	{
		if (ctx.dice->roll(1, 100) <= 15)
		{
			item->generate_random_enhancement(MagicalPrefixes::EXCLUDED, *ctx.dice);
		}
	}

	return item;
}

bool ShopKeeper::process_player_purchase(GameContext& ctx, Item& item, Creature& buyer, Creature& owner)
{
	const int price = get_buy_price(item);

	// Both refusals come before anything moves, so a refused purchase leaves the shelf,
	// the pack and both purses as they were.
	if (buyer.get_gold() < price)
	{
		ctx.messageSystem->message(ColorPairId::WHITE_RED, "You don't have enough gold!", MessageCompletion::FINISHED);
		return false;
	}
	if (is_inventory_full(buyer.inventoryData))
	{
		ctx.messageSystem->message(ColorPairId::WHITE_RED, "Your inventory is full!", MessageCompletion::FINISHED);
		return false;
	}
	if (!is_within_weight_limit(item, buyer, *ctx.dataManager))
	{
		ctx.messageSystem->message(ColorPairId::WHITE_RED, "Too heavy to carry.", MessageCompletion::FINISHED);
		return false;
	}

	// The buy menu offers only what is on these shelves, so the item is always found.
	auto taken = remove_item(shopInventory, item);
	assert(taken.has_value() && "process_player_purchase called with an item not on the shelves");

	// The item itself moves, so its key and enhancement come with it. Room and weight
	// were checked above, so this add cannot fail, and the item lives on in the pack,
	// which keeps the reference valid for the message below.
	[[maybe_unused]] const auto handedOver = add_item_to_inventory(buyer.inventoryData, std::move(*taken), buyer, *ctx.dataManager);
	assert(handedOver.has_value());

	buyer.adjust_gold(-price);
	owner.adjust_gold(price);

	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "You bought ");
	ctx.messageSystem->append_message_part(ColorPairId::YELLOW_BLACK, item.get_name()); // Use enhanced name
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " for ");
	ctx.messageSystem->append_message_part(ColorPairId::YELLOW_BLACK, std::to_string(price));
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " gold.");
	ctx.messageSystem->finalize_message();

	return true;
}

bool ShopKeeper::process_player_sale(GameContext& ctx, Item& item, Creature& seller, Creature& owner)
{
	const int price = get_sell_price(item);

	// Both refusals come before anything moves, so a refused sale leaves the pack, the
	// shelves and both purses as they were.
	if (is_inventory_full(shopInventory))
	{
		ctx.messageSystem->message(ColorPairId::WHITE_RED, "Shopkeeper's inventory is full.", MessageCompletion::FINISHED);
		return false;
	}
	if (owner.get_gold() < price)
	{
		ctx.messageSystem->message(ColorPairId::WHITE_RED, "Shopkeeper does not have enough gold to buy the item.", MessageCompletion::FINISHED);
		return false;
	}

	// The sell menu offers only what is in the pack, so the item is always found.
	auto removed = remove_item(seller.inventoryData, item);
	assert(removed.has_value() && "process_player_sale called with an item the seller does not hold");

	// Room was checked above, so shelving cannot fail. The item lives on in the shop,
	// which keeps the reference valid for the message below.
	[[maybe_unused]] const auto shelved = add_item(shopInventory, std::move(*removed));
	assert(shelved.has_value());

	owner.adjust_gold(-price);
	seller.adjust_gold(price);

	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "You sold ");
	ctx.messageSystem->append_message_part(ColorPairId::YELLOW_BLACK, item.get_name()); // Use enhanced name
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " for ");
	ctx.messageSystem->append_message_part(ColorPairId::YELLOW_BLACK, std::to_string(price));
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " gold.");
	ctx.messageSystem->finalize_message();

	return true;
}

void ShopKeeper::generate_shop_name(RandomDice& dice)
{
	std::vector<std::string> weapon_names = {
		"The Sharp Edge", "Blades & Bludgeons", "Steel & Iron", "The Armory", "Warrior's Arsenal", "The Forge", "Sword & Shield", "Battle Ready"
	};

	std::vector<std::string> armor_names = {
		"Ironclad Armory", "Plate & Mail", "The Defender", "Armor & Protection", "Steel Defense", "Guardian's Gear", "The Shield Wall", "Heavy Metal"
	};

	std::vector<std::string> potion_names = {
		"Mystic Brews", "The Alchemy Shop", "Bubbling Cauldron", "Elixir Emporium", "Potion Master", "The Brew House", "Magical Mixtures", "Liquid Magic"
	};

	std::vector<std::string> scroll_names = {
		"Arcane Scrolls", "The Scriptorium", "Magical Manuscripts", "Spell Scrolls", "The Magic Word", "Enchanted Texts", "Scroll & Quill", "Ancient Writings"
	};

	std::vector<std::string> general_names = {
		"General Store", "The Trading Post", "Odds & Ends", "Everything Shop", "The Merchant's Den", "All Things", "Trade & Barter", "The Bazaar"
	};

	switch (shopType)
	{
	case ShopType::WEAPON_SHOP:
	{
		shopName = weapon_names[dice.roll(0, static_cast<int>(weapon_names.size()) - 1)];
		break;
	}
	case ShopType::ARMOR_SHOP:
	{
		shopName = armor_names[dice.roll(0, static_cast<int>(armor_names.size()) - 1)];
		break;
	}
	case ShopType::POTION_SHOP:
	{
		shopName = potion_names[dice.roll(0, static_cast<int>(potion_names.size()) - 1)];
		break;
	}
	case ShopType::SCROLL_SHOP:
	{
		shopName = scroll_names[dice.roll(0, static_cast<int>(scroll_names.size()) - 1)];
		break;
	}
	case ShopType::GENERAL_STORE:
	{
		shopName = general_names[dice.roll(0, static_cast<int>(general_names.size()) - 1)];
		break;
	}
	default:
	{
		shopName = "Shop";
		break;
	}
	}
}

void ShopKeeper::save(json& j)
{
	j["shop_type"] = static_cast<int>(shopType);
	j["shop_quality"] = static_cast<int>(shopQuality);
	j["shop_name"] = shopName;
	j["markup_percent"] = markupPercent;
	j["sellback_percent"] = sellbackPercent;

	// Save shop inventory
	json inventoryJson;
	save_inventory(shopInventory, inventoryJson);
	j["shop_inventory"] = inventoryJson;
}

void ShopKeeper::load(const json& j)
{
	shopType = static_cast<ShopType>(j.at("shop_type").get<int>());
	shopQuality = static_cast<ShopQuality>(j.at("shop_quality").get<int>());
	shopName = j.at("shop_name").get<std::string>();
	markupPercent = j.at("markup_percent").get<int>();
	sellbackPercent = j.at("sellback_percent").get<int>();

	// Load shop inventory
	shopInventory = FloorInventory(50);
	load_inventory(shopInventory, j.at("shop_inventory"));
}

std::unique_ptr<ShopKeeper> ShopKeeper::create(const json& j)
{
	auto shopkeeper = std::make_unique<ShopKeeper>();
	shopkeeper->load(j);
	return shopkeeper;
}
