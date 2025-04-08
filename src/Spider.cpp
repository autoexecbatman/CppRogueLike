#include "Spider.h"
#include "Game.h"
#include "AiSpider.h"
#include "Random/RandomDice.h"

// ActorData definitions for different spider types
ActorData smallSpiderData
{
    's',
    "small spider",
    TROLL_PAIR  // Using existing color pairs - green on black
};

ActorData giantSpiderData
{
    'S',
    "giant spider",
    ORC_PAIR  // Using existing color pairs - red on black
};

ActorData webSpinnerData
{
    'W',
    "web weaver",  // More impressive name
    SPIDER_PAIR  // Using existing color pairs - green on red
};

// Base Spider constructor
Spider::Spider(Vector2D position, SpiderType type)
    : Creature(position, smallSpiderData), // Default to small spider data
    spiderType(type)
{
    // Initialize based on spider type
    init_spider_type();
}

void Spider::init_spider_type()
{
    RandomDice d;

    // Common spider traits
    add_state(ActorState::CAN_SWIM); // Spiders can walk on water (they're light)

    switch (spiderType)
    {
    case SpiderType::SMALL:
        // Update actor data for small spider
        actorData = smallSpiderData;

        // Stats for small spider
        strength = d.d6() + d.d6() + d.d6();  // Minimum strength of 3
        dexterity = d.d6() + d.d6() + d.d6();  // Small spiders are very agile
        constitution = d.d6();

        // Combat properties
        destructible = std::make_unique<MonsterDestructible>(d.d2() + 2, 0, "dead small spider", 15, 20, 7);
        attacker = std::make_unique<Attacker>("D4");

        // AI - use spider AI for intelligent movement
        ai = std::make_unique<AiSpider>();
        break;

    case SpiderType::GIANT:
        // Update actor data for giant spider
        actorData = giantSpiderData;

        // Stats for giant spider
        strength = d.d6() + d.d6() + d.d6();
        dexterity = d.d6() + d.d6() + d.d6();
        constitution = d.d6() + 1;

        // Combat properties - giant spiders have more HP and do more damage

        destructible = std::make_unique<MonsterDestructible>(
            d.d4() + 3,
            1,
            "dead giant spider",
            40,
            19,
            5
        );
        attacker = std::make_unique<Attacker>("D4");

        // AI - use spider AI for intelligent movement
        ai = std::make_unique<AiSpider>();
        break;

    case SpiderType::WEB_SPINNER:
        // Update actor data for web spinner
        actorData = webSpinnerData;

        // Stats for web spinner - now much more formidable
        strength = d.d6() + d.d6() + d.d6();
        dexterity = d.d6() + d.d6() + d.d6();
        constitution = d.d6() + d.d6() + d.d6();

        // Combat properties - significantly stronger
        destructible = std::make_unique<MonsterDestructible>(d.d8() + 5, 1, "dead web weaver", 60, 17, 5);
        attacker = std::make_unique<Attacker>("D6");

        // AI - use web spinner AI for web creation and movement
        ai = std::make_unique<AiWebSpinner>();
        break;
    }
}

void Spider::update()
{
    // Call the base class update method
    Creature::update();
}

int Spider::get_poison_chance() const
{
    // Return poison chance based on spider type
    switch (spiderType)
    {
    case SpiderType::SMALL:
        return 25;
    case SpiderType::GIANT:
        return 15;
    case SpiderType::WEB_SPINNER:
        return 15;
    default:
        return 0;
    }
}

// Small Spider implementation
SmallSpider::SmallSpider(Vector2D position) : Spider(position, SpiderType::SMALL)
{
    // Any additional small spider initialization
}

// Giant Spider implementation
GiantSpider::GiantSpider(Vector2D position) : Spider(position, SpiderType::GIANT)
{
    // Any additional giant spider initialization
}

// Web Spinner implementation
WebSpinner::WebSpinner(Vector2D position) : Spider(position, SpiderType::WEB_SPINNER)
{
    // Any additional web spinner initialization
}