#pragma once

#include <string>

struct StrengthAttributes
{
    int Str{};
    int hitProb{};
    int dmgAdj{};
    int wgtAllow{};
    int maxPress{};
    int openDoors{};
    double BB_LG{};
    std::string notes{};
};