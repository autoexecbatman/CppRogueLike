#include <algorithm>
#include <ranges>

#include "TargetingSystem.h"
#include "../Core/GameContext.h"
#include "../Systems/CreatureManager.h"
#include "../Systems/MessageSystem.h"
#include "../ActorTypes/Player.h"
#include "../Systems/InputHandler.h"
#include "../Systems/RenderingManager.h"
#include "../Systems/DataManager.h"
#include "../Items/ItemClassification.h"
#include "../Gui/Gui.h"

const Vector2D TargetingSystem::select_target(GameContext& ctx, Vector2D startPos, int maxRange) const
{
	// Check if player is trying to attack but doesn't have a ranged weapon
	if (!ctx.player->has_state(ActorState::IS_RANGED))
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "You need a ranged weapon to attack at a distance!", true);
		return Vector2D{ -1, -1 };
	}

	// Build sorted list of valid targets (visible + in range + LOS)
	std::vector<Creature*> valid_targets;
	for (const auto& creature : *ctx.creatures)
	{
		if (!creature || creature->destructible->is_dead())
			continue;
		if (!ctx.map->is_in_fov(creature->position))
			continue;
		if (!is_valid_target(ctx, startPos, creature->position, maxRange))
			continue;
		valid_targets.push_back(creature.get());
	}

	auto by_distance = [&startPos](const Creature* a, const Creature* b)
	{
		return a->get_tile_distance(startPos) < b->get_tile_distance(startPos);
	};
	std::ranges::sort(valid_targets, by_distance);

	// Start on the nearest target, or player position if no targets in range
	int target_index = 0;
	Vector2D targetCursor = valid_targets.empty()
		? ctx.player->position
		: valid_targets[0]->position;

	// TODO: stub - targeting UI loop requires curses replacement
	// Previously used clear/render/mvchgat/mvaddch/mvprintw/attron/attroff/refresh/getch
	// Key constants: UP=0x103, DOWN=0x102, LEFT=0x104, RIGHT=0x105
	// Game logic for target cycling (Tab/Shift+Tab), fine aim (arrow keys),
	// fire (Enter), cancel (Esc) needs to be reimplemented with new renderer

	// For now, auto-select nearest target if available
	if (!valid_targets.empty())
	{
		targetCursor = valid_targets[0]->position;
		if (is_valid_target(ctx, startPos, targetCursor, maxRange))
		{
			if (ctx.map->is_in_fov(targetCursor))
			{
				const auto& actor = ctx.map->get_actor(targetCursor, ctx);
				if (actor)
				{
					ctx.player->attacker->attack(*ctx.player, *actor, ctx);
					*ctx.game_status = GameStatus::NEW_TURN;
					return targetCursor;
				}
			}
		}
	}

	return Vector2D{ -1, -1 };
}

void TargetingSystem::draw_los(GameContext& ctx, Vector2D targetCursor) const
{
	// TODO: stub - LOS line drawing requires curses replacement
	// Previously used TCODLine + mvchgat/attron/mvaddch/attroff for LOS visualization
	// and Bresenham line with blocked/clear color indicators
}

void TargetingSystem::draw_range_indicator(GameContext& ctx, Vector2D center, int range) const
{
	// TODO: stub - range indicator drawing requires curses replacement
	// Previously used attron/mvaddch/attroff to highlight range boundary
}

// Check if a target position is valid
bool TargetingSystem::is_valid_target(GameContext& ctx, Vector2D from, Vector2D to, int maxRange) const
{
	// Check if the target is within range
	if (from.distance_to(to) > maxRange)
	{
		return false;
	}

	// Check if there's a clear line of sight
	if (!ctx.map->has_los(from, to))
	{
		return false;
	}

	// Check if there's an enemy at the target
	auto entity = ctx.map->get_actor(to, ctx);
	if (!entity)
	{
		return false;
	}

	// Make sure it's not the player
	if (entity == ctx.player)
	{
		return false;
	}

	return true;
}

// Process a ranged attack
bool TargetingSystem::process_ranged_attack(GameContext& ctx, Creature& attacker, Vector2D targetPos) const
{
	auto target = ctx.map->get_actor(targetPos, ctx);
	if (!target || target == &attacker)
	{
		return false;
	}

	// Check range and line of sight
	if (!is_valid_target(ctx, attacker.position, targetPos, 4))
	{
		return false;
	}

	// Get projectile info from the player
	char projectileSymbol = '*';
	int projectileColor = 2;

	// Animate the projectile
	animate_projectile(ctx, attacker.position, targetPos, projectileSymbol, projectileColor);

	// Perform the attack
	attacker.attacker->attack(attacker, *target, ctx);

	return true;
}

// Animate a projectile flying from source to target
void TargetingSystem::animate_projectile(GameContext& ctx, Vector2D from, Vector2D to, char projectileSymbol, int colorPair) const
{
	// TODO: stub - projectile animation requires curses replacement
	// Previously animated along Bresenham path using attron/mvaddch/attroff/refresh/napms
}

// AOE preview from pick_tile method
void TargetingSystem::draw_aoe_preview(Vector2D center, int radius) const
{
	// TODO: stub - AOE preview drawing requires curses replacement
	// Previously used mvchgat with A_REVERSE and WHITE_BLUE_PAIR
}

// Game.cpp compatibility method - preserves exact behavior
bool TargetingSystem::pick_tile(GameContext& ctx, Vector2D* position, int maxRange) const
{
	// TODO: stub - tile picking UI loop requires curses replacement
	// Previously used clear/render/mvchgat/mvaddch/attron/attroff/refresh/getch
	// Key constants: UP=0x103, DOWN=0x102, LEFT=0x104, RIGHT=0x105
	// Game logic for cursor movement, target confirm (f/Enter), cancel (r/Esc)
	// needs to be reimplemented with new renderer
	return false;
}

// Handle ranged attack coordination
void TargetingSystem::handle_ranged_attack(GameContext& ctx) const
{
	// Get equipped missile weapon to determine range
	Item* missileWeapon = ctx.player->get_equipped_item(EquipmentSlot::MISSILE_WEAPON);
	const int weaponRange = get_weapon_range(missileWeapon);

	// Enter targeting mode with weapon-specific range
	Vector2D targetPos = select_target(ctx, ctx.player->position, weaponRange);

	// If a valid target was selected and we're in attack mode
	if (targetPos.x != -1 && targetPos.y != -1)
	{
		// Process the ranged attack (including projectile animation)
		process_ranged_attack(ctx, *ctx.player, targetPos);

		// Clean up dead creatures after ranged combat
		ctx.creature_manager->cleanup_dead_creatures(*ctx.creatures);
	}
}

// Get weapon range based on weapon type (AD&D 2e ranges)
int TargetingSystem::get_weapon_range(const Item* weapon)
{
	if (!weapon)
		return 4; // Default fallback range

	switch (weapon->itemClass)
	{
		case ItemClass::BOW:
			if (weapon->itemId == ItemId::LONG_BOW)
				return 7; // AD&D 2e: 70 yards (~7 dungeon tiles)
			else
				return 5; // AD&D 2e: 50 yards (~5 dungeon tiles) for short bow
		case ItemClass::CROSSBOW:
			return 6; // AD&D 2e: 60 yards (~6 dungeon tiles)
		default:
			return 4; // Default for unknown weapons
	}
}

TargetResult TargetingSystem::acquire_targets(GameContext& ctx, TargetMode mode, Vector2D origin, int range, int aoe_radius) const
{
    switch (mode)
    {
    case TargetMode::AUTO_NEAREST:  return target_auto_nearest(ctx, origin, range);
    case TargetMode::PICK_TILE_SINGLE: return target_pick_single(ctx);
    case TargetMode::PICK_TILE_AOE:    return target_pick_aoe(ctx, aoe_radius);
    }
    return {};
}

TargetResult TargetingSystem::target_auto_nearest(GameContext& ctx, Vector2D origin, int range) const
{
    const auto& monster = ctx.creature_manager->get_closest_monster(*ctx.creatures, origin, range);
    if (!monster)
        return {};
    return {true, monster->position, {monster}};
}

TargetResult TargetingSystem::target_pick_single(GameContext& ctx) const
{
    Vector2D pos{0, 0};
    if (!pick_tile(ctx, &pos, 0))
        return {};
    Creature* actor = ctx.map->get_actor(pos, ctx);
    if (!actor)
        return {};
    return {true, pos, {actor}};
}

TargetResult TargetingSystem::target_pick_aoe(GameContext& ctx, int aoe_radius) const
{
    Vector2D center{0, 0};
    if (!pick_tile(ctx, &center, aoe_radius))
        return {};
    std::vector<Creature*> targets;
    for (const auto& c : *ctx.creatures)
    {
        if (c && !c->destructible->is_dead() && c->get_tile_distance(center) <= aoe_radius)
            targets.push_back(c.get());
    }
    return {true, center, targets};
}
