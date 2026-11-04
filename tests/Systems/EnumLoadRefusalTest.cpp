// file: EnumLoadRefusalTest.cpp
// An enum read back from a save or a data file is a boundary: a number no
// enumerator names is refused through saved_enum_field, whose message names
// the loader, the field, the number and the enum.
//
// One test per read site, because each site must call its check. Each builds a
// valid record by saving a real object, sets one field to one past the enum's
// last enumerator, and expects the std::runtime_error. Beside each, a record
// holding the last enumerator still loads, so the check refuses only what no
// enumerator names.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=EnumLoadRefusal*

#include <gtest/gtest.h>

#include <array>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "src/Actor.h"
#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/CreatureClass.h"
#include "src/ExperienceReward.h"
#include "src/GameStateManager.h"
#include "src/HealthPool.h"
#include "src/HungerSystem.h"
#include "src/Item.h"
#include "src/ItemEnhancements.h"
#include "src/ItemIdentification.h"
#include "src/Lifespan.h"
#include "src/Map.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "src/PlayerRaceState.h"
#include "src/ShopKeeper.h"
#include "src/TileConfig.h"
#include "src/TileRef.h"
#include "src/TileRefJson.h"
#include "src/TileType.h"
#include "src/Trap.h"
#include "tests/mocks/MockGameContext.h"
#include "tests/mocks/TempTestPath.h"

namespace CppRL
{

	using json = nlohmann::json;

	namespace
	{
		// What a load threw, or empty when it loaded. Any std::exception is
		// read, so a record of the wrong type fails on the message rather than
		// escaping.
		//
		// Example:
		//   what_loading_threw(load_a_good_record); // -> ""
		//   what_loading_threw(load_a_bad_record);
		//   // -> "unknown prefix 'blunt'"
		template <typename Load>
		std::string what_loading_threw(Load load)
		{
			try
			{
				load();
			}
			catch (const std::exception& refusal)
			{
				return refusal.what();
			}
			return {};
		}

		// Every prefix's and suffix's key in a save, in enum order: the
		// enumerator's name in snake_case.
		const auto PREFIX_KEYS = std::to_array<std::string_view>({
			"none",
			"sharp",
			"keen",
			"masterwork",
			"blessed",
			"flaming",
			"frost",
			"shock",
			"reinforced",
			"studded",
			"elven",
			"dwarven",
			"magical",
			"cursed",
			"ancient",
			"rusted",
			"cracked",
		});

		const auto SUFFIX_KEYS = std::to_array<std::string_view>({
			"none",
			"of_slaying",
			"of_accuracy",
			"of_protection",
			"of_power",
			"of_fire_resistance",
			"of_cold_resistance",
			"of_lightning_resistance",
			"of_poison_resistance",
			"of_speed",
			"of_stealth",
			"of_magic",
			"of_health",
			"of_the_eagle",
			"of_the_bear",
			"of_the_owl",
			"of_weakness",
			"of_slowness",
			"of_brittleness",
		});

		// One past an enum's last enumerator, as a saved number.
		template <typename Enum>
		int one_past(Enum last)
		{
			return static_cast<int>(last) + 1;
		}

	} // namespace

	class EnumLoadRefusalTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ctx = mock.to_game_context();
			scratch = temp_test_path("record.json");
		}

		void TearDown() override { std::filesystem::remove(scratch); }

		// A saved character of the race given.
		json saved_player_of(PlayerRaceState race)
		{
			Player player{ Vector2D{ 1, 1 } };
			player.experienceReward = std::make_unique<ExperienceReward>(0);
			player.armorClass = std::make_unique<ArmorClass>(10);
			player.healthPool = std::make_unique<HealthPool>(20);
			player.set_creature_class(CreatureClass::FIGHTER);
			player.lifespan = Lifespan{ race, 20, 90, 0, 0 };
			json saved;
			player.save(saved);
			return saved;
		}

		// A saved plain item.
		json saved_item()
		{
			Item item{
				Vector2D{ 0, 0 },
				ActorData{ TileRef{}, "test", ColorPairId::WHITE_BLACK },
			};
			json saved;
			item.save(saved);
			return saved;
		}

		// What a saved item record loads as, saved again.
		json item_reloaded(const json& saved)
		{
			Item loaded{
				Vector2D{ 0, 0 },
				ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK },
			};
			loaded.load(saved);
			json again;
			loaded.save(again);
			return again;
		}

		// A saved level holding one trap of the type given.
		json saved_level_with_a(TrapType type)
		{
			std::vector<std::unique_ptr<Trap>> traps{};
			traps.push_back(
				std::make_unique<Trap>(
					Vector2D{ 3, 3 },
					type,
					mock.tile_config));
			json level;
			save_traps(traps, level);
			return level;
		}

		// A saved map of floor.
		json saved_map()
		{
			Map map{ 4, 4 };
			map.init_tiles();
			json saved;
			map.save(saved);
			return saved;
		}

		// The shipped tile config, its first `section` record given `sheet`,
		// written to this test's own file; damagedGroup names that record.
		void write_config_with_sheet(const std::string& section, int sheet)
		{
			std::ifstream source(Paths::resolve(Paths::TILE_CONFIG));
			ASSERT_TRUE(source.is_open())
				<< "cannot read " << Paths::TILE_CONFIG;
			json document = json::parse(source);
			ASSERT_FALSE(document.at(section).empty())
				<< "the shipped config has no " << section
				<< " record to damage";
			damagedGroup = document.at(section).begin().key();
			document.at(section).begin()->at("sheet") = sheet;
			std::ofstream out(scratch);
			out << document.dump(2);
		}

		MockGameContext mock{};
		GameContext ctx{};
		std::filesystem::path scratch{};
		std::string damagedGroup{};
	};

	TEST_F(EnumLoadRefusalTest, ARaceNoPlayerRaceStateNamesIsRefused)
	{
		json saved = saved_player_of(PlayerRaceState::HUMAN);
		const int bad = one_past(PlayerRaceState::HALFELF);
		saved.at("lifespan").at("race") = bad;
		const auto load_it = [&saved]()
		{
			Player loaded{ Vector2D{ 0, 0 } };
			loaded.load(saved);
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format(
				"Creature::load: race {} is not a valid PlayerRaceState",
				bad));
	}

	TEST_F(EnumLoadRefusalTest, TheLastPlayerRaceStateStillLoads)
	{
		const json saved = saved_player_of(PlayerRaceState::HALFELF);
		Player loaded{ Vector2D{ 0, 0 } };

		loaded.load(saved);

		ASSERT_TRUE(loaded.lifespan.has_value());
		EXPECT_EQ(loaded.lifespan->race, PlayerRaceState::HALFELF);
	}

	TEST_F(EnumLoadRefusalTest, ATrapTypeNoTrapTypeNamesIsRefused)
	{
		json level = saved_level_with_a(TrapType::PIT);
		const int bad = one_past(TrapType::ARROW);
		level.at("traps").at(0).at("type") = bad;
		const auto load_it = [this, &level]()
		{
			std::vector<std::unique_ptr<Trap>> restored{};
			load_traps(level, restored, *ctx.tileConfig);
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format("load_traps: type {} is not a valid TrapType", bad));
	}

	TEST_F(EnumLoadRefusalTest, TheLastTrapTypeStillLoads)
	{
		const json level = saved_level_with_a(TrapType::ARROW);
		std::vector<std::unique_ptr<Trap>> restored{};

		load_traps(level, restored, *ctx.tileConfig);

		ASSERT_EQ(restored.size(), 1u);
		EXPECT_EQ(restored.front()->get_type(), TrapType::ARROW);
	}

	TEST_F(EnumLoadRefusalTest, ATrapStateNoTrapStateNamesIsRefused)
	{
		json level = saved_level_with_a(TrapType::PIT);
		const int bad = one_past(TrapState::DISARMED);
		level.at("traps").at(0).at("state") = bad;
		const auto load_it = [this, &level]()
		{
			std::vector<std::unique_ptr<Trap>> restored{};
			load_traps(level, restored, *ctx.tileConfig);
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format("Trap::load: state {} is not a valid TrapState", bad));
	}

	TEST_F(EnumLoadRefusalTest, TheLastTrapStateStillLoads)
	{
		json level = saved_level_with_a(TrapType::PIT);
		level.at("traps").at(0).at("state") =
			static_cast<int>(TrapState::DISARMED);
		std::vector<std::unique_ptr<Trap>> restored{};

		load_traps(level, restored, *ctx.tileConfig);

		ASSERT_EQ(restored.size(), 1u);
		EXPECT_EQ(restored.front()->get_state(), TrapState::DISARMED);
	}

	TEST_F(EnumLoadRefusalTest, AHungerStateNoHungerStateNamesIsRefused)
	{
		json saved;
		HungerSystem{}.save(saved);
		const int bad = one_past(HungerState::DYING);
		saved["lastNotifiedState"] = bad;
		const auto load_it = [&saved]()
		{
			HungerSystem loaded{};
			loaded.load(saved);
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format(
				"HungerSystem::load: lastNotifiedState {} is not a valid HungerState",
				bad));
	}

	TEST_F(EnumLoadRefusalTest, TheLastHungerStateStillLoads)
	{
		json saved;
		HungerSystem{}.save(saved);
		saved["lastNotifiedState"] = static_cast<int>(HungerState::DYING);
		HungerSystem loaded{};

		loaded.load(saved);

		json again;
		loaded.save(again);
		EXPECT_EQ(
			again.at("lastNotifiedState"),
			static_cast<int>(HungerState::DYING));
	}

	// An item's class is saved by its key, so the refusal names the key.
	TEST_F(EnumLoadRefusalTest, AnItemClassNoItemClassNamesIsRefused)
	{
		json saved = saved_item();
		saved.at("itemClass") = "sandals";
		const auto load_it = [this, &saved]()
		{
			[[maybe_unused]] const json again = item_reloaded(saved);
		};

		EXPECT_EQ(what_loading_threw(load_it), "unknown item_class 'sandals'");
	}

	// A prefix and a suffix are saved by their keys, so the refusal names the
	// key.
	TEST_F(EnumLoadRefusalTest, APrefixNoPrefixTypeNamesIsRefused)
	{
		json saved = saved_item();
		saved.at("enhancement").at("prefix") = "blunt";
		const auto load_it = [this, &saved]()
		{
			[[maybe_unused]] const json again = item_reloaded(saved);
		};

		EXPECT_EQ(what_loading_threw(load_it), "unknown prefix 'blunt'");
	}

	TEST_F(EnumLoadRefusalTest, ASuffixNoSuffixTypeNamesIsRefused)
	{
		json saved = saved_item();
		saved.at("enhancement").at("suffix") = "of_sandals";
		const auto load_it = [this, &saved]()
		{
			[[maybe_unused]] const json again = item_reloaded(saved);
		};

		EXPECT_EQ(what_loading_threw(load_it), "unknown suffix 'of_sandals'");
	}

	TEST_F(EnumLoadRefusalTest, EverySavedPrefixIsItsKey)
	{
		ASSERT_EQ(std::size(PREFIX_KEYS), PREFIX_TABLE.size());
		for (std::size_t row = 0; row < PREFIX_TABLE.size(); ++row)
		{
			Item item{
				Vector2D{ 0, 0 },
				ActorData{ TileRef{}, "test", ColorPairId::WHITE_BLACK },
			};
			item.enhancement.prefix = PREFIX_TABLE[row].type;
			json saved;

			item.save(saved);

			EXPECT_EQ(
				saved.at("enhancement").at("prefix"),
				json(PREFIX_KEYS[row]))
				<< "row " << row;
		}
	}

	TEST_F(EnumLoadRefusalTest, EverySavedSuffixIsItsKey)
	{
		ASSERT_EQ(std::size(SUFFIX_KEYS), SUFFIX_TABLE.size());
		for (std::size_t row = 0; row < SUFFIX_TABLE.size(); ++row)
		{
			Item item{
				Vector2D{ 0, 0 },
				ActorData{ TileRef{}, "test", ColorPairId::WHITE_BLACK },
			};
			item.enhancement.suffix = SUFFIX_TABLE[row].type;
			json saved;

			item.save(saved);

			EXPECT_EQ(
				saved.at("enhancement").at("suffix"),
				json(SUFFIX_KEYS[row]))
				<< "row " << row;
		}
	}

	// Each key loads as its own row, so no two keys are swapped on the way in.
	TEST_F(EnumLoadRefusalTest, AKeyLoadsItsOwnRow)
	{
		for (std::size_t row = 0; row < PREFIX_TABLE.size(); ++row)
		{
			json saved = saved_item();
			saved.at("enhancement").at("prefix") = PREFIX_KEYS[row];
			Item loaded{
				Vector2D{ 0, 0 },
				ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK },
			};

			ASSERT_NO_THROW(loaded.load(saved)) << "prefix row " << row;

			EXPECT_EQ(loaded.enhancement.prefix, PREFIX_TABLE[row].type)
				<< "prefix row " << row;
		}
		for (std::size_t row = 0; row < SUFFIX_TABLE.size(); ++row)
		{
			json saved = saved_item();
			saved.at("enhancement").at("suffix") = SUFFIX_KEYS[row];
			Item loaded{
				Vector2D{ 0, 0 },
				ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK },
			};

			ASSERT_NO_THROW(loaded.load(saved)) << "suffix row " << row;

			EXPECT_EQ(loaded.enhancement.suffix, SUFFIX_TABLE[row].type)
				<< "suffix row " << row;
		}
	}

	TEST_F(EnumLoadRefusalTest, ABlessingNoBlessingStatusNamesIsRefused)
	{
		json saved = saved_item();
		const int bad = one_past(BlessingStatus::CURSED);
		saved.at("enhancement").at("blessing") = bad;
		const auto load_it = [this, &saved]()
		{
			[[maybe_unused]] const json again = item_reloaded(saved);
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format(
				"Item::load: blessing {} is not a valid BlessingStatus",
				bad));
	}

	TEST_F(EnumLoadRefusalTest, TheLastOfEachItemEnumStillLoads)
	{
		json saved = saved_item();
		saved.at("itemClass") = "quest_item";
		saved.at("enhancement").at("prefix") = "cracked";
		saved.at("enhancement").at("suffix") = "of_brittleness";
		saved.at("enhancement").at("blessing") =
			static_cast<int>(BlessingStatus::CURSED);

		const json again = item_reloaded(saved);

		EXPECT_EQ(again.at("itemClass"), saved.at("itemClass"));
		EXPECT_EQ(
			again.at("enhancement").at("prefix"),
			saved.at("enhancement").at("prefix"));
		EXPECT_EQ(
			again.at("enhancement").at("suffix"),
			saved.at("enhancement").at("suffix"));
		EXPECT_EQ(
			again.at("enhancement").at("blessing"),
			saved.at("enhancement").at("blessing"));
	}

	TEST_F(EnumLoadRefusalTest, ATileTypeNoTileTypeNamesIsRefused)
	{
		json saved = saved_map();
		const int bad = one_past(TileType::CORRIDOR);
		saved.at("tiles").at(0).at("type") = bad;
		const auto load_it = [&saved]()
		{
			Map loaded{ 4, 4 };
			loaded.load(saved);
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format("Map::load: type {} is not a valid TileType", bad));
	}

	TEST_F(EnumLoadRefusalTest, TheLastTileTypeStillLoads)
	{
		json saved = saved_map();
		saved.at("tiles").at(0).at("type") =
			static_cast<int>(TileType::CORRIDOR);
		Map loaded{ 4, 4 };

		loaded.load(saved);

		json again;
		loaded.save(again);
		EXPECT_EQ(
			again.at("tiles").at(0).at("type"),
			static_cast<int>(TileType::CORRIDOR));
	}

	TEST_F(EnumLoadRefusalTest, AShopTypeNoShopTypeNamesIsRefused)
	{
		json saved;
		ShopKeeper{ ShopType::GENERAL_STORE, ShopQuality::AVERAGE, mock.dice }
			.save(saved);
		const int bad = one_past(ShopType::ADVENTURING_GEAR);
		saved.at("shop_type") = bad;
		const auto load_it = [&saved]()
		{
			ShopKeeper loaded{};
			loaded.load(saved);
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format(
				"ShopKeeper::load: shop_type {} is not a valid ShopType",
				bad));
	}

	TEST_F(EnumLoadRefusalTest, AShopQualityNoShopQualityNamesIsRefused)
	{
		json saved;
		ShopKeeper{ ShopType::GENERAL_STORE, ShopQuality::AVERAGE, mock.dice }
			.save(saved);
		const int bad = one_past(ShopQuality::EXCELLENT);
		saved.at("shop_quality") = bad;
		const auto load_it = [&saved]()
		{
			ShopKeeper loaded{};
			loaded.load(saved);
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format(
				"ShopKeeper::load: shop_quality {} is not a valid ShopQuality",
				bad));
	}

	TEST_F(EnumLoadRefusalTest, TheLastShopTypeAndQualityStillLoad)
	{
		json saved;
		ShopKeeper{
			ShopType::ADVENTURING_GEAR,
			ShopQuality::EXCELLENT,
			mock.dice,
		}
			.save(saved);
		ShopKeeper loaded{};

		loaded.load(saved);

		json again;
		loaded.save(again);
		EXPECT_EQ(
			again.at("shop_type"),
			static_cast<int>(ShopType::ADVENTURING_GEAR));
		EXPECT_EQ(
			again.at("shop_quality"),
			static_cast<int>(ShopQuality::EXCELLENT));
	}

	TEST_F(EnumLoadRefusalTest, AnAutotileGroupSheetNoTileSheetNamesIsRefused)
	{
		const int bad = static_cast<int>(TileSheet::COUNT);
		write_config_with_sheet("autotile_groups", bad);
		const auto load_it = [this]()
		{
			TileConfig config{};
			config.load(scratch.string());
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format(
				"TileConfig::load: autotile group {}: sheet {} is not a valid TileSheet",
				damagedGroup,
				bad));
	}

	TEST_F(
		EnumLoadRefusalTest,
		AWallAutotileGroupSheetNoTileSheetNamesIsRefused)
	{
		const int bad = static_cast<int>(TileSheet::COUNT);
		write_config_with_sheet("wall_autotile_groups", bad);
		const auto load_it = [this]()
		{
			TileConfig config{};
			config.load(scratch.string());
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format(
				"TileConfig::load: wall autotile group {}: sheet {} is not a valid TileSheet",
				damagedGroup,
				bad));
	}

	TEST_F(EnumLoadRefusalTest, TheLastTileSheetStillLoadsInAnAutotileGroup)
	{
		write_config_with_sheet(
			"autotile_groups",
			static_cast<int>(TileSheet::SHEET_BARS));
		TileConfig config{};

		EXPECT_NO_THROW(config.load(scratch.string()));
	}

	TEST_F(EnumLoadRefusalTest, ATileSheetNoTileSheetNamesIsRefused)
	{
		const int bad = static_cast<int>(TileSheet::COUNT);
		const json record{ { "sheet", bad }, { "col", 0 }, { "row", 0 } };
		const auto load_it = [&record]()
		{
			[[maybe_unused]] const TileRef tile =
				TileRefJson::parse_tile(record);
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format(
				"TileRefJson::parse_tile: sheet {} is not a valid TileSheet",
				bad));
	}

	TEST_F(EnumLoadRefusalTest, TheLastTileSheetStillLoads)
	{
		const json record{
			{ "sheet", static_cast<int>(TileSheet::SHEET_BARS) },
			{ "col", 0 },
			{ "row", 0 },
		};

		EXPECT_EQ(TileRefJson::parse_tile(record).sheet, TileSheet::SHEET_BARS);
	}

	TEST_F(EnumLoadRefusalTest, AnActorStateNoActorStateNamesIsRefused)
	{
		json saved = saved_item();
		const int bad = one_past(ActorState::IS_SURPRISED);
		saved.at("states") = json::array({ bad });
		const auto load_it = [this, &saved]()
		{
			[[maybe_unused]] const json again = item_reloaded(saved);
		};

		EXPECT_EQ(
			what_loading_threw(load_it),
			std::format(
				"Actor::load: states {} is not a valid ActorState",
				bad));
	}

	TEST_F(EnumLoadRefusalTest, TheLastActorStateStillLoads)
	{
		json saved = saved_item();
		saved.at("states") =
			json::array({ static_cast<int>(ActorState::IS_SURPRISED) });

		const json again = item_reloaded(saved);

		EXPECT_EQ(
			again.at("states"),
			json::array({ static_cast<int>(ActorState::IS_SURPRISED) }));
	}

	// end of file: EnumLoadRefusalTest.cpp

} // namespace CppRL
