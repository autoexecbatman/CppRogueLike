#include "AiMimic.h"
#include "../Game.h"
#include "../Colors/Colors.h"
#include "../Ai/AiPlayer.h"
#include "../Actor/InventoryOperations.h"
#include "../Items/ItemClassification.h"

using namespace InventoryOperations; // For clean function calls

AiMimic::AiMimic() : AiMonster(), disguiseChangeCounter(0), consumptionCooldown(0) {}

void AiMimic::update(Creature& owner)
{
    // Cast the owner to a Mimic - we still need this for type-specific operations
    Mimic* mimic = dynamic_cast<Mimic*>(&owner);
    if (!mimic)
    {
        // This shouldn't happen - but if it does, use standard monster AI
        game.log("Error: AiMimic::update called on non-Mimic creature");
        AiMonster::update(owner);
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
        if (disguiseChangeCounter >= disguiseChangeRate)
        {
            change_disguise(*mimic);
            disguiseChangeCounter = 0;
            game.log("Mimic changed disguise");
        }

        // Check if player is close enough to reveal
        check_revealing(*mimic);
    }
    else
    {
        // Not disguised - can consume items and move
        consume_nearby_items(*mimic);

        // Use standard monster AI for movement and attacks
        AiMonster::update(owner);
    }
}

bool AiMimic::consume_nearby_items(Mimic& mimic)
{
    // Only consuming items when revealed and active
    if (isDisguised || mimic.destructible->is_dead())
    {
        return false;
    }

    // Add cooldown mechanic - can only consume items every few turns
    consumptionCooldown++;
    if (consumptionCooldown < 3)
    {
        return false;
    }

    // Reset cooldown
    consumptionCooldown = 0;

    // Check for items in a 1-tile radius
    bool itemConsumed = false;
    std::vector<size_t> itemsToRemove;

    // Check if inventory has no items
	if (game.inventory_data.items.empty())
    {
        return false;
    }

    game.log("Checking for items. Inventory size: " + std::to_string(game.inventory_data.items.size()));

	// First pass - identify items to consume
	for (size_t i = 0; i < game.inventory_data.items.size(); i++)
	{
		auto& item = game.inventory_data.items[i];
		if (!item) continue;

        int itemDistance = mimic.get_tile_distance(item->position);
        game.log("Item at distance " + std::to_string(itemDistance) + ": " + item->actorData.name);

        if (itemDistance <= 1)
        {
            // Found an item to consume - check if it has a pickable component
            if (!item->pickable)
            {
                game.log("Mimic found non-pickable item, skipping: " + item->actorData.name);
                continue;
            }
            
            // Found an item to consume!
            game.append_message_part(RED_YELLOW_PAIR, "The mimic ");
            game.append_message_part(WHITE_BLACK_PAIR, "consumes the ");
            game.append_message_part(item->actorData.color, item->actorData.name);
            game.append_message_part(WHITE_BLACK_PAIR, "!");
            game.finalize_message();

            game.log("Mimic consuming item: " + item->actorData.name);

            // Grow stronger based on item type using PickableTypeRegistry
            itemsConsumed++;
            
            // Get item type using the ItemClass system (what the item IS, not what component it uses)
            ItemClass itemClass = item->itemClass;

            // Different bonuses based on item type
            if (itemClass == ItemClass::HEALTH_POTION)
            {
                // Health potions give the mimic more HP
                mimic.destructible->set_max_hp(mimic.destructible->get_max_hp() + 1);
                mimic.destructible->set_hp(mimic.destructible->get_hp() + 1);
                game.log("Mimic gained 1 HP from potion");
            }
            else if (itemClass == ItemClass::SCROLL_LIGHTNING ||
                     itemClass == ItemClass::SCROLL_FIREBALL ||
                     itemClass == ItemClass::SCROLL_CONFUSION)
            {
                // Scrolls increase confusion duration
                confusionDuration = std::min(confusionDuration + 1, 5);
                game.log("Mimic increased confusion duration to " + std::to_string(confusionDuration));
            }
            else if (itemClass == ItemClass::GOLD)
            {
                // Gold makes the mimic more defensive
                if (mimic.destructible->get_dr() < 2)
                {
                    mimic.destructible->set_dr(mimic.destructible->get_dr() + 1);
                    game.log("Mimic gained 1 DR from gold");
                }
            }
            else if (itemClass == ItemClass::DAGGER ||
                     itemClass == ItemClass::SHORT_SWORD ||
                     itemClass == ItemClass::LONG_SWORD ||
                     itemClass == ItemClass::GREAT_SWORD ||
                     itemClass == ItemClass::BATTLE_AXE ||
                     itemClass == ItemClass::GREAT_AXE ||
                     itemClass == ItemClass::WAR_HAMMER ||
                     itemClass == ItemClass::STAFF ||
                     itemClass == ItemClass::LONG_BOW)
            {
                // Weapons make the mimic hit harder - use proper damage system
                const auto& currentDamage = mimic.attacker->get_damage_info();
                if (currentDamage.maxDamage < 6)
                {
                    DamageInfo improvedDamage(currentDamage.minDamage, std::min(currentDamage.maxDamage + 1, 6),
                                            "1d" + std::to_string(std::min(currentDamage.maxDamage + 1, 6)));
                    mimic.attacker->set_damage_info(improvedDamage);
                    game.log("Mimic improved attack to " + improvedDamage.displayRoll);
                }
            }
            else if (itemClass == ItemClass::LEATHER_ARMOR ||
                     itemClass == ItemClass::CHAIN_MAIL ||
                     itemClass == ItemClass::PLATE_MAIL)
            {
                // Armor increases defense
                if (mimic.destructible->get_dr() < 3)
                {
                    mimic.destructible->set_dr(mimic.destructible->get_dr() + 1);
                    game.log("Mimic gained 1 DR from armor");
                }
            }
            else if (itemClass == ItemClass::FOOD_RATION ||
                     itemClass == ItemClass::BREAD ||
                     itemClass == ItemClass::MEAT ||
                     itemClass == ItemClass::FRUIT)
            {
                // Food provides general health boost
                mimic.destructible->set_max_hp(mimic.destructible->get_max_hp() + 1);
                mimic.destructible->set_hp(std::min(mimic.destructible->get_hp() + 1, mimic.destructible->get_max_hp()));
                game.log("Mimic gained health from food");
            }
            else
            {
                // Unknown item type - provide minimal benefit
                game.log("Mimic consumed unknown item type: " + item->get_name());
            }

            // Mark item for removal
            itemsToRemove.push_back(i);
            itemConsumed = true;

            // If mimic has consumed enough items, change appearance
            if (itemsConsumed >= 5)
            {
                mimic.actorData.ch = 'W';
                mimic.actorData.color = RED_YELLOW_PAIR;
                mimic.actorData.name = "greater mimic";
                game.log("Mimic transformed into greater mimic");
            }

            // Only consume one item per turn to give player a chance
            break;
        }
    }

    // Second pass - remove consumed items using proper inventory operations
    for (auto it = itemsToRemove.rbegin(); it != itemsToRemove.rend(); ++it)
    {
        size_t index = *it;
        game.log("Removing consumed item at index " + std::to_string(index));

        if (index < game.inventory_data.items.size() && game.inventory_data.items[index])
        {
            remove_item_at(game.inventory_data, index);
        }
    }

    return itemConsumed;
}

void AiMimic::check_revealing(Mimic& mimic)
{
    // Calculate distance to player
    int distanceToPlayer = mimic.get_tile_distance(game.player->position);
    game.log("Mimic distance to player: " + std::to_string(distanceToPlayer));

    // Check if player is close enough to reveal - using our internal revealDistance
    if (distanceToPlayer <= revealDistance)
    {
        // Reveal true form!
        isDisguised = false;
        mimic.actorData.ch = 'M';
        mimic.actorData.name = "mimic";
        mimic.actorData.color = RED_YELLOW_PAIR;
        mimic.add_state(ActorState::BLOCKS); // Now it's solid

        game.log("Mimic revealed itself!");

        // Try to confuse the player
        if (game.d.d20() > game.player->get_wisdom())
        {
            game.append_message_part(WHITE_GREEN_PAIR, "The ");
            game.append_message_part(RED_YELLOW_PAIR, "mimic");
            game.append_message_part(WHITE_GREEN_PAIR, " reveals itself and confuses you!");
            game.finalize_message();

            // Apply confusion to player
            game.player->add_state(ActorState::IS_CONFUSED);

            // Apply confusion through player's AI
            auto playerAi = dynamic_cast<AiPlayer*>(game.player->ai.get());
            if (playerAi)
            {
                // Using our confusionDuration for consistency
                playerAi->applyConfusion(confusionDuration);
                game.log("Applied confusion to player for " + std::to_string(confusionDuration) + " turns");
            }
        }
        else
        {
            game.append_message_part(RED_YELLOW_PAIR, "A mimic");
            game.append_message_part(WHITE_BLACK_PAIR, " reveals itself but you resist its confusion!");
            game.finalize_message();
            game.log("Player resisted mimic confusion");
        }
    }
}

void AiMimic::change_disguise(Mimic& mimic)
{
    // Don't change if already revealed - using our isDisguised property
    if (!isDisguised) return;

    // Get the possible disguises from the mimic
    auto possibleDisguises = mimic.get_possible_disguises();

    // Select a random disguise
    int index = game.d.roll(0, possibleDisguises.size() - 1);
    const auto& chosen = possibleDisguises[index];

    // Apply the disguise
    mimic.actorData.ch = chosen.ch;
    mimic.actorData.name = chosen.name;
    mimic.actorData.color = chosen.color;

    // Remove the BLOCKS state while disguised
    mimic.remove_state(ActorState::BLOCKS);
}

void AiMimic::load(const json& j)
{
    AiMonster::load(j);

    // Load AiMimic specific data
    if (j.contains("disguiseChangeCounter")) {
        disguiseChangeCounter = j.at("disguiseChangeCounter").get<int>();
    }

    if (j.contains("consumptionCooldown")) {
        consumptionCooldown = j.at("consumptionCooldown").get<int>();
    }

    if (j.contains("isDisguised")) {
        isDisguised = j.at("isDisguised").get<bool>();
    }

    if (j.contains("revealDistance")) {
        revealDistance = j.at("revealDistance").get<int>();
    }

    if (j.contains("confusionDuration")) {
        confusionDuration = j.at("confusionDuration").get<int>();
    }

    if (j.contains("itemsConsumed")) {
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