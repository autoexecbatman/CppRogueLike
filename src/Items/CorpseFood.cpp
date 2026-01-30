#include <algorithm>
#include <unordered_map>
#include <string_view>

#include "CorpseFood.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "../Systems/HungerSystem.h"
#include "../Systems/MessageSystem.h"
#include "../Actor/Actor.h"

// Define nutrition values for different monster types
const std::unordered_map<std::string, int> CORPSE_NUTRITION_VALUES = {
    {"dead goblin", 40},
    {"dead orc", 80},
    {"dead troll", 120},
    {"dead dragon", 200},
    {"dead archer", 70},
    {"dead mage", 60},
    {"dead shopkeeper", 100},
};

// Define flavor text for different monster types
const std::unordered_map<std::string, std::string> CORPSE_FLAVOR_TEXT = {
    {"dead goblin", "It's greasy and gamey."},
    {"dead orc", "It's tough and stringy."},
    {"dead troll", "It's surprisingly filling, if you can stomach it."},
    {"dead dragon", "It tastes exotic and somewhat spicy!"},
    {"dead archer", "It tastes... questionable."},
    {"dead mage", "There's a strange aftertaste of magical residue."},
    {"dead shopkeeper", "Well-marbled, but you feel guilty..."},
};

namespace
{
    // Named lambda for map lookups without iterator boilerplate
    //
    // C++20 Templated Lambda Explanation:
    // - `constexpr auto` = Lambda can be evaluated at compile-time when possible
    // - `[]<typename K, typename V>` = C++20 syntax for generic lambda with explicit template parameters
    //   (Before C++20, you'd need to use a template function instead)
    // - `noexcept` = Guarantees this function won't throw exceptions (optimization hint)
    // - `-> V` = Trailing return type (explicit return type specification)
    //
    // Usage: get_or_default(myMap, "key", "default_value")
    // Returns the value if key exists, otherwise returns the default_value
    constexpr auto get_or_default = []<typename K, typename V>(
        const std::unordered_map<K, V>& map,
        const K& key,
        const V& default_value) noexcept -> V
    {
        // C++17 if-with-initializer: declare 'it' in the if statement scope
        if (const auto it = map.find(key); it != map.end())
        {
            return it->second;  // Key found, return the value
        }
        return default_value;  // Key not found, return default
    };
}

// TODO: Rule of 5.
CorpseFood::CorpseFood(int nutrition_value) : nutrition_value(nutrition_value) {}

bool CorpseFood::use(Item& owner, Creature& wearer, GameContext& ctx)
{
    // Dynamically calculate nutrition based on corpse type if it wasn't set
    if (nutrition_value <= 0)
    {
        nutrition_value = get_or_default(CORPSE_NUTRITION_VALUES, owner.actorData.name, 50);
    }

    // Add a small random variation to make it interesting
    int actualNutrition = nutrition_value + ctx.dice->roll(-10, 10);
    actualNutrition = std::max(10, actualNutrition); // Minimum 10 nutrition

    // Reduce hunger by the nutrition value of the corpse
    ctx.hunger_system->decrease_hunger(ctx, actualNutrition);

    // Generate flavor text based on the corpse type
    const std::string flavorText = get_or_default(
        CORPSE_FLAVOR_TEXT,
        owner.actorData.name,
        std::string{"It tastes... questionable."}
    );

    // Display message
    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "You eat the ");
    ctx.message_system->append_message_part(RED_BLACK_PAIR, owner.actorData.name);
    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, ". " + flavorText);
    ctx.message_system->finalize_message();

    // Corpse is consumed after eating
    return Pickable::use(owner, wearer, ctx);
}

void CorpseFood::load(const json& j)
{
    if (j.contains("nutrition_value") && j["nutrition_value"].is_number())
    {
        nutrition_value = j["nutrition_value"].get<int>();
    }
    else
    {
        nutrition_value = 50; // Default value
    }
}

void CorpseFood::save(json& j)
{
    j["type"] = static_cast<int>(PickableType::CORPSE_FOOD);
    j["nutrition_value"] = nutrition_value;
}
