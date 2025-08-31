// file: Systems/ObjectManager.cpp
#include "ObjectManager.h"
#include "../Objects/Web.h"
#include "../Actor/Container.h"
#include <algorithm>

Web* ObjectManager::findWebAt(Vector2D position, const std::vector<std::unique_ptr<Object>>& objects)
{
    for (const auto& obj : objects)
    {
        if (obj && obj->position == position &&
            obj->actorData.name == "spider web")
        {
            return dynamic_cast<Web*>(obj.get());
        }
    }
    return nullptr;
}

void ObjectManager::cleanup_destroyed_objects(std::vector<std::unique_ptr<Object>>& objects)
{
    // Remove destroyed objects
    auto isNull = [](const auto& obj) { return !obj; };
    std::erase_if(objects, isNull);
}

// end of file: Systems/ObjectManager.cpp
