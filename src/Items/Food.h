#pragma once

#include "../Actor/Pickable.h"
#include "../Actor/Actor.h"

class Food : public Pickable {
public:
    Food(int nutrition_value);

    bool use(Item& owner, Creature& wearer) override;
    void load(const json& j) override;
    void save(json& j) override;
    PickableType get_type() const override { return PickableType::FOOD; }

private:
    int nutrition_value;  // How much hunger this food reduces
};

// Different food types with varying nutritional values
class Ration : public Item {
public:
    Ration(Vector2D position);
};

class Fruit : public Item {
public:
    Fruit(Vector2D position);
};

class Bread : public Item {
public:
    Bread(Vector2D position);
};

class Meat : public Item {
public:
    Meat(Vector2D position);
};