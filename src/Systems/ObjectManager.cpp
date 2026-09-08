#include <cassert>
#include <memory>
#include <vector>

#include "../Actor/Actor.h"
#include "../Objects/Web.h"
#include "../Utils/Vector2D.h"
#include "ObjectManager.h"

Web* ObjectManager::find_web_at(Vector2D position, const std::vector<std::unique_ptr<TileFeature>>& objects) const
{
	for (const auto& obj : objects)
	{
		assert(obj && "tileFeatures holds a null entry");
		if (!obj->is_destroyed() &&
			obj->position == position &&
			obj->get_kind() == FeatureKind::WEB)
		{
			return dynamic_cast<Web*>(obj.get());
		}
	}
	return nullptr;
}

void ObjectManager::cleanup_destroyed_objects(std::vector<std::unique_ptr<TileFeature>>& objects)
{
	// Remove destroyed objects
	auto isNull = [](const auto& obj)
	{
		return !obj;
	};
	std::erase_if(objects, isNull);
}
