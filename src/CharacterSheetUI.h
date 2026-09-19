#pragma once

#include <string>

#include "BaseMenu.h"

class Creature;
class DataManager;
class Player;
struct GameContext;

namespace CharacterSheetText
{

// The attribute panel's Constitution line: the score and the hit points it adds to
// each level's die. Once the creature's next level no longer takes that adjustment
// (Player's Handbook, PDF page 32), the line names the last level that did.
//
// Example:
//   constitution_line(wizard, dataManager);    // 3rd level, Con 17 -> "CON: 17  (+2 HP/level)"
//   constitution_line(fighter, dataManager);   // 12th level, Con 17 -> "CON: 17  (+3 HP/level through level 9)"
[[nodiscard]] std::string constitution_line(const Creature& creature, const DataManager& dataManager);

} // namespace CharacterSheetText

class CharacterSheetUI : public BaseMenu
{
public:
    CharacterSheetUI(const Player& player);
    ~CharacterSheetUI() = default;
    CharacterSheetUI(const CharacterSheetUI&) = delete;
    CharacterSheetUI& operator=(const CharacterSheetUI&) = delete;
    CharacterSheetUI(CharacterSheetUI&&) = delete;
    CharacterSheetUI& operator=(CharacterSheetUI&&) = delete;

    void menu(GameContext& ctx) override;

private:
    const Player& player_ref;
};
