// file: Tools/BalanceViewer.h
// Developer overlay: shows monster and item spawn distributions for the current dungeon level.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "../Factories/ItemFactory.h"
#include "../Factories/MonsterFactory.h"
#include "../Menu/BaseMenu.h"

struct GameContext;

class BalanceViewer : public BaseMenu
{
public:
    BalanceViewer(int dungeonLevel, GameContext& ctx);
    ~BalanceViewer() = default;
    BalanceViewer(const BalanceViewer&) = delete;
    BalanceViewer& operator=(const BalanceViewer&) = delete;
    BalanceViewer(BalanceViewer&&) = delete;
    BalanceViewer& operator=(BalanceViewer&&) = delete;

    void menu(GameContext& ctx) override;

private:
    int dungeonLevel{};
    std::vector<MonsterPercentage> monsterDist{};
    std::vector<ItemPercentage> itemDist{};

    static void draw_column(
        std::string_view header,
        const std::vector<MonsterPercentage>& dist,
        int startCol,
        GameContext& ctx);
    static void draw_item_column(
        std::string_view header,
        const std::vector<ItemPercentage>& dist,
        int startCol,
        GameContext& ctx);
};

// end of file: Tools/BalanceViewer.h
