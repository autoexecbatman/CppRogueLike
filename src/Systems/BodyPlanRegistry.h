#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "../Actor/EquipmentSlot.h"

// file: BodyPlanRegistry.h
//
// The body templates creatures are built on, loaded from
// data/content/body_plans.json.
//
// A body plan is the set of slots a creature has at all, which is a different
// question from what is currently in them. Templates are shared vocabulary:
// sixteen monsters and the player are all built on "humanoid", so the
// composition is written once and named many times.
//
// Composition follows the Monstrous Manual. A medusa cannot easily wear a
// helmet, so "serpent_haired" is a humanoid body without the head slot.
//
// Usage -- giving a creature a body:
//
//   BodyPlanRegistry registry;
//   registry.load("data/content/body_plans.json"); // throws if the file is missing
//   creature.set_body_plan(registry.get("humanoid"));
//   registry.get("");                              // -> empty, wears nothing
//   registry.get("humaniod");                      // throws, misspelled
class BodyPlanRegistry
{
private:
	std::unordered_map<std::string, std::vector<EquipmentSlot>> plans{};
	const std::vector<EquipmentSlot> nothing{};

public:
	// Reads every template in the file, replacing whatever was loaded before.
	// An unknown slot name throws, so a typo fails here rather than leaving a
	// creature quietly missing a limb.
	void load(std::string_view path);

	// The slots a template grants. An empty name is a creature that wears
	// nothing and yields an empty plan; any other unknown name throws.
	[[nodiscard]] const std::vector<EquipmentSlot>& get(std::string_view name) const;
};
