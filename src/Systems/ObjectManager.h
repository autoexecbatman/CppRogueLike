// file: Systems/ObjectManager.h
#ifndef OBJECT_MANAGER_H
#define OBJECT_MANAGER_H

#pragma once

#include <vector>
#include <memory>
#include "../Utils/Vector2D.h"
#include "../Actor/Container.h"

class Object;
class Web;
class Creature;

class ObjectManager 
{
public:
    ObjectManager() = default;
    ~ObjectManager() = default;

    // Object queries
    Web* find_web_at(Vector2D position, const std::vector<std::unique_ptr<Object>>& objects) const;
    
    // Object creation templates - implemented inline to avoid template instantiation issues
    template <typename T>
    void create_creature(Vector2D position, std::vector<std::unique_ptr<Creature>>& creatures)
    {
        creatures.push_back(std::make_unique<T>(position));
    }

    template <typename T>
    void create_item(Vector2D position, Container& container)
    {
        container.inv.push_back(std::make_unique<T>(position));
    }

    // Object lifecycle management
    void cleanup_destroyed_objects(std::vector<std::unique_ptr<Object>>& objects);
};

#endif // OBJECT_MANAGER_H
// end of file: Systems/ObjectManager.h
