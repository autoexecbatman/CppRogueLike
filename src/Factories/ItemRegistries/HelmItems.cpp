// file: HelmItems.cpp
#include <span>

#include "../../Actor/Pickable.h"
#include "../../Colors/Colors.h"
#include "../../Items/ItemClassification.h"
#include "../../Items/MagicalItemEffects.h"
#include "../ItemCreator.h"
#include "HelmItems.h"

namespace
{
const ItemRegistryEntry entries[] = {
	{ ItemId::HELM_OF_BRILLIANCE, { .name = "helm of brilliance", .color = YELLOW_BLACK_PAIR, .itemClass = ItemClass::HELMET, .value = 12000, .pickable_type = PickableType::MAGICAL_HELM, .effect = MagicalEffect::BRILLIANCE, .base_weight = 1, .level_minimum = 6, .level_scaling = 0.15f, .category = "magical_helm" } },

	{ ItemId::HELM_OF_TELEPORTATION, { .name = "helm of teleportation", .color = MAGENTA_BLACK_PAIR, .itemClass = ItemClass::HELMET, .value = 7500, .pickable_type = PickableType::MAGICAL_HELM, .effect = MagicalEffect::TELEPORTATION } },

	{ ItemId::HELM_OF_TELEPATHY, { .name = "helm of telepathy", .color = BLUE_BLACK_PAIR, .itemClass = ItemClass::HELMET, .value = 8000, .pickable_type = PickableType::MAGICAL_HELM, .effect = MagicalEffect::TELEPATHY } },

	{ ItemId::HELM_OF_UNDERWATER_ACTION, { .name = "helm of underwater action", .color = CYAN_BLACK_PAIR, .itemClass = ItemClass::HELMET, .value = 6000, .pickable_type = PickableType::MAGICAL_HELM, .effect = MagicalEffect::UNDERWATER_ACTION } },
};
}

std::span<const ItemRegistryEntry> get_helm_items()
{
	return entries;
}
