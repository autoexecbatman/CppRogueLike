// file: Systems/ObjectManager.h
#ifndef OBJECT_MANAGER_H
#define OBJECT_MANAGER_H

#pragma once

#include <vector>
#include <memory>
#include "../Utils/Vector2D.h"

class Object;
class Web;
class Creature;
class Container;

class ObjectManager 
{
public:
    ObjectManager() = default;
    ~ObjectManager() = default;

    // Object queries
    Web* findWebAt(Vector2D position, const std::vector<std::unique_ptr<Object>>& objects);
    
    // Object creation templates - implemented inline to avoid template instantiation issues
    template <typename T>
    void create_creature(Vector2D position, std::vector<std::unique_ptr<Creature>>& creatures)
    {
        creatures.push_back(std::make_unique<T>(position));
    }

    template <typename T>
    void create_item(Vector2D position, Container& container);

    // Object lifecycle management
    void cleanup_destroyed_objects(std::vector<std::unique_ptr<Object>>& objects);

private:
    // Private helper methods if needed
};

// Template specialization for create_item - must be in header but after class definition
#include "../Actor/Container.h"

template <typename T>
void ObjectManager::create_item(Vector2D position, Container& container)
{
    container.inv.push_back(std::make_unique<T>(position));
}

#endif // OBJECT_MANAGER_H
// end of file: Systems/ObjectManager.h
