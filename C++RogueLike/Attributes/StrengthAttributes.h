#pragma once

#include <string>
#include <vector>

struct StrengthAttributes {
    int Str{};
    int hitProb{};
    int dmgAdj{};
    int wgtAllow{};
    int maxPress{};
    int openDoors{};
    double BB_LG{};
    std::string notes{};

    void print_chart();
};

std::vector<StrengthAttributes> loadStrengthAttributes();