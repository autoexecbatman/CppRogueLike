#include <format>
#include <ranges>  // For std::views::reverse

#include "AiMimic.h"
#include "../Colors/Colors.h"
#include "../Ai/AiPlayer.h"
#include "../Actor/InventoryOperations.h"
#include "../Items/ItemClassification.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"
#include "../ActorTypes/Player.h"
#include "../ActorTypes/Monsters.h"

using namespace InventoryOperations; // For clean function calls

// OCP-compliant: data-driven item bonus mapping
enum class MimicBonusType
{
	HEALTH,
	CONFUSION,
	ATTACK,
	DEFENSE_GOLD,
	DEFENSE_ARMOR,
	NONE
};

static const std::unordered_map<ItemClass, MimicBonusType> item_bonus_map = {
	{ItemClass::POTION, MimicBonusType::HEALTH},
	{ItemClass::FOOD, MimicBonusType::HEALTH},
	{ItemClass::SCROLL, MimicBonusType::CONFUSION},
	{ItemClass::GOLD, MimicBonusType::DEFENSE_GOLD},
	{ItemClass::ARMOR, MimicBonusType::DEFENSE_ARMOR},
	{ItemClass::DAGGER, MimicBonusType::ATTACK},
	{ItemClass::SWORD, MimicBonusType::ATTACK},
	{ItemClass::GREAT_SWORD, MimicBonusType::ATTACK},
	{ItemClass::AXE, MimicBonusType::ATTACK},
	{ItemClass::HAMMER, MimicBonusType::ATTACK},
	{ItemClass::MACE, MimicBonusType::ATTACK},
	{ItemClass::STAFF, MimicBonusType::ATTACK},
	{ItemClass::BOW, MimicBonusType::ATTACK},
	{ItemClass::CROSSBOW, MimicBonusType::ATTACK},
};

void AiMimic::update(Creature& owner, GameContext& ctx)
{
    // Cast the owner to a Mimic - we still need this for type-specific operations
    Mimic* mimic = dynamic_cast<Mimic*>(&owner);
    if (!mimic)
    {
        // This shouldn't happen - but if it does, use standard monster AI
        ctx.message_system->log("Error: AiMimic::update called on non-Mimic creature");
        AiMonster::update(owner, ctx);
        return;
    }

    // Skip if mimic is dead
    if (mimic->destructible->is_dead())
    {
        return;
    }

    // Check if disguised - now using our internal state
    if (isDisguised)
    {
        // Increment disguise change counter
        disguiseChangeCounter++;

        // Occasionally change disguise if still hidden
        if (disguiseChangeCounter >= DISGUISE_CHANGE_RATE)
        {
            change_disguise(*mimic, ctx);
            disguiseChangeCounter = 0;
            ctx.message_system->log("Mimic changed disguise");
        }

        // Check if player is close enough to reveal
        check_revealing(*mimic, ctx);
    }
    else
    {
        // Not disguised - can consume items and move
        const bool itemConsumed = consume_nearby_items(*mimic, ctx);
        
        // Only move/attack if we didn't just consume an item
        if (!itemConsumed)
        {
            // Use standard monster AI for movement and attacks
            AiMonster::update(owner, ctx);
        }
    }
}

[[nodiscard]] bool AiMimic::consume_nearby_items(Mimic& mimic, GameContext& ctx)
{
    // Only consume items when revealed and active
    if (isDisguised || mimic.destructible->is_dead())
    {
        return false;
    }

    // Apply cooldown mechanic
    ++consumptionCooldown;
    if (consumptionCooldown < CONSUMPTION_COOLDOWN_TURNS)
    {
        return false;
    }
    consumptionCooldown = 0;

    // Early exit if no items exist
    if (ctx.inventory_data->items.empty())
    {
        return false;
    }

    ctx.message_system->log(std::format("Checking for items. Inventory size: {}", ctx.inventory_data->items.size()));

    // Find and consume one item within range
    std::vector<size_t> itemsToRemove;
    bool itemConsumed = false;

    for (size_t i = 0; i < ctx.inventory_data->items.size(); ++i)
    {
        const auto& item = ctx.inventory_data->items[i];
        if (!item) continue;

        const int itemDistance = mimic.get_tile_distance(item->position);
        ctx.message_system->log(std::format("Item at distance {}: {}", itemDistance, item->actorData.name));

        if (itemDistance <= CONSUMPTION_RADIUS)
        {
            // Verify item is pickable
            if (!item->pickable)
            {
                ctx.message_system->log(std::format("Mimic found non-pickable item, skipping: {}", item->actorData.name));
                continue;
            }

            // Display consumption message
            ctx.message_system->append_message_part(RED_YELLOW_PAIR, "The mimic ");
            ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "consumes the ");
            ctx.message_system->append_message_part(item->actorData.color, item->actorData.name);
            ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "!");
            ctx.message_system->finalize_message();

            ctx.message_system->log(std::format("Mimic consuming item: {}", item->actorData.name));

            // Apply item-specific bonuses
            ++itemsConsumed;
            apply_item_bonus(mimic, item->itemClass, ctx);

            // Check for transformation
            if (itemsConsumed >= ITEMS_FOR_TRANSFORMATION)
            {
                transform_to_greater_mimic(mimic, ctx);
            }

            // Mark for removal and stop (one item per turn)
            itemsToRemove.push_back(i);
            itemConsumed = true;
            break;
        }
    }

    // Remove consumed items in reverse order to maintain indices
    for (size_t index : std::views::reverse(itemsToRemove))
    {
        ctx.message_system->log(std::format("Removing consumed item at index {}", index));
        
        if (index < ctx.inventory_data->items.size() && ctx.inventory_data->items[index])
        {
            remove_item_at(*ctx.inventory_data, index);
        }
    }

    return itemConsumed;
}

// OCP-compliant: data-driven item bonus application
void AiMimic::apply_item_bonus(Mimic& mimic, ItemClass itemClass, GameContext& ctx)
{
    if (!item_bonus_map.contains(itemClass))
    {
        return; // Unknown item class - no bonus
    }

    const MimicBonusType bonusType = item_bonus_map.at(itemClass);

    switch (bonusType)
    {
        case MimicBonusType::HEALTH:
            boost_health(mimic, HEALTH_BONUS, ctx);
            ctx.message_system->log(std::format("Mimic gained {} HP", HEALTH_BONUS));
            break;

        case MimicBonusType::CONFUSION:
            boost_confusion_power(ctx);
            break;

        case MimicBonusType::DEFENSE_GOLD:
            boost_defense(mimic, DR_BONUS, MAX_GOLD_DR_BONUS, ctx);
            if (mimic.destructible->get_dr() <= MAX_GOLD_DR_BONUS)
            {
                ctx.message_system->log(std::format("Mimic gained {} DR from gold", DR_BONUS));
            }
            break;

        case MimicBonusType::DEFENSE_ARMOR:
            boost_defense(mimic, DR_BONUS, MAX_ARMOR_DR_BONUS, ctx);
            if (mimic.destructible->get_dr() <= MAX_ARMOR_DR_BONUS)
            {
                ctx.message_system->log(std::format("Mimic gained {} DR from armor", DR_BONUS));
            }
            break;

        case MimicBonusType::ATTACK:
            boost_attack(mimic, ctx);
            break;

        case MimicBonusType::NONE:
            break;
    }
}

void AiMimic::boost_health(Mimic & mimic, int amount, GameContext & ctx)
{
    mimic.destructible->set_max_hp(mimic.destructible->get_max_hp() + amount);
    const int newCurrentHp = std::min(
        mimic.destructible->get_hp() + amount,
        mimic.destructible->get_max_hp()
    );
    mimic.destructible->set_hp(newCurrentHp);
    ctx.message_system->log(std::format("Mimic gained {} health from food", amount));
}

void AiMimic::boost_defense(Mimic& mimic, int amount, int maxDR, GameContext& ctx)
{
    const int currentDR = mimic.destructible->get_dr();
    if (currentDR < maxDR)
    {
        mimic.destructible->set_dr(currentDR + amount);
    }
}

void AiMimic::boost_attack(Mimic& mimic, GameContext& ctx)
{
    const auto& currentDamage = mimic.attacker->get_damage_info();
    if (currentDamage.maxDamage < MAX_WEAPON_DAMAGE)
    {
        const int newMaxDamage = std::min(currentDamage.maxDamage + 1, MAX_WEAPON_DAMAGE);
        const DamageInfo improvedDamage(
            currentDamage.minDamage,
            newMaxDamage,
            std::format("1d{}", newMaxDamage)
        );
        mimic.attacker->set_damage_info(improvedDamage);
        ctx.message_system->log(std::format("Mimic improved attack to {}", improvedDamage.displayRoll));
    }
}

void AiMimic::boost_confusion_power(GameContext& ctx)
{
    confusionDuration = std::min(confusionDuration + CONFUSION_BONUS, MAX_CONFUSION_DURATION);
    ctx.message_system->log(std::format("Mimic increased confusion duration to {}", confusionDuration));
}

void AiMimic::transform_to_greater_mimic(Mimic& mimic, GameContext& ctx)
{
    mimic.actorData.ch = 'W';
    mimic.actorData.color = RED_YELLOW_PAIR;
    mimic.actorData.name = "greater mimic";
    ctx.message_system->log("Mimic transformed into greater mimic");
}

void AiMimic::check_revealing(Mimic& mimic, GameContext& ctx)
{
    // Calculate distance to player
    int distanceToPlayer = mimic.get_tile_distance(ctx.player->position);
    ctx.message_system->log("Mimic distance to player: " + std::to_string(distanceToPlayer));

    // Check if player is close enough to reveal - using our internal revealDistance
    if (distanceToPlayer <= revealDistance)
    {
        // Reveal true form!
        isDisguised = false;
        mimic.actorData.ch = 'M';
        mimic.actorData.name = "mimic";
        mimic.actorData.color = RED_YELLOW_PAIR;
        mimic.add_state(ActorState::BLOCKS); // Now it's solid

        ctx.message_system->log("Mimic revealed itself!");

        // Try to confuse the player
        if (ctx.dice->d20() > ctx.player->get_wisdom())
        {
            ctx.message_system->append_message_part(WHITE_GREEN_PAIR, "The ");
            ctx.message_system->append_message_part(RED_YELLOW_PAIR, "mimic");
            ctx.message_system->append_message_part(WHITE_GREEN_PAIR, " reveals itself and confuses you!");
            ctx.message_system->finalize_message();

            // Apply confusion to player
            ctx.player->add_state(ActorState::IS_CONFUSED);

            // Apply confusion through player's AI
            auto playerAi = dynamic_cast<AiPlayer*>(ctx.player->ai.get());
            if (playerAi)
            {
                // Using our confusionDuration for consistency
                playerAi->applyConfusion(confusionDuration);
                ctx.message_system->log("Applied confusion to player for " + std::to_string(confusionDuration) + " turns");
            }
        }
        else
        {
            ctx.message_system->append_message_part(RED_YELLOW_PAIR, "A mimic");
            ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " reveals itself but you resist its confusion!");
            ctx.message_system->finalize_message();
            ctx.message_system->log("Player resisted mimic confusion");
        }
    }
}

void AiMimic::change_disguise(Mimic& mimic, GameContext& ctx)
{
    // Don't change if already revealed - using our isDisguised property
    if (!isDisguised) return;

    // Get the possible disguises from the mimic
    const auto& possibleDisguises = mimic.get_possible_disguises();

    // Select a random disguise
    if (!possibleDisguises.empty())
    {
        const size_t index = ctx.dice->roll(0, static_cast<int>(possibleDisguises.size()) - 1);
        const auto& chosen = possibleDisguises.at(index);
        // Apply the disguise
        mimic.actorData.ch = chosen.ch;
        mimic.actorData.name = chosen.name;
        mimic.actorData.color = chosen.color;
    }
    else
    {
        throw "possibleDisguises is empty in change_disguise()";
    }

    // Remove the BLOCKS state while disguised
    mimic.remove_state(ActorState::BLOCKS);
}

void AiMimic::load(const json& j)
{
    AiMonster::load(j);

    // Load AiMimic specific data
    if (j.contains("disguiseChangeCounter"))
    {
        disguiseChangeCounter = j.at("disguiseChangeCounter").get<int>();
    }

    if (j.contains("consumptionCooldown"))
    {
        consumptionCooldown = j.at("consumptionCooldown").get<int>();
    }

    if (j.contains("isDisguised"))
    {
        isDisguised = j.at("isDisguised").get<bool>();
    }

    if (j.contains("revealDistance"))
    {
        revealDistance = j.at("revealDistance").get<int>();
    }

    if (j.contains("confusionDuration"))
    {
        confusionDuration = j.at("confusionDuration").get<int>();
    }

    if (j.contains("itemsConsumed"))
    {
        itemsConsumed = j.at("itemsConsumed").get<int>();
    }
}

void AiMimic::save(json& j)
{
    AiMonster::save(j);

    // Save AiMimic specific data
    j["disguiseChangeCounter"] = disguiseChangeCounter;
    j["consumptionCooldown"] = consumptionCooldown;
    j["isDisguised"] = isDisguised;
    j["revealDistance"] = revealDistance;
    j["confusionDuration"] = confusionDuration;
    j["itemsConsumed"] = itemsConsumed;
}