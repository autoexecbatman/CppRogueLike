#pragma once

#include "../../Actor/Actor.h"

// Forward declarations
class Vector2D;
struct GameContext;

// Enumeration for different spider types
enum class SpiderType
{
    SMALL,      // Small spiders - faster but weaker
    GIANT,      // Giant spiders - stronger, can poison
    WEB_SPINNER // Web spinner spiders - can create webs to slow down players
};

// Base Spider class
class Spider : public Creature
{
public:
    Spider(Vector2D position, GameContext& ctx, SpiderType type = SpiderType::SMALL);

    // Override update if needed for spider-specific behavior
    void update(GameContext& ctx);

    // Poison attack chance based on spider type
    int get_poison_chance() const;

    // Get spider type
    SpiderType get_spider_type() const { return spiderType; }

private:
    SpiderType spiderType;

    // Initialize based on spider type
    void init_spider_type(GameContext& ctx);
};

// Small Spider - faster but weaker
class SmallSpider : public Spider
{
public:
    SmallSpider(Vector2D position, GameContext& ctx);
};

// Giant Spider - stronger, can poison
class GiantSpider : public Spider
{
public:
    GiantSpider(Vector2D position, GameContext& ctx);
};

// Web Spinner Spider - can create webs
class WebSpinner : public Spider
{
public:
    WebSpinner(Vector2D position, GameContext& ctx);
};