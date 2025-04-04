#pragma once

#include "Ai/AiMonster.h"
#include "Vector2D.h"

// Forward declarations
class Creature;
class Map;

// Base spider AI class - handles spider movement patterns
class AiSpider : public AiMonster
{
public:
    AiSpider();
    void update(Creature& owner) override;
    void load(const json& j) override;
    void save(json& j) override;

protected:
    int ambushCounter = 0;    // Counter for ambush behavior
    bool isAmbushing = false; // Is this spider currently in ambush mode?
    int poisonCooldown = 0;   // Cooldown for poison attacks

    // Specialized spider movement pattern that prefers walls and corners
    virtual void moveOrAttack(Creature& owner, Vector2D targetPosition);

    // Check if the spider can attempt a poison attack
    bool canPoisonAttack(Creature& owner);

    // Perform a poison attack on the target
    void poisonAttack(Creature& owner, Creature& target);

    // Find the best ambush position near walls
    Vector2D findAmbushPosition(Creature& owner, Vector2D targetPosition);

    // Check if a position is a good ambush spot (near walls)
    bool isGoodAmbushSpot(Vector2D position);
};

// Web spinner spider AI - can create webs to slow down players
class AiWebSpinner : public AiSpider
{
public:
    AiWebSpinner();
    void update(Creature& owner) override;
    void load(const json& j) override;
    void save(json& j) override;

private:
    int webCooldown = 0;       // Cooldown timer for creating webs
    static const int MAX_WEBS = 5; // Maximum number of webs that can exist at once per spider

    // Try to create a web at the current position
    bool tryCreateWeb(Creature& owner);

    // Check if we should create a web (based on player proximity)
    bool shouldCreateWeb(Creature& owner);

    // Generate a complex web pattern
    void generateWebPattern(Vector2D center, int size);

    // Check if a position is valid for placing a web
    bool isValidWebPosition(Vector2D pos);
    void generateWebEntities(Vector2D center, int size);
};