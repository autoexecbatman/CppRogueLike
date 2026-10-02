#pragma once

#include "Colors.h"
#include <optional>
#include <string>
#include <vector>

#include "AiMonster.h"
#include "Creature.h"
#include "Persistent.h"

class ContentRegistry;
class ItemRegistry;
struct GameContext;
enum class ItemClass;

// Disguise data for mimic AI -- owned by AiMimic, not by the Creature subclass.
struct Disguise
{
	TileRef tile{};
	std::string name{};
	ColorPairId color{ ColorPairId::WHITE_BLACK };
};

// Build the list of item appearances a mimic can adopt.
// Single source of truth: called at fresh construction AND as lazy init after load.
namespace Appearance
{
[[nodiscard]] std::vector<Disguise> build_mimic_list(ContentRegistry& tiles, const ItemRegistry& items);
} // namespace Appearance

class AiMimic : public AiMonster
{
private:
	bool isDisguised{ true };
	int confusionDuration{ 3 };
	int itemsConsumed{ 0 };
	// The clock reading the next disguise change is due at. Empty until the mimic takes
	// its first turn: MonsterCreator already dressed it, so the rate runs from a turn it
	// actually took rather than from a clock reading it never saw.
	std::optional<int> disguiseChangeTime{};

	// The clock reading the mimic may next swallow something at. Set when it reveals
	// itself, which is the first moment it can eat at all.
	int consumptionReadyTime{ 0 };
	int revealDistance{ 1 };
	std::vector<Disguise> possibleDisguises; // empty on load; rebuilt lazily on first update

	[[nodiscard]] bool consume_nearby_items(Creature& owner, GameContext& ctx);
	void check_revealing(Creature& owner, GameContext& ctx);
	void change_disguise(Creature& owner, GameContext& ctx);

	void apply_item_bonus(Creature& owner, ItemClass itemClass, GameContext& ctx);
	void boost_health(Creature& owner, int amount, GameContext& ctx);
	void boost_defense(Creature& owner, int amount, int maxDR);
	void boost_attack(Creature& owner, GameContext& ctx);
	void boost_confusion_power(GameContext& ctx);
	void transform_to_greater_mimic(Creature& owner, GameContext& ctx);

public:
	AiMimic() = default; // load path: possibleDisguises rebuilt lazily on first update
	explicit AiMimic(std::vector<Disguise> initialDisguises);

	void update(Creature& owner, GameContext& ctx) override;

	[[nodiscard]] AiType get_ai_type() const noexcept override { return AiType::MIMIC; }
	void load(const json& j) override;
	void save(json& j) override;
};
