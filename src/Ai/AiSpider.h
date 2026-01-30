#pragma once

#include "AiMonster.h"

// Forward declarations
class Creature;
struct Vector2D;
struct GameContext;

// Base spider AI class - handles spider movement patterns
class AiSpider : public AiMonster
{
public:
    void update(Creature& owner, GameContext& ctx) override;
    void move_toward_player(Creature& owner, GameContext& ctx);
    void random_move(Creature& owner, GameContext& ctx);
    void load(const json& j) override;
    void save(json& j) override;

    // Functions moved from Spider class
    bool has_laid_web() const { return webLaid; }
    void set_web_laid(bool status) { webLaid = status; }

protected:
    int ambushCounter{ 0 };    // Counter for ambush behavior
    bool isAmbushing{ false }; // Is this spider currently in ambush mode?
    int poisonCooldown{ 0 };   // Cooldown for poison attacks
    bool webLaid{ false };     // Tracks if this spider has created a web

    // Specialized spider movement pattern that prefers walls and corners
    void move_or_attack(Creature& owner, Vector2D targetPosition, GameContext& ctx) override;

    // Check if the spider can attempt a poison attack
    bool can_poison_attack(Creature& owner, GameContext& ctx);

    // Perform a poison attack on the target
    void poison_attack(Creature& owner, Creature& target, GameContext& ctx);

    // Find the best ambush position near walls
    Vector2D find_ambush_position(Creature& owner, Vector2D targetPosition, GameContext& ctx);

    // Check if a position is a good ambush spot (near walls)
    bool is_good_ambush_spot(Vector2D position, GameContext& ctx);
};

// Web spinner spider AI - can create webs to slow down players
class AiWebSpinner : public AiSpider
{
public:
    void update(Creature& owner, GameContext& ctx) override;
    void load(const json& j) override;
    void save(json& j) override;

private:
    int webCooldown{ 0 };       // Cooldown timer for creating webs
    static constexpr int MAX_WEBS{ 5 }; // Maximum number of webs that can exist at once per spider

    // Try to create a web at the current position
    bool try_create_web(Creature& owner, GameContext& ctx);

    // Check if we should create a web (based on player proximity)
    bool should_create_web(Creature& owner, GameContext& ctx);

    // Generate a complex web pattern with entities (CIRCULAR, SPIRAL, RADIAL, CHAOTIC)
    void generate_web_entities(Vector2D center, int size, GameContext& ctx);

    // Check if a position is valid for placing a web
    bool is_valid_web_position(Vector2D pos, GameContext& ctx);
};