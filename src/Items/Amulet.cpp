#include "Amulet.h"
#include "../Game.h"
#include "../Colors/Colors.h"

Amulet::Amulet() {}

bool Amulet::use(Item& owner, Creature& wearer, GameContext& ctx) {
    // When used, trigger the victory condition
    ctx.message_system->message(WHITE_BLACK_PAIR, "The Amulet of Yendor glows brightly in your hands!", true);
    ctx.message_system->message(WHITE_BLACK_PAIR, "You feel a powerful magic enveloping you...", true);

    // Set the game status to victory
    *ctx.game_status = GameStatus::VICTORY;

    // Don't consume the item
    return false;
}

void Amulet::load(const json& j) {
    // Nothing to load for the Amulet
}

void Amulet::save(json& j) {
    j["type"] = static_cast<int>(PickableType::AMULET);
}