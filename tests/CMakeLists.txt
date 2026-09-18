cmake_minimum_required(VERSION 3.13...3.21)

project(C++RogueLike_Tests)

# Set C++ standard to match main project
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(GTest CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Include directories from main project
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Test source files - explicitly listed
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/AssertProbeTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils/UniqueIdTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/GameContextWiringTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Vector2DTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Random/DiceExprTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/DamageInfoTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/WeaponDamageRegistryTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/AttackKindTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/AttackerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/WornResistanceTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/SavingThrowTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/TypedAttackResistanceTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Factories/ItemCreatorTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Factories/MonsterCreatorTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Factories/MonsterEquipmentTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Factories/MonsterRoundTripTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Factories/ItemRoundTripTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Factories/EnhancedRulesLoadTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Factories/TreasureGoldTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Factories/ItemFactoryReloadTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Factories/MonsterFactoryReloadTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/LevelUpSystemTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/ShopKeeperSerializationTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/ShopStockLevelTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/ShopTransactionTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/EnhancementSystemTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/CreatureSerializationTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/ItemSerializationTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/PlayerSerializationTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/PickableAcBonusTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/PlayerVirtualInterfaceTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Ai/AiMimicTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Map/MapTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Map/TrapSpawnTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Map/TreasureRoomTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Map/WaterMovementTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Map/DescribeTileTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/EquipmentStatBonusTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/AttitudeTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/AwarenessTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/AlignmentTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/BodyPlanTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/EquipmentStorageTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/ProtectionFromEvilTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/TurningTableTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/TurnUndeadTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Objects/WebTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Objects/TrapDamageTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/BuffTypeEncodingTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/CodecRoundTripTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/SpellCodecRoundTripTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/HitPointProgressionTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/Thac0ProgressionTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/TileDefinitionTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/SleepSpellTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/FireballDamageTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/ConstitutionTrackerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/RangedWeaponTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/CarryWeightTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/MonsterHitDiceTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/SpiderAmbushTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/WebSpinnerSanctuaryTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/SanctuarySaveTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/SpellDataLoadTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/CurseSystemTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/HungerSystemTest.cpp
)

message(STATUS "Found ${CMAKE_CURRENT_LIST_LENGTH} test source files: ${TEST_SOURCES}")

# Explicitly list parent project sources (excludes main.cpp)
set(PARENT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(PARENT_SOURCES
    # Game
    ${PARENT_SOURCE_DIR}/Game.cpp

    # Renderer (raylib)
    ${PARENT_SOURCE_DIR}/Renderer.cpp
    ${PARENT_SOURCE_DIR}/InputSystem.cpp

    # Actor
    ${PARENT_SOURCE_DIR}/Actor.cpp
    ${PARENT_SOURCE_DIR}/Attacker.cpp
    ${PARENT_SOURCE_DIR}/MonsterAttacker.cpp
    ${PARENT_SOURCE_DIR}/PlayerAttacker.cpp
    ${PARENT_SOURCE_DIR}/Container.cpp
    ${PARENT_SOURCE_DIR}/Pickable.cpp
    ${PARENT_SOURCE_DIR}/InventoryOperations.cpp
    ${PARENT_SOURCE_DIR}/Stairs.cpp
    ${PARENT_SOURCE_DIR}/Creature.cpp
    ${PARENT_SOURCE_DIR}/Item.cpp

    # Persistent
    ${PARENT_SOURCE_DIR}/Persistent.cpp

    # Ai
    ${PARENT_SOURCE_DIR}/Ai.cpp
    ${PARENT_SOURCE_DIR}/AiMonster.cpp
    ${PARENT_SOURCE_DIR}/AiMonsterConfused.cpp
    ${PARENT_SOURCE_DIR}/PlayerController.cpp
    ${PARENT_SOURCE_DIR}/AiShopkeeper.cpp
    ${PARENT_SOURCE_DIR}/AiMimic.cpp
    ${PARENT_SOURCE_DIR}/AiMonsterRanged.cpp
    ${PARENT_SOURCE_DIR}/AiSpider.cpp
    ${PARENT_SOURCE_DIR}/AiWebSpinner.cpp

    # Colors
    ${PARENT_SOURCE_DIR}/Colors.cpp

    # Menu
    ${PARENT_SOURCE_DIR}/AnimationBarrierMenu.cpp
    ${PARENT_SOURCE_DIR}/BaseMenu.cpp
    ${PARENT_SOURCE_DIR}/ListMenu.cpp
    ${PARENT_SOURCE_DIR}/NotificationMenu.cpp
    ${PARENT_SOURCE_DIR}/Menu.cpp
    ${PARENT_SOURCE_DIR}/MenuClass.cpp
    ${PARENT_SOURCE_DIR}/MenuGender.cpp
    ${PARENT_SOURCE_DIR}/MenuName.cpp
    ${PARENT_SOURCE_DIR}/MenuRace.cpp
    ${PARENT_SOURCE_DIR}/MenuBuy.cpp
    ${PARENT_SOURCE_DIR}/MenuSell.cpp
    ${PARENT_SOURCE_DIR}/MenuTrade.cpp
    ${PARENT_SOURCE_DIR}/MenuSpellCast.cpp
    ${PARENT_SOURCE_DIR}/ContextMenu.cpp
    ${PARENT_SOURCE_DIR}/DeathMenu.cpp

    # ActorTypes
    ${PARENT_SOURCE_DIR}/Monsters.cpp
    ${PARENT_SOURCE_DIR}/Player.cpp
    ${PARENT_SOURCE_DIR}/Spider.cpp

    # Gui
    ${PARENT_SOURCE_DIR}/Gui.cpp
    ${PARENT_SOURCE_DIR}/LogMessage.cpp

    # Map
    ${PARENT_SOURCE_DIR}/Map.cpp
    ${PARENT_SOURCE_DIR}/TreasureRoom.cpp
    ${PARENT_SOURCE_DIR}/DungeonGenerator.cpp
    ${PARENT_SOURCE_DIR}/DungeonNames.cpp
    ${PARENT_SOURCE_DIR}/FovMap.cpp
    ${PARENT_SOURCE_DIR}/Minimap.cpp

    # Items
    ${PARENT_SOURCE_DIR}/ItemClassification.cpp
    ${PARENT_SOURCE_DIR}/Weapons.cpp
    ${PARENT_SOURCE_DIR}/MagicalItemEffects.cpp

    # Systems
    ${PARENT_SOURCE_DIR}/TargetingMenu.cpp
    ${PARENT_SOURCE_DIR}/TargetingSystem.cpp
    ${PARENT_SOURCE_DIR}/SpellAnimations.cpp
    ${PARENT_SOURCE_DIR}/EncounterPlanner.cpp
    ${PARENT_SOURCE_DIR}/SpawnUtils.cpp
    ${PARENT_SOURCE_DIR}/HungerSystem.cpp
    ${PARENT_SOURCE_DIR}/BuffSystem.cpp
    ${PARENT_SOURCE_DIR}/CurseSystem.cpp
    ${PARENT_SOURCE_DIR}/LevelUpSystem.cpp
    ${PARENT_SOURCE_DIR}/MessageSystem.cpp
    ${PARENT_SOURCE_DIR}/RenderingManager.cpp
    ${PARENT_SOURCE_DIR}/InputHandler.cpp
    ${PARENT_SOURCE_DIR}/GameStateManager.cpp
    ${PARENT_SOURCE_DIR}/LevelManager.cpp
    ${PARENT_SOURCE_DIR}/CreatureManager.cpp
    ${PARENT_SOURCE_DIR}/MenuManager.cpp
    ${PARENT_SOURCE_DIR}/DisplayManager.cpp
    ${PARENT_SOURCE_DIR}/GameLoopCoordinator.cpp
    ${PARENT_SOURCE_DIR}/AnimationSystem.cpp
    ${PARENT_SOURCE_DIR}/ContentRegistry.cpp
    ${PARENT_SOURCE_DIR}/ContentRegistryIO.cpp
    ${PARENT_SOURCE_DIR}/BodyPlanRegistry.cpp
    ${PARENT_SOURCE_DIR}/TileConfig.cpp
    ${PARENT_SOURCE_DIR}/FloatingTextSystem.cpp
    ${PARENT_SOURCE_DIR}/BalanceViewer.cpp
    ${PARENT_SOURCE_DIR}/ContentEditor.cpp
    ${PARENT_SOURCE_DIR}/DecorEditor.cpp
    ${PARENT_SOURCE_DIR}/ItemEditor.cpp
    ${PARENT_SOURCE_DIR}/MonsterEditor.cpp
    ${PARENT_SOURCE_DIR}/PrefabLibrary.cpp
    ${PARENT_SOURCE_DIR}/RoomEditor.cpp
    ${PARENT_SOURCE_DIR}/SpellEditor.cpp
    ${PARENT_SOURCE_DIR}/DataManager.cpp
    ${PARENT_SOURCE_DIR}/ShopKeeper.cpp
    ${PARENT_SOURCE_DIR}/ShopkeeperFactory.cpp
    ${PARENT_SOURCE_DIR}/ItemEnhancements.cpp
    ${PARENT_SOURCE_DIR}/SpellSystem.cpp

    # Factories
    ${PARENT_SOURCE_DIR}/MonsterFactory.cpp
    ${PARENT_SOURCE_DIR}/TurnUndead.cpp
    ${PARENT_SOURCE_DIR}/MonsterCreator.cpp
    ${PARENT_SOURCE_DIR}/ItemFactory.cpp
    ${PARENT_SOURCE_DIR}/ItemCreator.cpp

    # Objects
    ${PARENT_SOURCE_DIR}/Web.cpp
    ${PARENT_SOURCE_DIR}/Trap.cpp

    # Utils
    ${PARENT_SOURCE_DIR}/Dijkstra.cpp
    ${PARENT_SOURCE_DIR}/UniqueId.cpp

    # Combat
    ${PARENT_SOURCE_DIR}/DeathHandler.cpp
    ${PARENT_SOURCE_DIR}/GameContext.cpp
    ${PARENT_SOURCE_DIR}/WeaponDamageRegistry.cpp
    ${PARENT_SOURCE_DIR}/ArmorClass.cpp
    ${PARENT_SOURCE_DIR}/DamageResolver.cpp
    ${PARENT_SOURCE_DIR}/SavingThrow.cpp
    ${PARENT_SOURCE_DIR}/ConstitutionTracker.cpp
    ${PARENT_SOURCE_DIR}/HealthPool.cpp
    ${PARENT_SOURCE_DIR}/CombatStats.cpp
    ${PARENT_SOURCE_DIR}/ExperienceReward.cpp
    ${PARENT_SOURCE_DIR}/CorpseData.cpp

    # UI
    ${PARENT_SOURCE_DIR}/CloseButtonArea.cpp
    ${PARENT_SOURCE_DIR}/InventoryUI.cpp
    ${PARENT_SOURCE_DIR}/LevelUpUI.cpp
    ${PARENT_SOURCE_DIR}/CharacterSheetUI.cpp
)

message(STATUS "Found ${CMAKE_CURRENT_LIST_LENGTH} parent source files")

# Create test executable with both test sources and parent sources
add_executable(test_exe ${TEST_SOURCES} ${PARENT_SOURCES})

# Add TESTING_MODE definition for test-specific code
target_compile_definitions(test_exe PRIVATE TESTING_MODE)

# Link against Google Test and required libraries
target_link_libraries(test_exe
    PRIVATE
        GTest::gtest
        raylib
        nlohmann_json::nlohmann_json
)

# For MSVC, add compile options
if(MSVC)
    target_compile_options(test_exe PRIVATE /W3 /utf-8)
    set_target_properties(test_exe PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_exe>")
endif()

# Copy all JSON files from source to test binary directory
file(GLOB JSON_FILES "${PARENT_SOURCE_DIR}/json/*.json")

foreach(JSON_FILE ${JSON_FILES})
    get_filename_component(JSON_FILENAME ${JSON_FILE} NAME)
    add_custom_command(TARGET test_exe POST_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different
                              ${JSON_FILE}
                              $<TARGET_FILE_DIR:test_exe>/${JSON_FILENAME})
endforeach()

# Copy tileset BMPs to test build directory (optional - enables tile rendering)
if(EXISTS "${CMAKE_SOURCE_DIR}/tileset.bmp")
    add_custom_command(TARGET test_exe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tileset.bmp"
            $<TARGET_FILE_DIR:test_exe>/tileset.bmp)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/tileset1.bmp")
    add_custom_command(TARGET test_exe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tileset1.bmp"
            $<TARGET_FILE_DIR:test_exe>/tileset1.bmp)
endif()

# Add custom target to run tests
add_custom_target(test_run
    COMMAND test_exe
    DEPENDS test_exe
    WORKING_DIRECTORY $<TARGET_FILE_DIR:test_exe>
    COMMENT "Running unit tests"
)

# Enable CTest and add test with proper working directory
include(CTest)
enable_testing()
add_test(NAME unit_tests COMMAND test_exe)
set_tests_properties(unit_tests PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:test_exe>)
