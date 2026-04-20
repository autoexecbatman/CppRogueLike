cmake_minimum_required(VERSION 3.13...3.21)

project(C++RogueLike_Tests)

# Set C++ standard to match main project
set(CMAKE_CXX_STANDARD 20)
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
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils/UniqueIdTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils/Vector2DTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/DamageInfoTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/WeaponDamageRegistryTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Combat/AttackerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Factories/ItemCreatorTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Factories/MonsterCreatorTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/LevelUpSystemTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/ShopKeeperSerializationTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/EnhancementSystemTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/DestructibleSerializationTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/DestructibleEdgeCaseTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/CreatureSerializationTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/ItemSerializationTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/PlayerSerializationTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/PickableAcBonusTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/PlayerVirtualInterfaceTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Ai/AiMimicTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Map/MapTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Actor/EquipmentStatBonusTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/CurseSystemTest.cpp
)

message(STATUS "Found ${CMAKE_CURRENT_LIST_LENGTH} test source files: ${TEST_SOURCES}")

# Explicitly list parent project sources (excludes main.cpp)
set(PARENT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(PARENT_SOURCES
    # Game
    ${PARENT_SOURCE_DIR}/Game.cpp

    # Renderer (raylib)
    ${PARENT_SOURCE_DIR}/Renderer/Renderer.cpp
    ${PARENT_SOURCE_DIR}/Renderer/InputSystem.cpp
    ${PARENT_SOURCE_DIR}/Renderer/Panel.cpp

    # Actor
    ${PARENT_SOURCE_DIR}/Actor/Actor.cpp
    ${PARENT_SOURCE_DIR}/Actor/Attacker.cpp
    ${PARENT_SOURCE_DIR}/Actor/MonsterAttacker.cpp
    ${PARENT_SOURCE_DIR}/Actor/PlayerAttacker.cpp
    ${PARENT_SOURCE_DIR}/Actor/Container.cpp
    ${PARENT_SOURCE_DIR}/Actor/Destructible.cpp
    ${PARENT_SOURCE_DIR}/Actor/Pickable.cpp
    ${PARENT_SOURCE_DIR}/Actor/InventoryOperations.cpp
    ${PARENT_SOURCE_DIR}/Actor/Stairs.cpp
    ${PARENT_SOURCE_DIR}/Actor/Creature.cpp
    ${PARENT_SOURCE_DIR}/Actor/Item.cpp

    # Persistent
    ${PARENT_SOURCE_DIR}/Persistent/Persistent.cpp

    # Ai
    ${PARENT_SOURCE_DIR}/Ai/Ai.cpp
    ${PARENT_SOURCE_DIR}/Ai/AiMonster.cpp
    ${PARENT_SOURCE_DIR}/Ai/AiMonsterConfused.cpp
    ${PARENT_SOURCE_DIR}/Ai/PlayerController.cpp
    ${PARENT_SOURCE_DIR}/Ai/AiShopkeeper.cpp
    ${PARENT_SOURCE_DIR}/Ai/AiMimic.cpp
    ${PARENT_SOURCE_DIR}/Ai/AiMonsterRanged.cpp
    ${PARENT_SOURCE_DIR}/Ai/AiSpider.cpp
    ${PARENT_SOURCE_DIR}/Ai/AiWebSpinner.cpp

    # Colors
    ${PARENT_SOURCE_DIR}/Colors/Colors.cpp

    # Menu
    ${PARENT_SOURCE_DIR}/Menu/BaseMenu.cpp
    ${PARENT_SOURCE_DIR}/Menu/ListMenu.cpp
    ${PARENT_SOURCE_DIR}/Menu/NotificationMenu.cpp
    ${PARENT_SOURCE_DIR}/Menu/Menu.cpp
    ${PARENT_SOURCE_DIR}/Menu/MenuClass.cpp
    ${PARENT_SOURCE_DIR}/Menu/MenuGender.cpp
    ${PARENT_SOURCE_DIR}/Menu/MenuName.cpp
    ${PARENT_SOURCE_DIR}/Menu/MenuRace.cpp
    ${PARENT_SOURCE_DIR}/Menu/MenuBuy.cpp
    ${PARENT_SOURCE_DIR}/Menu/MenuSell.cpp
    ${PARENT_SOURCE_DIR}/Menu/MenuTrade.cpp
    ${PARENT_SOURCE_DIR}/Menu/MenuSpellCast.cpp
    ${PARENT_SOURCE_DIR}/Menu/ContextMenu.cpp
    ${PARENT_SOURCE_DIR}/Menu/DeathMenu.cpp

    # ActorTypes
    ${PARENT_SOURCE_DIR}/ActorTypes/Monsters.cpp
    ${PARENT_SOURCE_DIR}/ActorTypes/Player.cpp
    ${PARENT_SOURCE_DIR}/ActorTypes/Monsters/Spider.cpp

    # Gui
    ${PARENT_SOURCE_DIR}/Gui/Gui.cpp
    ${PARENT_SOURCE_DIR}/Gui/LogMessage.cpp

    # Map
    ${PARENT_SOURCE_DIR}/Map/Map.cpp
    ${PARENT_SOURCE_DIR}/Map/DungeonGenerator.cpp
    ${PARENT_SOURCE_DIR}/Map/FovMap.cpp
    ${PARENT_SOURCE_DIR}/Map/Minimap.cpp

    # Items
    ${PARENT_SOURCE_DIR}/Items/ItemClassification.cpp
    ${PARENT_SOURCE_DIR}/Items/Weapons.cpp
    ${PARENT_SOURCE_DIR}/Items/MagicalItemEffects.cpp

    # Systems
    ${PARENT_SOURCE_DIR}/Systems/TargetingMenu.cpp
    ${PARENT_SOURCE_DIR}/Systems/TargetingSystem.cpp
    ${PARENT_SOURCE_DIR}/Systems/SpellAnimations.cpp
    ${PARENT_SOURCE_DIR}/Systems/EncounterPlanner.cpp
    ${PARENT_SOURCE_DIR}/Systems/HungerSystem.cpp
    ${PARENT_SOURCE_DIR}/Systems/BuffSystem.cpp
    ${PARENT_SOURCE_DIR}/Systems/CurseSystem.cpp
    ${PARENT_SOURCE_DIR}/Systems/LevelUpSystem.cpp
    ${PARENT_SOURCE_DIR}/Systems/MessageSystem.cpp
    ${PARENT_SOURCE_DIR}/Systems/RenderingManager.cpp
    ${PARENT_SOURCE_DIR}/Systems/InputHandler.cpp
    ${PARENT_SOURCE_DIR}/Systems/GameStateManager.cpp
    ${PARENT_SOURCE_DIR}/Systems/LevelManager.cpp
    ${PARENT_SOURCE_DIR}/Systems/CreatureManager.cpp
    ${PARENT_SOURCE_DIR}/Systems/MenuManager.cpp
    ${PARENT_SOURCE_DIR}/Systems/DisplayManager.cpp
    ${PARENT_SOURCE_DIR}/Systems/GameLoopCoordinator.cpp
    ${PARENT_SOURCE_DIR}/Systems/AnimationSystem.cpp
    ${PARENT_SOURCE_DIR}/Systems/ContentRegistry.cpp
    ${PARENT_SOURCE_DIR}/Systems/ContentRegistryIO.cpp
    ${PARENT_SOURCE_DIR}/Systems/TileConfig.cpp
    ${PARENT_SOURCE_DIR}/Systems/FloatingTextSystem.cpp
    ${PARENT_SOURCE_DIR}/Tools/BalanceViewer.cpp
    ${PARENT_SOURCE_DIR}/Tools/ContentEditor.cpp
    ${PARENT_SOURCE_DIR}/Tools/DecorEditor.cpp
    ${PARENT_SOURCE_DIR}/Tools/ItemEditor.cpp
    ${PARENT_SOURCE_DIR}/Tools/MonsterEditor.cpp
    ${PARENT_SOURCE_DIR}/Tools/PrefabLibrary.cpp
    ${PARENT_SOURCE_DIR}/Tools/RoomEditor.cpp
    ${PARENT_SOURCE_DIR}/Tools/SpellEditor.cpp
    ${PARENT_SOURCE_DIR}/Systems/ObjectManager.cpp
    ${PARENT_SOURCE_DIR}/Systems/DataManager.cpp
    ${PARENT_SOURCE_DIR}/Systems/ShopKeeper.cpp
    ${PARENT_SOURCE_DIR}/Systems/Shopkeepers/ShopkeeperFactory.cpp
    ${PARENT_SOURCE_DIR}/Systems/ItemEnhancements/ItemEnhancements.cpp
    ${PARENT_SOURCE_DIR}/Systems/SpellSystem.cpp

    # Factories
    ${PARENT_SOURCE_DIR}/Factories/MonsterFactory.cpp
    ${PARENT_SOURCE_DIR}/Factories/MonsterCreator.cpp
    ${PARENT_SOURCE_DIR}/Factories/ItemFactory.cpp
    ${PARENT_SOURCE_DIR}/Factories/ItemCreator.cpp

    # Objects
    ${PARENT_SOURCE_DIR}/Objects/Web.cpp
    ${PARENT_SOURCE_DIR}/Objects/Trap.cpp

    # Utils
    ${PARENT_SOURCE_DIR}/Utils/Dijkstra.cpp
    ${PARENT_SOURCE_DIR}/Utils/UniqueId.cpp

    # Combat
    ${PARENT_SOURCE_DIR}/Combat/WeaponDamageRegistry.cpp

    # UI
    ${PARENT_SOURCE_DIR}/UI/InventoryUI.cpp
    ${PARENT_SOURCE_DIR}/UI/LevelUpUI.cpp
    ${PARENT_SOURCE_DIR}/UI/CharacterSheetUI.cpp
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
