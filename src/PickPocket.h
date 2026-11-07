#pragma once
// file: PickPocket.h
// The thief's Pick Pockets skill, by the Player's Handbook (PDF page 86):
// filching an item from a creature beside the thief.
//
// One d100 roll answers two questions. At or under the thief's Pick Pockets
// score, an item is taken; at or above 100 minus three times the victim's
// level, the victim notices, and a creature that notices turns hostile. The
// two are independent, so a try can take an item and be caught at once.
// Tries are unlimited; being caught is the cost.
//
// What a creature carries is its shop stock if it keeps a shop, and its pack
// otherwise. The item taken is chosen at random.
//
// Usage, the key's command, which picks the victim and spends the round:
//
//   pick_pocket(ctx);
//   // one creature beside the thief: the try is made on it
//   // several: a menu asks which
//   // none carrying anything: "There is nothing within reach to steal."
//
// The pieces it is built from, for a caller that already has a victim:
//
//   pick_pocket_notice_roll(5);   // -> 85, the lowest roll a level 5 notices
//   pick_pocket_victims(thief, creatures);   // who is beside and carrying
//   attempt_pick_pocket(thief, shopkeeper, ctx);
//   // -> PickPocketAttempt::TOOK_ITEM on a roll at or under the score

#include <memory>
#include <vector>

namespace CppRL
{
	class Creature;
	class Player;
	struct GameContext;

	// What one try did. The refusals spend no time; both tries spend a round.
	// Being noticed is not an outcome here: it shows as the victim turning
	// hostile, whatever the try took.
	enum class PickPocketAttempt
	{
		NOT_A_THIEF, // no Pick Pockets score to roll against
		PACK_FULL, // nowhere to put what would be taken
		TOOK_NOTHING, // the roll was made and the fingers came away empty
		TOOK_ITEM, // an item moved from the victim to the thief
	};

	// The lowest d100 roll the victim notices: 100 less three times its
	// level, so a 13th-level victim notices on 61 or more.
	//
	// Example:
	//   pick_pocket_notice_roll(1);   // -> 97
	//   pick_pocket_notice_roll(13);  // -> 61
	[[nodiscard]] int pick_pocket_notice_roll(int victimLevel) noexcept;

	// The living creatures beside the thief, diagonals included, that carry
	// something to take. The thief is never his own victim.
	//
	// Example, a thief beside a shopkeeper and an empty-handed rat:
	//   pick_pocket_victims(thief, creatures);   // -> { &shopkeeper }
	[[nodiscard]] std::vector<Creature*> pick_pocket_victims(
		const Creature& thief,
		const std::vector<std::unique_ptr<Creature>>& creatures);

	// One try at the victim's goods, rolled against the thief's Pick Pockets
	// score. A success takes one random carried item, unless it is too heavy
	// for the thief to carry, when it stays and the try took nothing. A roll
	// the victim notices turns it hostile. The caller spends the round on
	// either try; a refusal costs nothing. The victim must carry something.
	//
	// Example, a thief of score 50 beside a level 1 shopkeeper:
	//   attempt_pick_pocket(thief, shopkeeper, ctx);   // d100 30
	//   // -> PickPocketAttempt::TOOK_ITEM, unnoticed
	//   attempt_pick_pocket(thief, shopkeeper, ctx);   // d100 98
	//   // -> PickPocketAttempt::TOOK_NOTHING, and the shopkeeper is hostile
	[[nodiscard]] PickPocketAttempt attempt_pick_pocket(
		Player& thief,
		Creature& victim,
		GameContext& ctx);

	// The key's command: refuses a player with no score, then tries the one
	// creature beside the thief that carries something, or asks which when
	// there are several, and spends the round on a try.
	//
	// Example:
	//   pick_pocket(ctx);   // a fighter -> "You cannot pick pockets."
	void pick_pocket(GameContext& ctx);

} // namespace CppRL
