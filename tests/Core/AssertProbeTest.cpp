// file: AssertProbeTest.cpp
//
// Drives invariants past their bound and checks that the assertion guarding each
// one aborts the process.
//
// An assertion nobody has watched fire has not been shown to exist. The rest of
// the suite is blind to these, because an abort is not an exception: no
// EXPECT_THROW reaches one, and no ordinary test survives one. Each probe
// therefore runs in a child process through GoogleTest's death-test mechanism,
// and the text it expects is the assertion's own message - which is what
// separates "the process died" from "this assertion is what stopped it".
//
// What this deliberately does not do: it never checks that the guarded code
// works. Every call below is a violation. Correct behaviour for the same
// functions is covered by BodyPlanTest, EquipmentStorageTest and
// MonsterEquipmentTest.
//
// Where the expectations come from: each string is copied out of the assertion
// it targets, and carries enough of that assertion's own wording to identify it
// - two of them would otherwise match each other's message. A probe that
// stops matching is a probe whose assertion was reworded, moved or deleted, and
// that is the finding.
//
// Run it:
//
//   cmake --build build --config Debug --target test_exe
//   cd build/bin/Debug && ./test_exe.exe --gtest_filter=AssertProbeDeathTest.*
//
//   [==========] Running 16 tests from 1 test suite.
//   [ RUN      ] AssertProbeDeathTest.WearingNothingAborts
//   [       OK ] AssertProbeDeathTest.WearingNothingAborts (69 ms)
//   ...
//   [  PASSED  ] 16 tests.
//
// A green run here says every probe died with the right message. It does not
// say the probes have teeth - for that, tests/assert_probes.ps1 deletes each
// targeted assertion in turn, rebuilds, and confirms the matching probe fails.

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "src/Ai.h"
#include "src/ArmorClass.h"
#include "src/AttackKind.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/Decoration.h"
#include "src/DungeonRoom.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/InputSystem.h"
#include "src/InventoryData.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/ListCursor.h"
#include "src/Map.h"
#include "src/MenuSell.h"
#include "src/MonsterAttacker.h"
#include "src/MonsterCreator.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/PlayerController.h"
#include "src/RandomDice.h"
#include "src/Renderer.h"
#include "src/ShopKeeper.h"
#include "src/SpellSystem.h"
#include "src/TargetingSystem.h"
#include "src/TurnUndead.h"

using namespace Turning;
#include "src/WeaponDamageRegistry.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/MockGameContext.h"

// GoogleTest runs any suite whose name ends in DeathTest before the others, so
// the abort happens before anything else has had a chance to start a thread.
// set_door is protected, so a probe that needs a door placed reaches it through this.
class MapWithProbeDoors : public Map
{
public:
	using Map::Map;
	using Map::set_door;
};

class AssertProbeDeathTest : public ::testing::Test
{
protected:
	MockGameContext mock{};
	GameContext ctx{};

	void SetUp() override
	{
#ifdef NDEBUG
		// Said out loud rather than passed silently: with the assertions
		// compiled out there is nothing here to fire, and a green run would
		// claim otherwise.
		GTEST_SKIP() << "assertions are compiled out under NDEBUG; these probes can prove nothing in this build";
#endif
		// The mock loads content, so a probe dies on the invariant it names rather
		// than on a missing file, which aborts with the wrong message.
		ctx = mock.to_game_context();
	}

	static std::unique_ptr<Creature> make_creature()
	{
		return std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "orc", ColorPairId::WHITE_BLACK });
	}

	static std::unique_ptr<Item> make_item(std::string_view name)
	{
		return std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, std::string(name), ColorPairId::WHITE_BLACK });
	}

	// The goblin carries a short sword, so this one call reaches both of
	// create_from_params' registry assertions. The creature it would return is
	// discarded because neither probe lets the call get that far.
	void build_a_goblin()
	{
		[[maybe_unused]] const std::unique_ptr<Creature> built =
			MonsterCreator::create_from_params(Vector2D{ 0, 0 }, mock.monsterRegistry.get_params("goblin"), ctx);
	}
};

// wear() takes ownership, so a caller passing nothing has already lost whatever
// it meant to hand over. There is no state to record for equipping nothing.
TEST_F(AssertProbeDeathTest, WearingNothingAborts)
{
	std::unique_ptr<Creature> creature = make_creature();
	creature->set_body_plan({ EquipmentSlot::RIGHT_HAND });

	EXPECT_DEATH(creature->wear(nullptr, EquipmentSlot::RIGHT_HAND), "wear called with no item");
}

// Every caller of the weapon damage table guards on is_weapon() first, and a test
// walks the data and fails on any weapon missing from the table, so a key that is
// not in it cannot arrive from a correctly wired game. The unarmed damage it falls
// back to is what a Release build should still draw rather than crash on; the
// assertion is what stops a Debug build from drawing a longsword as a fist and
// saying nothing.
TEST_F(AssertProbeDeathTest, AskingTheWeaponTableForSomethingThatIsNotAWeaponAborts)
{
	EXPECT_DEATH(WeaponDamageRegistry::get_damage_info("health_potion"), "no damage of its own");
}

// A body plan is the set of slots a creature has at all. Wearing into one it
// does not have puts an item where no lookup will ever find it again.
TEST_F(AssertProbeDeathTest, WearingIntoASlotTheBodyLacksAborts)
{
	std::unique_ptr<Creature> creature = make_creature();
	creature->set_body_plan({ EquipmentSlot::RIGHT_HAND });

	EXPECT_DEATH(creature->wear(make_item("chain mail"), EquipmentSlot::BODY), "a slot this body does not have");
}

// Hit dice are rolled from dice with a floor above zero, so a roll at or below
// it is a caller that has stopped rolling dice. HealthPool itself refuses
// nothing, and a pool of zero is a creature born dead.
TEST_F(AssertProbeDeathTest, HitDiceAtZeroAbort)
{
	std::unique_ptr<Creature> creature = make_creature();

	EXPECT_DEATH(creature->set_hit_dice(0), "roll at or below zero");
}

// Every path that builds a creature gives it a Strength above zero - a spider
// rolls 3d6, MonsterCreator floors its roll at 1, and a player's comes from the
// blueprint - and nothing in the game lowers a monster's score. So a swing from a
// creature with none is a creature that was never finished being built, and the
// attack path is where that first becomes visible.
TEST_F(AssertProbeDeathTest, AttackingWithNoStrengthAborts)
{
	std::unique_ptr<Creature> attacker = make_creature();
	attacker->healthPool = std::make_unique<HealthPool>(10);
	attacker->attacker = std::make_unique<MonsterAttacker>(*attacker, DamageInfo{ "1d4", DamageType::PHYSICAL });

	std::unique_ptr<Creature> target = make_creature();
	target->healthPool = std::make_unique<HealthPool>(10);

	EXPECT_DEATH(attacker->attacker->attack(*target, AttackKind::MELEE, ctx), "attacked with no Strength");
}

// The pack owns every entry it holds, and dying dereferences each one to drop it.
// A null there is a container that stopped owning its contents, which the loop
// would find by walking into it.
TEST_F(AssertProbeDeathTest, DroppingAPackHoldingNothingAborts)
{
	// die() checks for a player first, so without one this would abort on that
	// instead and prove nothing about the pack.
	std::unique_ptr<Player> killer = std::make_unique<Player>(Vector2D{ 1, 1 });
	killer->set_creature_class(CreatureClass::FIGHTER);
	killer->healthPool = std::make_unique<HealthPool>(20);
	killer->experienceReward = std::make_unique<ExperienceReward>(0);
	ctx.playerOwner = &killer;

	std::unique_ptr<Creature> creature = make_creature();
	creature->healthPool = std::make_unique<HealthPool>(8);
	creature->experienceReward = std::make_unique<ExperienceReward>(5);
	creature->inventoryData.items.push_back(nullptr);

	EXPECT_DEATH(creature->die(ctx), "a pack holds nothing where an item should be");
}

// Fire and cold are resisted per die before they land, so a plain total of
// either is a producer that skipped the reduction. It has to fail here, where
// the producer is, rather than quietly deal its whole damage to a ring wearer.
TEST_F(AssertProbeDeathTest, TakingFireAsAPlainTotalAborts)
{
	std::unique_ptr<Creature> creature = make_creature();
	creature->set_hit_dice(10);

	EXPECT_DEATH(creature->take_damage_and_check_death(5, ctx, DamageType::FIRE), "arrive as ResistedDamage");
}

// Floating text is always wired in a running game, so a context without it is a fault and
// the first damage number fails where it would have been drawn.
TEST_F(AssertProbeDeathTest, TakingDamageWithNoFloatingTextAborts)
{
	std::unique_ptr<Creature> creature = make_creature();
	creature->set_hit_dice(10);
	ctx.floatingText = nullptr;

	EXPECT_DEATH(creature->take_damage_and_check_death(2, ctx, DamageType::PHYSICAL), "with no floating text");
}

// Every monster's body comes from the registry. Without one it would be built
// with no slots at all and quietly wear nothing it was authored to carry.
TEST_F(AssertProbeDeathTest, BuildingAMonsterWithNoBodyPlanRegistryAborts)
{
	ctx.bodyPlanRegistry = nullptr;

	EXPECT_DEATH(build_a_goblin(), "create_from_params called without a bodyPlanRegistry");
}

// What a monster carries is a real item, made through the same registry the
// player's gear comes from. A goblin's short sword has to exist to be worn.
TEST_F(AssertProbeDeathTest, BuildingAMonsterWithNoContentRegistryAborts)
{
	ctx.contentRegistry = nullptr;

	EXPECT_DEATH(build_a_goblin(), "create_from_params called without a contentRegistry");
}

// A shopkeeper's shop is chosen for the level it stands on, so one built with no level
// manager has no level to stock for.
TEST_F(AssertProbeDeathTest, AShopkeeperBuiltWithNoLevelManagerAborts)
{
	std::unique_ptr<Creature> shopkeeper = make_creature();
	const std::unique_ptr<Ai> ai = Ai::make(AiType::SHOPKEEPER);
	ctx.levelManager = nullptr;

	EXPECT_DEATH(ai->on_created(*shopkeeper, ctx), "AiShopkeeper::on_created: called without a levelManager");
}

// Death pays experience to the player, so a context holding no player has
// nobody to pay. This path runs on every kill in the game.
TEST_F(AssertProbeDeathTest, KillingACreatureWithNoPlayerInContextAborts)
{
	std::unique_ptr<Creature> creature = make_creature();
	creature->experienceReward = std::make_unique<ExperienceReward>(10);

	EXPECT_DEATH(creature->die(ctx), "requires a live player in context");
}

// Every spell effect that lands calls onSuccess, at four sites and sometimes a turn
// later from a targeting callback. An empty one would throw bad_function_call from
// inside that callback; the entry point refuses it instead, naming the caller's
// mistake. The assert is the first statement, so no spell data is needed to reach it.
TEST_F(AssertProbeDeathTest, CastingWithoutACallbackAborts)
{
	std::unique_ptr<Creature> caster = make_creature();

	EXPECT_DEATH(SpellSystem::cast_spell_by_key("sleep", *caster, SpellSource::MEMORIZED, {}, ctx), "cast_spell_by_key requires a callback");
}

// Regeneration counts rounds that have run, from 1. Round 0 is divisible by every
// interval, so a caller passing it would heal every character at once.
TEST_F(AssertProbeDeathTest, RegeneratingBeforeARoundHasRunAborts)
{
	std::unique_ptr<Creature> creature = make_creature();

	EXPECT_DEATH(creature->regenerate_from_constitution(0, mock.data_manager), "called before a round has run");
}

// A ring of regeneration counts rounds the same way, and round 0 would heal every wearer.
TEST_F(AssertProbeDeathTest, RegeneratingFromARingBeforeARoundHasRunAborts)
{
	std::unique_ptr<Creature> creature = make_creature();

	EXPECT_DEATH(creature->regenerate_from_ring(0), "regenerate_from_ring called before a round has run");
}

// Fire and acid damage is part of the damage taken, so it can never exceed it. A pool
// at full health holding some would let regeneration heal a negative amount.
TEST_F(AssertProbeDeathTest, RegeneratingWithMoreFireAndAcidThanDamageAborts)
{
	HealthPool pool{ 20 };
	pool.set_unregenerable_damage(5);

	EXPECT_DEATH([[maybe_unused]] const int healed = pool.regenerate(1), "more fire and acid damage than damage");
}

// Turning walks the whole creature list looking for undead, and the list owns
// every entry it holds. A null there is a container that stopped owning its
// contents, which the walk would find by dereferencing it.
TEST_F(AssertProbeDeathTest, TurningWithANullInTheCreatureListAborts)
{
	// Only a cleric reaches the walk; anything else is refused before it.
	std::unique_ptr<Player> priest = std::make_unique<Player>(Vector2D{ 1, 1 });
	priest->set_creature_class(CreatureClass::CLERIC);
	ctx.playerOwner = &priest;

	std::vector<std::unique_ptr<Creature>> creatures{};
	creatures.push_back(nullptr);
	ctx.creatures = &creatures;

	EXPECT_DEATH([[maybe_unused]] const TurnUndeadReport report = turn_undead(*priest, ctx),
		"turn_undead: creatures list holds a null entry");
}

// Sleep gathers its targets from the whole creature list, which owns every entry
// it holds. A null there would be dereferenced by the first eligibility check.
TEST_F(AssertProbeDeathTest, SleepingWithANullInTheCreatureListAborts)
{
	// An orc casts on no Wisdom table, so the spell reaches its effect without a fizzle roll.
	std::unique_ptr<Creature> caster = make_creature();
	const auto ignoreCompletion = [](GameContext& completedCtx) {
	};

	std::vector<std::unique_ptr<Creature>> creatures{};
	creatures.push_back(nullptr);
	ctx.creatures = &creatures;

	EXPECT_DEATH(SpellSystem::cast_spell_by_key("sleep", *caster, SpellSource::MEMORIZED, ignoreCompletion, ctx),
		"cast_sleep: creatures list holds a null entry");
}

// Creature::load builds the pool, the armour class and the reward only when the
// record carries them. That is right for a monster and wrong for the player, so
// Player::load closes the window. The probe reopens it by deleting one field from
// a record that was otherwise written by save().
TEST_F(AssertProbeDeathTest, LoadingAPlayerRecordMissingItsHealthPoolAborts)
{
	std::unique_ptr<Player> saved = std::make_unique<Player>(Vector2D{ 1, 1 });
	saved->healthPool = std::make_unique<HealthPool>(20);
	saved->armorClass = std::make_unique<ArmorClass>(10);
	saved->experienceReward = std::make_unique<ExperienceReward>(0);

	json record;
	saved->save(record);
	record.erase("healthPool");

	Player loaded{ Vector2D{ 1, 1 } };

	EXPECT_DEATH(loaded.load(record), "Player::load finished with a player the game cannot run");
}

// Fireball walks every creature on the level to find what the burst touches. The
// list owns its entries, so a null is a fault; the old condition folded that into
// the is-it-dead test and skipped past it.
TEST_F(AssertProbeDeathTest, BurstingAFireballWithANullInTheCreatureListAborts)
{
	std::vector<std::unique_ptr<Creature>> creatures{};
	creatures.push_back(nullptr);
	ctx.creatures = &creatures;

	EXPECT_DEATH([[maybe_unused]] const auto burst = SpellSystem::burst_fireball(Vector2D{ 5, 5 }, 3, 2, ctx),
		"burst_fireball: creatures list holds a null entry");
}

// The burst reaches the player too, so a context with no player is a fault in its wiring.
TEST_F(AssertProbeDeathTest, BurstingAFireballWithNoPlayerAborts)
{
	std::vector<std::unique_ptr<Creature>> creatures{};
	ctx.creatures = &creatures;

	EXPECT_DEATH([[maybe_unused]] const auto burst = SpellSystem::burst_fireball(Vector2D{ 5, 5 }, 3, 2, ctx),
		"burst_fireball: called without a player");
}

// Magic missile gathers its targets from the whole list before it fires any.
TEST_F(AssertProbeDeathTest, CastingMagicMissileWithANullInTheCreatureListAborts)
{
	std::unique_ptr<Creature> caster = make_creature();

	std::vector<std::unique_ptr<Creature>> creatures{};
	creatures.push_back(nullptr);
	ctx.creatures = &creatures;

	const auto ignoreCompletion = [](GameContext& completedCtx) {
	};

	EXPECT_DEATH(SpellSystem::cast_spell_by_key("magic_missile", *caster, SpellSource::MEMORIZED, ignoreCompletion, ctx),
		"cast_magic_missile: creatures list holds a null entry");
}

// Hold person walks the list up to its 1d4 cap, so the null is reached before the cap.
TEST_F(AssertProbeDeathTest, CastingHoldPersonWithANullInTheCreatureListAborts)
{
	std::unique_ptr<Creature> caster = make_creature();

	std::vector<std::unique_ptr<Creature>> creatures{};
	creatures.push_back(nullptr);
	ctx.creatures = &creatures;

	const auto ignoreCompletion = [](GameContext& completedCtx) {
	};

	EXPECT_DEATH(SpellSystem::cast_spell_by_key("hold_person", *caster, SpellSource::MEMORIZED, ignoreCompletion, ctx),
		"cast_hold_person: creatures list holds a null entry");
}

// An inventory owns what it holds, so a null in it is a fault. Skipping one on the
// way out would write a record shorter than the inventory, and the next load would
// hand back fewer items without saying so - the loader already refuses to lose items
// that way, and the saver now matches it.
TEST_F(AssertProbeDeathTest, SavingAnInventoryHoldingANullAborts)
{
	CreatureInventory pack{ 10 };
	pack.items.push_back(nullptr);

	nlohmann::json record;

	EXPECT_DEATH(InventoryOperations::save_inventory(pack, record),
		"save_inventory: inventory holds a null where an item should be");
}

// The identify scroll walks the whole pack. The pack owns what it holds, so a null
// there is a fault - and this one had no check of any kind, so it dereferenced
// straight through.
TEST_F(AssertProbeDeathTest, IdentifyingAPackHoldingANullAborts)
{
	std::unique_ptr<Creature> wearer = make_creature();
	wearer->inventoryData.items.push_back(nullptr);

	std::unique_ptr<Item> scrollItem = make_item("identify scroll");
	IdentifyScroll behavior{};

	EXPECT_DEATH([[maybe_unused]] const bool spent = use(behavior, *scrollItem, *wearer, ctx),
		"identify scroll: the pack holds a null where an item should be");
}

// The decoration list owns its entries, so a null in it is a fault. The old condition
// folded that into the is-it-broken test, so a null read as a broken barrel and the
// search walked past it.
TEST_F(AssertProbeDeathTest, FindingADecorationWithANullInTheListAborts)
{
	Map map{ 20, 20 };

	std::vector<std::unique_ptr<Decoration>> decorations{};
	decorations.push_back(nullptr);
	ctx.decorations = &decorations;

	EXPECT_DEATH([[maybe_unused]] const Decoration* found = map.find_decoration_at(Vector2D{ 1, 1 }, ctx),
		"find_decoration_at: the decoration list holds a null entry");
}

// The decoration list is Game's own member, wired into every context the game builds,
// so a lookup with no list is a broken context rather than a level without decorations.
TEST_F(AssertProbeDeathTest, FindingADecorationWithNoListAborts)
{
	Map map{ 20, 20 };
	ctx.decorations = nullptr;

	EXPECT_DEATH([[maybe_unused]] const Decoration* found = map.find_decoration_at(Vector2D{ 1, 1 }, ctx),
		"find_decoration_at: called without decorations");
}

// An equipment entry owns the item in it. The entry exists because something put an
// item in a slot, so an entry with nothing in it is a slot that lost what it was
// holding - and ten readers across five files reach through this pointer, which is
// why the invariant is established here rather than re-checked at each of them.
TEST_F(AssertProbeDeathTest, AnEquipmentEntryBuiltWithNoItemAborts)
{
	// Parenthesised whole: the brace list holds a comma, and EXPECT_DEATH is a macro.
	EXPECT_DEATH((EquippedItem{ nullptr, EquipmentSlot::RIGHT_HAND }),
		"an equipment slot is built around the item in it");
}

// Resting scans the creature list for a nearby enemy. The list owns its entries, so
// a null is a fault - and the old condition folded it into the is-it-dead test, so a
// null read as a corpse and the scan walked past it. A player would then rest beside
// whatever that entry should have been.
TEST_F(AssertProbeDeathTest, RestingWithANullInTheCreatureListAborts)
{
	auto player = std::make_unique<Player>(Vector2D{ 0, 0 });
	player->healthPool = std::make_unique<HealthPool>(20);
	// Resting refuses outright at full health, so the scan is only reached hurt.
	player->healthPool->set_hp(5);

	std::vector<std::unique_ptr<Creature>> creatures{};
	creatures.push_back(nullptr);
	ctx.creatures = &creatures;

	EXPECT_DEATH([[maybe_unused]] const bool rested = player->rest(ctx),
		"Player::rest: creatures list holds a null entry");
}

// Targeting walks the creature list to find the nearest living thing in range. The
// list owns its entries, so a null is a fault. This is the one of the nine messages
// renamed for issues/0009 that can be probed at all: target_auto_nearest is private,
// and acquire_nearest is a public dispatcher that does nothing but call it. The other
// eight sit behind a renderer or a private controller path the mock cannot build.
TEST_F(AssertProbeDeathTest, AcquiringATargetWithANullInTheCreatureListAborts)
{
	std::unique_ptr<Creature> attacker = make_creature();

	std::vector<std::unique_ptr<Creature>> creatures{};
	creatures.push_back(nullptr);
	ctx.creatures = &creatures;

	const TargetingSystem targeting{};

	EXPECT_DEATH([[maybe_unused]] const TargetResult result = targeting.acquire_nearest(ctx, *attacker, 10),
		"target_auto_nearest: creatures list holds a null entry");
}

// A scripted die returned whatever was queued, whatever range was asked for, so a
// test could script a 5 on a d4 and pass on a character no die could roll. The queue
// cannot be checked where it is filled, because the range a value will be drawn
// against is not known until something rolls, so the assertion is at consumption.
// RandomDice::HIGHEST and LOWEST are how a test names an end of the range instead.
TEST_F(AssertProbeDeathTest, ScriptingARollThatTheDieCannotMakeAborts)
{
	RandomDice dice{};
	dice.set_next_roll(5);

	EXPECT_DEATH(dice.roll(1, 4), "outside the range");
}

// The other end of the same guard. Scripting a zero against a d20 was how a test said
// "fail every check" before RandomDice::HIGHEST existed to say it by name, and a guard
// that only checked the top of the range would still let it through.
TEST_F(AssertProbeDeathTest, ScriptingARollBelowWhatTheDieCanMakeAborts)
{
	RandomDice dice{};
	dice.set_next_roll(0);

	EXPECT_DEATH(dice.roll(1, 20), "outside the range");
}

// A mimic wears the look of an item, read from the item registry, so one built with no
// registry has nothing to look like.
TEST_F(AssertProbeDeathTest, AMimicBuiltWithNoItemRegistryAborts)
{
	std::unique_ptr<Creature> mimic = make_creature();
	const std::unique_ptr<Ai> ai = Ai::make(AiType::MIMIC);
	ctx.itemRegistry = nullptr;

	EXPECT_DEATH(ai->on_created(*mimic, ctx), "AiMimic::on_created: called without an itemRegistry");
}

// The floor list owns every item lying in the dungeon, so a null in it is a fault. A step
// reads what lies on the tile it lands on, which is reached only through
// PlayerController::update, so the probe walks the player there with a scripted key.
TEST_F(AssertProbeDeathTest, SteppingOntoAFloorHoldingANullAborts)
{
	mock.map.init_tiles();
	mock.map.set_tile(Vector2D{ 4, 5 }, TileType::FLOOR);
	mock.map.set_tile(Vector2D{ 5, 5 }, TileType::FLOOR);
	mock.player = std::make_unique<Player>(Vector2D{ 4, 5 });
	ctx.floorInventory->items.push_back(nullptr);

	ctx.inputSystem->queue_key(GameKey::RIGHT, 0);
	ctx.inputSystem->poll();
	ctx.inputHandler->key_listen(*ctx.inputSystem);

	EXPECT_DEATH(ctx.player_concrete().controller->update(ctx), "look_on_floor: the floor holds a null");
}

// A door is locked only on a room's wall ring, by the treasure room's pass, so a locked
// door inside a room's floor is a locker that broke that rule.
TEST_F(AssertProbeDeathTest, OpeningARoomWithALockedDoorOnItsFloorAborts)
{
	MapWithProbeDoors map{ 20, 20 };
	map.init_tiles();
	std::vector<DungeonRoom> rooms{ DungeonRoom{ .col = 2, .row = 2, .width = 5, .height = 4 } };
	// Inside the room's floor, which runs over columns 2-6 and rows 2-5.
	map.set_door(Vector2D{ 4, 3 });
	ASSERT_TRUE(map.lock_door(Vector2D{ 4, 3 }));
	ctx.rooms = &rooms;

	// From a door on the room's top wall, so the room is found and its ring scanned.
	EXPECT_DEATH(map.open_all_room_doors(Vector2D{ 4, 1 }, ctx), "a locked door inside a room's floor");
}

// Every way into the sell menu comes from a shopkeeper, so a creature with no shop is a
// caller that broke that. The assertion sits after the menu's renderer check, so the
// probe opens a hidden renderer first; where none can open, it skips with the reason.
TEST_F(AssertProbeDeathTest, OpeningTheSellMenuOnACreatureWithNoShopAborts)
{
	if (exit_code_in_child(&open_and_close_hidden_renderer) == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}

	const auto open_the_menu_on_a_creature_with_no_shop = [this]()
	{
		HiddenRenderer hidden{};
		ctx.renderer = &hidden.renderer;
		Player seller{ Vector2D{ 1, 1 } };
		auto shopless = make_creature();
		MenuSell menu{ *shopless, seller, ctx };
	};
	EXPECT_DEATH(open_the_menu_on_a_creature_with_no_shop(), "MenuSell opened on a creature with no shop");
}

// Game::tick opens the one frame of a tick, so a second open is a menu or a view that
// still frames itself. The first open needs a window, so the probe opens a hidden one.
TEST_F(AssertProbeDeathTest, OpeningAFrameInsideAnOpenFrameAborts)
{
	if (exit_code_in_child(&open_and_close_hidden_renderer) == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}

	const auto open_the_frame_twice = []()
	{
		HiddenRenderer hidden{};
		hidden.renderer.begin_frame();
		hidden.renderer.begin_frame();
	};
	EXPECT_DEATH(open_the_frame_twice(), "begin_frame: a frame is already open");
}

// A close with no frame open is a second close in one tick, or a close by something
// other than Game::tick. The assertion comes before any raylib call, so no window is needed.
TEST_F(AssertProbeDeathTest, ClosingAFrameThatIsNotOpenAborts)
{
	Renderer renderer{};
	EXPECT_DEATH(renderer.end_frame(), "end_frame: no frame is open");
}

// The run's outcome is stored, not read off the player, so a lost run with a player who
// still has hit points is the two drifting apart. The death screen refuses to open on it.
TEST_F(AssertProbeDeathTest, ALostRunWithALivingPlayerAborts)
{
	mock.player = std::make_unique<Player>(Vector2D{ 4, 5 });
	mock.player->healthPool = std::make_unique<HealthPool>(10);
	ASSERT_FALSE(ctx.player()->is_dead()) << "the mock's player starts dead, so nothing here is being measured";
	ctx.gameState->set_run_outcome(RunOutcome::DEFEAT);
	EXPECT_DEATH(ctx.gameLoopCoordinator->open_end_of_run_screen(ctx), "open_end_of_run_screen: the run is lost while the player still has hit points");
}

// Game::tick opens an end screen only when the outcome was decided during the tick, so
// a call with the run still undecided is a caller that skipped that check.
TEST_F(AssertProbeDeathTest, OpeningAnEndScreenOnARunThatHasNotEndedAborts)
{
	ASSERT_EQ(ctx.gameState->get_run_outcome(), RunOutcome::UNDECIDED) << "the mock's run has already ended, so nothing here is being measured";
	EXPECT_DEATH(ctx.gameLoopCoordinator->open_end_of_run_screen(ctx), "open_end_of_run_screen: the run has not ended");
}

// A list with no rows has no current row, so reading one is a caller that skipped is_empty().
TEST_F(AssertProbeDeathTest, ReadingTheCurrentRowOfAnEmptyListAborts)
{
	const ListCursor cursor{};
	EXPECT_DEATH([[maybe_unused]] const size_t row = cursor.current_row(), "current_row: the list is empty");
}

// Stepping down from the last row is a caller that skipped can_move_down().
TEST_F(AssertProbeDeathTest, MovingDownFromTheLastRowAborts)
{
	ListCursor cursor{};
	cursor.reset(1);
	EXPECT_DEATH(cursor.move_down(3), "move_down: the cursor cannot move down");
}

// Selecting a row the list does not have is a caller that read its row from somewhere else.
TEST_F(AssertProbeDeathTest, SelectingARowPastTheEndAborts)
{
	ListCursor cursor{};
	cursor.reset(10);
	EXPECT_DEATH(cursor.select(10), "select: the row is past the end of the list");
}

// A value already above its bound has no clamped step to take.
TEST_F(AssertProbeDeathTest, SteppingAValueAboveItsBoundAborts)
{
	EXPECT_DEATH([[maybe_unused]] const size_t stepped = step_within(10, 1, 9), "step_within: value is above its own bound");
}
