#pragma once

// file: MonsterSpellcasting.h
//
// A monster's choice to cast instead of moving or striking. Its candidates are its memorized
// spells in the order they were filled, then its innate powers in the order its record lists
// them. It casts the first that is ready and can reach the target, through the same casting
// the player's spells go through, aimed at the target rather than chosen by the player.
//
// Usage, from a monster's perceived arm, before it moves or attacks:
//
//   if (MonsterSpellcasting::try_cast_at(lich, *ctx.player(), ctx) == MonsterCast::CAST)
//   {
//       return;   // the cast was the lich's action
//   }
//   // NOTHING_CAST: no spell is ready, or none reaches; the lich acts as it would have
//
// A silenced monster casts nothing. Its dead are left for the sweep after the creature loop,
// since the cast runs inside that loop.

struct GameContext;
class Creature;

// What a monster's turn did with its spells.
enum class MonsterCast
{
	// It cast, and the cast is its action.
	CAST,
	// It had nothing ready that reaches, so its action is still to take.
	NOTHING_CAST
};

namespace MonsterSpellcasting
{

// Casts the first ready candidate that reaches target, spending its memorized copy or
// recording its innate use, and says whether it did.
//
// Example, a lich of wizard 18 with { magic_missile, fireball } memorized, four tiles from
// the player in sight:
//   try_cast_at(lich, player, ctx);   // -> CAST, five missiles; { fireball } left
//   try_cast_at(lich, player, ctx);   // -> CAST, a 10d6 fireball on the player's tile
//   try_cast_at(lich, player, ctx);   // -> NOTHING_CAST, nothing left
MonsterCast try_cast_at(Creature& caster, Creature& target, GameContext& ctx);

} // namespace MonsterSpellcasting

// end of file: MonsterSpellcasting.h
