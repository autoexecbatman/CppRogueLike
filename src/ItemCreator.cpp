#include "ItemCreator.h"
#include "Game.h"
#include "ActorTypes/Healer.h"
#include "ActorTypes/LightningBolt.h"
#include "ActorTypes/Fireball.h"
#include "Actor/Confuser.h"
#include "Actor/Pickable.h"
#include "Armor.h"
#include "Colors/Colors.h"

std::unique_ptr<Item> ItemCreator::create_health_potion(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'!', "health potion", HPBARMISSING_PAIR});
    item->pickable = std::make_unique<Healer>(10);
    item->value = 50; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_scroll_lightning(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'#', "scroll of lightning", LIGHTNING_PAIR});
    item->pickable = std::make_unique<LightningBolt>(5, 20);
    item->value = 150; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_scroll_fireball(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'#', "scroll of fireball", FIREBALL_PAIR});
    item->pickable = std::make_unique<Fireball>(3, 12);
    item->value = 100; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_scroll_confusion(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'#', "scroll of confusion", CONFUSION_PAIR});
    item->pickable = std::make_unique<Confuser>(10, 8);
    item->value = 120; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_dagger(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "dagger", WHITE_PAIR});
    item->pickable = std::make_unique<Dagger>();
    item->value = 2; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_short_sword(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "short sword", WHITE_PAIR});
    item->pickable = std::make_unique<ShortSword>();
    item->value = 10; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_long_sword(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "long sword", WHITE_PAIR});
    item->pickable = std::make_unique<LongSword>();
    item->value = 15; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_staff(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "staff", WHITE_PAIR});
    item->pickable = std::make_unique<Staff>();
    item->value = 6; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_longbow(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{')', "longbow", LIGHTNING_PAIR});
    item->pickable = std::make_unique<Longbow>();
    item->value = 75; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_leather_armor(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "leather armor", DOOR_PAIR});
    item->pickable = std::make_unique<LeatherArmor>();
    item->value = 5; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_chain_mail(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "chain mail", DOOR_PAIR});
    item->pickable = std::make_unique<ChainMail>();
    item->value = 75; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_plate_mail(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "plate mail", DOOR_PAIR});
    item->pickable = std::make_unique<PlateMail>();
    item->value = 400; // AD&D 2e price
    return item;
}

void ItemCreator::ensure_correct_value(Item& item)
{
    // Fix items with incorrect values based on their name and pickable type
    if (item.actorData.name == "health potion" && item.value != 50)
    {
        item.value = 50;
    }
    else if (item.actorData.name == "scroll of lightning" && item.value != 150)
    {
        item.value = 150;
    }
    else if (item.actorData.name == "scroll of fireball" && item.value != 100)
    {
        item.value = 100;
    }
    else if (item.actorData.name == "scroll of confusion" && item.value != 120)
    {
        item.value = 120;
    }
    else if (item.actorData.name == "dagger" && item.value != 2)
    {
        item.value = 2;
    }
    else if (item.actorData.name == "short sword" && item.value != 10)
    {
        item.value = 10;
    }
    else if (item.actorData.name == "long sword" && item.value != 15)
    {
        item.value = 15;
    }
    else if (item.actorData.name == "staff" && item.value != 6)
    {
        item.value = 6;
    }
    else if (item.actorData.name == "longbow" && item.value != 75)
    {
        item.value = 75;
    }
    else if (item.actorData.name == "leather armor" && item.value != 5)
    {
        item.value = 5;
    }
    else if (item.actorData.name == "chain mail" && item.value != 75)
    {
        item.value = 75;
    }
    else if (item.actorData.name == "plate mail" && item.value != 400)
    {
        item.value = 400;
    }
}
