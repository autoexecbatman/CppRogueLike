#pragma once

#include <memory>
#include <string>

#include "../Actor/InventoryData.h"
#include "../Persistent/Persistent.h"

struct GameContext;

enum class ShopType
{
	WEAPON_SHOP,
	ARMOR_SHOP,
	GENERAL_STORE,
	POTION_SHOP,
	SCROLL_SHOP,
	ADVENTURING_GEAR
};

enum class ShopQuality
{
	POOR, // 70% base price
	AVERAGE, // 100% base price
	GOOD, // 130% base price
	EXCELLENT // 160% base price
};

// Forward declarations
struct GameContext;
class Item;
class Creature;

class ShopKeeper : public Persistent
{
private:
	ShopType shopType{ ShopType::GENERAL_STORE };
	ShopQuality shopQuality{ ShopQuality::AVERAGE };
	std::string shopName{};
	FloorInventory shopInventory{ 50 }; // Shop's items for sale
	int markupPercent{ 120 }; // Buy price percentage
	int sellbackPercent{ 60 }; // Sell price percentage

	void generate_shop_name();

	// Random item generation methods
	std::unique_ptr<Item> generate_random_item_by_type(GameContext& ctx);
	std::unique_ptr<Item> generate_random_weapon(GameContext& ctx);
	std::unique_ptr<Item> generate_random_armor(GameContext& ctx);
	std::unique_ptr<Item> generate_random_potion(GameContext& ctx);
	std::unique_ptr<Item> generate_random_scroll(GameContext& ctx);
	std::unique_ptr<Item> generate_random_misc_item(GameContext& ctx);

public:
	ShopKeeper(ShopType type, ShopQuality quality);
	ShopKeeper() = default;

	~ShopKeeper() override = default;
	ShopKeeper(const ShopKeeper&) = delete;
	ShopKeeper& operator=(const ShopKeeper&) = delete;
	ShopKeeper(ShopKeeper&&) = default;
	ShopKeeper& operator=(ShopKeeper&&) = default;

	// Serialization - required by Persistent interface
	void load(const json& j) override;
	void save(json& j) override;

	// Factory method for deserialization
	static std::unique_ptr<ShopKeeper> create(const json& j);

	// Accessors
	ShopType get_shop_type() const noexcept { return shopType; }
	ShopQuality get_shop_quality() const noexcept { return shopQuality; }
	const std::string& get_shop_name() const noexcept { return shopName; }
	FloorInventory& get_shop_inventory() noexcept { return shopInventory; }
	const FloorInventory& get_shop_inventory() const noexcept { return shopInventory; }
	int get_markup_percent() const noexcept { return markupPercent; }
	void set_markup_percent(int value) noexcept { markupPercent = value; }
	int get_sellback_percent() const noexcept { return sellbackPercent; }
	void set_sellback_percent(int value) noexcept { sellbackPercent = value; }

	// Trading operations - simplified for now
	bool can_buy_item(const Item& item) const { return true; } // Accept all items for now
	int get_buy_price(const Item& item) const;
	int get_sell_price(const Item& item) const;

	// Inventory management
	void generate_initial_inventory(GameContext& ctx);

	// Transaction handling
	bool process_player_purchase(GameContext& ctx, Item& item, Creature& player);
	bool process_player_sale(GameContext& ctx, Item& item, Creature& player);

	// Static utility function for creating random shopkeepers
	static std::unique_ptr<ShopKeeper> create_random_shopkeeper();
};
