#include "AiMimic.h"
#include "Game.h"
#include "Colors/Colors.h"
#include "Ai/AiPlayer.h"

AiMimic::AiMimic() : AiMonster(), disguiseChangeCounter(0), consumptionCooldown(0) {}

void AiMimic::update(Creature& owner)
{
    // Cast the owner to a Mimic
    Mimic* mimic = dynamic_cast<Mimic*>(&owner);
    if (!mimic) {
        // This shouldn't happen - but if it does, use standard monster AI
        game.log("Error: AiMimic::update called on non-Mimic creature");
        AiMonster::update(owner);
        return;
    }

    // Skip if mimic is dead
    if (mimic->destructible->is_dead()) {
        return;
    }

    // Check if disguised
    if (mimic->isDisguised) {
        // Increment disguise change counter
        disguiseChangeCounter++;

        // Occasionally change disguise if still hidden
        if (disguiseChangeCounter >= disguiseChangeRate) {
            changeDisguise(*mimic);
            disguiseChangeCounter = 0;
            game.log("Mimic changed disguise");
        }

        // Check if player is close enough to reveal
        checkRevealing(*mimic);
    }
    else {
        // Not disguised - can consume items and move
        consumeNearbyItems(*mimic);

        // Use standard monster AI for movement and attacks
        AiMonster::update(owner);
    }
}

bool AiMimic::consumeNearbyItems(Mimic& mimic)
{
    // Only consuming items when revealed and active
    if (mimic.isDisguised || mimic.destructible->is_dead()) {
        return false;
    }

    // Add cooldown mechanic - can only consume items every few turns
    consumptionCooldown++;
    if (consumptionCooldown < 3) {
        return false;
    }

    // Reset cooldown
    consumptionCooldown = 0;

    // Check for items in a 1-tile radius
    bool itemConsumed = false;
    std::vector<size_t> itemsToRemove;

    game.log("Checking for items. Container size: " + std::to_string(game.container->inv.size()));

    // First pass - identify items to consume
    for (size_t i = 0; i < game.container->inv.size(); i++) {
        auto& item = game.container->inv[i];
        if (!item) {
            continue;
        }

        int itemDistance = mimic.get_tile_distance(item->position);
        game.log("Item at distance " + std::to_string(itemDistance) + ": " + item->actorData.name);

        if (itemDistance <= 1) {
            // Found an item to consume!
            game.appendMessagePart(DRAGON_PAIR, "The mimic ");
            game.appendMessagePart(WHITE_PAIR, "consumes the ");
            game.appendMessagePart(item->actorData.color, item->actorData.name);
            game.appendMessagePart(WHITE_PAIR, "!");
            game.finalizeMessage();

            game.log("Mimic consuming item: " + item->actorData.name);

            // Grow stronger based on item type
            mimic.itemsConsumed++;

            // Different bonuses based on item type
            if (item->actorData.name.find("potion") != std::string::npos) {
                // Health potions give the mimic more HP
                mimic.destructible->hpMax += 1;
                mimic.destructible->hp += 1;
                game.log("Mimic gained 1 HP from potion");
            }
            else if (item->actorData.name.find("scroll") != std::string::npos) {
                // Scrolls increase confusion duration
                mimic.confusionDuration = std::min(mimic.confusionDuration + 1, 5);
                game.log("Mimic increased confusion duration to " + std::to_string(mimic.confusionDuration));
            }
            else if (item->actorData.name.find("gold") != std::string::npos) {
                // Gold makes the mimic more defensive
                if (mimic.destructible->dr < 2) {
                    mimic.destructible->dr += 1;
                    game.log("Mimic gained 1 DR from gold");
                }
            }
            else if (item->actorData.name.find("weapon") != std::string::npos ||
                item->actorData.name.find("sword") != std::string::npos ||
                item->actorData.name.find("dagger") != std::string::npos) {
                // Weapons make the mimic hit harder
                if (mimic.attacker->roll != "D6") {
                    int currentDamage = game.d.roll_from_string(mimic.attacker->roll);
                    mimic.attacker->roll = "D" + std::to_string(std::min(currentDamage + 1, 6));
                    game.log("Mimic improved attack to " + mimic.attacker->roll);
                }
            }

            // Mark item for removal
            itemsToRemove.push_back(i);
            itemConsumed = true;

            // If mimic has consumed enough items, change appearance
            if (mimic.itemsConsumed >= 5) {
                mimic.actorData.ch = 'W';
                mimic.actorData.color = FIREBALL_PAIR;
                mimic.actorData.name = "greater mimic";
                game.log("Mimic transformed into greater mimic");
            }

            // Only consume one item per turn to give player a chance
            break;
        }
    }

    // Second pass - remove consumed items (in reverse order to avoid index issues)
    for (auto it = itemsToRemove.rbegin(); it != itemsToRemove.rend(); ++it) {
        size_t index = *it;
        game.log("Removing consumed item at index " + std::to_string(index));

        if (index < game.container->inv.size()) {
            game.container->inv[index].reset();
            game.container->inv.erase(game.container->inv.begin() + index);
        }
    }

    return itemConsumed;
}

void AiMimic::checkRevealing(Mimic& mimic)
{
    // Calculate distance to player
    int distanceToPlayer = mimic.get_tile_distance(game.player->position);
    game.log("Mimic distance to player: " + std::to_string(distanceToPlayer));

    // Check if player is close enough to reveal
    if (distanceToPlayer <= mimic.revealDistance) {
        // Reveal true form!
        mimic.isDisguised = false;
        mimic.actorData.ch = 'M';
        mimic.actorData.name = "mimic";
        mimic.actorData.color = DRAGON_PAIR;
        mimic.add_state(ActorState::BLOCKS); // Now it's solid

        game.log("Mimic revealed itself!");

        // Try to confuse the player
        if (game.d.d20() > game.player->wisdom) {
            game.appendMessagePart(CONFUSION_PAIR, "The ");
            game.appendMessagePart(DRAGON_PAIR, "mimic");
            game.appendMessagePart(CONFUSION_PAIR, " reveals itself and confuses you!");
            game.finalizeMessage();

            // Apply confusion to player
            game.player->add_state(ActorState::IS_CONFUSED);

            // Apply confusion through player's AI
            auto playerAi = dynamic_cast<AiPlayer*>(game.player->ai.get());
            if (playerAi) {
                playerAi->applyConfusion(mimic.confusionDuration);
                game.log("Applied confusion to player for " + std::to_string(mimic.confusionDuration) + " turns");
            }
        }
        else {
            game.appendMessagePart(DRAGON_PAIR, "A mimic");
            game.appendMessagePart(WHITE_PAIR, " reveals itself but you resist its confusion!");
            game.finalizeMessage();
            game.log("Player resisted mimic confusion");
        }
    }
}

void AiMimic::changeDisguise(Mimic& mimic)
{
    if (!mimic.isDisguised) return; // Don't change if already revealed

    // Select a random disguise from the mimic's disguise list
    int index = game.d.roll(0, mimic.possibleDisguises.size() - 1);
    const auto& chosen = mimic.possibleDisguises[index];

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

    // Load AiMimic specific data if needed
    if (j.contains("disguiseChangeCounter")) {
        disguiseChangeCounter = j.at("disguiseChangeCounter").get<int>();
    }

    if (j.contains("consumptionCooldown")) {
        consumptionCooldown = j.at("consumptionCooldown").get<int>();
    }
}

void AiMimic::save(json& j)
{
    AiMonster::save(j);

    // Save AiMimic specific data
    j["disguiseChangeCounter"] = disguiseChangeCounter;
    j["consumptionCooldown"] = consumptionCooldown;
}