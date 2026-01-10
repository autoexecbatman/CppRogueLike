#pragma once

#include "../Actor/Actor.h"
#include "../Colors/Colors.h"

// Web class - represents a spider web that can trap players
class Web : public Object
{
public:
    Web(Vector2D position, int strength = 2);

    // Web properties
    int getStrength() const { return webStrength; }
    void setStrength(int strength) { webStrength = strength; }

    // Apply web effect to a creature trying to pass through
    bool applyEffect(Creature& creature, GameContext& ctx);

    // Destroy this web
    void destroy(GameContext& ctx);

private:
    int webStrength;    // How strong the web is (affects escape difficulty)
};