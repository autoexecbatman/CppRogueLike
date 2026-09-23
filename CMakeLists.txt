cmake_minimum_required(VERSION 3.13...3.21)

# Project definition
project(
    C++RogueLike
    LANGUAGES C CXX
)

# Export compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ensure the C++23 standard is available
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable parallel builds for MSVC
if(MSVC)
    add_compile_options(/MP10)
endif()

# Check if we're compiling with Emscripten
if(EMSCRIPTEN)
    message(STATUS "Building with Emscripten")

    # Common compilation options
    add_compile_options(-O2 -Wall)

    # Fetch dependencies for Emscripten build
    include(FetchContent)
    set(PLATFORM Web CACHE INTERNAL "")
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
    )
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
    )
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib nlohmann_json)
    set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)

    # ASYNCIFY is required: raylib 5.5 calls emscripten_sleep internally (glfwPollEvents,
    # glfwSwapBuffers) and cannot be prevented. Input correctness is handled by calling
    # PollInputEvents() explicitly in handle_input_phase before reading key state.
    set(EMCC_FLAGS "-sWASM=1 -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=['cwrap'] -sASSERTIONS=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 -sEXPORTED_FUNCTIONS=['_main','_malloc'] -sNO_DISABLE_EXCEPTION_CATCHING -sUSE_GLFW=3 -sGL_ENABLE_GET_PROC_ADDRESS=1 -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536")

    # Copy all JSON files to the build directory
    file(GLOB JSON_FILES ${CMAKE_SOURCE_DIR}/src/json/*.json)
    file(COPY ${JSON_FILES} DESTINATION "${CMAKE_BINARY_DIR}")

    # Set linker flags with shell file and preload all JSON files at once
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EMCC_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/shell.html")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file strength.json --preload-file dexterity.json --preload-file constitution.json --preload-file wisdom.json --preload-file intelligence.json --preload-file charisma.json")
    # Copy DawnLike tileset for web build
    if(EXISTS "${CMAKE_SOURCE_DIR}/DawnLike")
        file(COPY "${CMAKE_SOURCE_DIR}/DawnLike" DESTINATION "${CMAKE_BINARY_DIR}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file DawnLike")
    endif()

    # Copy data directory for web build (tile configs, prefabs, content)
    if(EXISTS "${CMAKE_SOURCE_DIR}/data")
        file(COPY "${CMAKE_SOURCE_DIR}/data" DESTINATION "${CMAKE_BINARY_DIR}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file data")
    endif()

    # Set output suffix for web
    set(OUTPUT_SUFFIX ".html")
else()
    # For native build, use vcpkg if available
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE
            "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    endif()

    # Set output suffix for native
    set(OUTPUT_SUFFIX "")
endif()

# Set the output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

# Set the source directory
set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")

# Set source files
set(SOURCE_FILES # Add Source Files Here !
    # Game
    ${PROJECT_SOURCE_DIR}/Game.cpp
    ${PROJECT_SOURCE_DIR}/Game.h
    ${PROJECT_SOURCE_DIR}/Paths.h
    ${PROJECT_SOURCE_DIR}/GameContext.h

    # Renderer (raylib)
    ${PROJECT_SOURCE_DIR}/Renderer.cpp
    ${PROJECT_SOURCE_DIR}/Renderer.h
    ${PROJECT_SOURCE_DIR}/InputSystem.cpp
    ${PROJECT_SOURCE_DIR}/InputSystem.h

    # Actor
    ${PROJECT_SOURCE_DIR}/Actor.cpp
    ${PROJECT_SOURCE_DIR}/Actor.h
    ${PROJECT_SOURCE_DIR}/Attacker.cpp
    ${PROJECT_SOURCE_DIR}/Attacker.h
    ${PROJECT_SOURCE_DIR}/AttackStrength.cpp
    ${PROJECT_SOURCE_DIR}/AttackStrength.h
    ${PROJECT_SOURCE_DIR}/AttackResult.h
    ${PROJECT_SOURCE_DIR}/MonsterAttacker.cpp
    ${PROJECT_SOURCE_DIR}/MonsterAttacker.h
    ${PROJECT_SOURCE_DIR}/PlayerAttacker.cpp
    ${PROJECT_SOURCE_DIR}/PlayerAttacker.h
    ${PROJECT_SOURCE_DIR}/Pickable.cpp
    ${PROJECT_SOURCE_DIR}/Pickable.h
    ${PROJECT_SOURCE_DIR}/InventoryOperations.cpp
    ${PROJECT_SOURCE_DIR}/InventoryOperations.h
    ${PROJECT_SOURCE_DIR}/InventoryData.h
    ${PROJECT_SOURCE_DIR}/Creature.cpp
    ${PROJECT_SOURCE_DIR}/Creature.h
    ${PROJECT_SOURCE_DIR}/Item.cpp
    ${PROJECT_SOURCE_DIR}/Item.h
    ${PROJECT_SOURCE_DIR}/NPC.h
    ${PROJECT_SOURCE_DIR}/Alignment.h
    ${PROJECT_SOURCE_DIR}/CreatureClass.h
    ${PROJECT_SOURCE_DIR}/TurnUndead.cpp
    ${PROJECT_SOURCE_DIR}/TurnUndead.h
    ${PROJECT_SOURCE_DIR}/TurningTable.h
    ${PROJECT_SOURCE_DIR}/TileFeature.h
    ${PROJECT_SOURCE_DIR}/Stairs.cpp
    ${PROJECT_SOURCE_DIR}/Stairs.h

    # Persistent
    ${PROJECT_SOURCE_DIR}/Persistent.cpp
    ${PROJECT_SOURCE_DIR}/Persistent.h

    # Ai - Updated paths
    ${PROJECT_SOURCE_DIR}/Ai.cpp
    ${PROJECT_SOURCE_DIR}/Ai.h
    ${PROJECT_SOURCE_DIR}/AiMonster.cpp
    ${PROJECT_SOURCE_DIR}/AiMonster.h
    ${PROJECT_SOURCE_DIR}/AiMonsterConfused.cpp
    ${PROJECT_SOURCE_DIR}/AiMonsterConfused.h
    ${PROJECT_SOURCE_DIR}/PlayerController.cpp
    ${PROJECT_SOURCE_DIR}/PlayerController.h
    ${PROJECT_SOURCE_DIR}/AiShopkeeper.cpp
    ${PROJECT_SOURCE_DIR}/AiShopkeeper.h
    ${PROJECT_SOURCE_DIR}/AiMimic.cpp
    ${PROJECT_SOURCE_DIR}/AiMimic.h
    ${PROJECT_SOURCE_DIR}/AiMonsterRanged.cpp
    ${PROJECT_SOURCE_DIR}/AiMonsterRanged.h
    ${PROJECT_SOURCE_DIR}/AiSpider.cpp
    ${PROJECT_SOURCE_DIR}/AiSpider.h
    ${PROJECT_SOURCE_DIR}/AiWebSpinner.cpp
    ${PROJECT_SOURCE_DIR}/AiGiantSpider.cpp
    ${PROJECT_SOURCE_DIR}/AiGiantSpider.h
    ${PROJECT_SOURCE_DIR}/AiWebSpinner.h

    # dnd_tables
    ${PROJECT_SOURCE_DIR}/CombatProgressionTables.h

    # Random
    ${PROJECT_SOURCE_DIR}/RandomDice.h

    # Attributes
    ${PROJECT_SOURCE_DIR}/CharismaAttributes.h
    ${PROJECT_SOURCE_DIR}/ConstitutionAttributes.h
    ${PROJECT_SOURCE_DIR}/DexterityAttributes.h
    ${PROJECT_SOURCE_DIR}/IntelligenceAttributes.h
    ${PROJECT_SOURCE_DIR}/StrengthAttributes.h
    ${PROJECT_SOURCE_DIR}/WisdomAttributes.h

    # Colors
    ${PROJECT_SOURCE_DIR}/Colors.cpp
    ${PROJECT_SOURCE_DIR}/Colors.h

    # Controls
    ${PROJECT_SOURCE_DIR}/Controls.h

    # Menu
    ${PROJECT_SOURCE_DIR}/AnimationBarrierMenu.cpp
    ${PROJECT_SOURCE_DIR}/AnimationBarrierMenu.h
    ${PROJECT_SOURCE_DIR}/BaseMenu.cpp
    ${PROJECT_SOURCE_DIR}/BaseMenu.h
    ${PROJECT_SOURCE_DIR}/MenuEntry.h
    ${PROJECT_SOURCE_DIR}/ListMenu.cpp
    ${PROJECT_SOURCE_DIR}/ListMenu.h
    ${PROJECT_SOURCE_DIR}/NotificationMenu.cpp
    ${PROJECT_SOURCE_DIR}/NotificationMenu.h
    ${PROJECT_SOURCE_DIR}/Menu.cpp
    ${PROJECT_SOURCE_DIR}/Menu.h
    ${PROJECT_SOURCE_DIR}/MenuClass.cpp
    ${PROJECT_SOURCE_DIR}/MenuClass.h
    ${PROJECT_SOURCE_DIR}/MenuGender.cpp
    ${PROJECT_SOURCE_DIR}/MenuGender.h
    ${PROJECT_SOURCE_DIR}/MenuName.cpp
    ${PROJECT_SOURCE_DIR}/MenuName.h
    ${PROJECT_SOURCE_DIR}/MenuRace.cpp
    ${PROJECT_SOURCE_DIR}/MenuRace.h
    ${PROJECT_SOURCE_DIR}/MenuBuy.cpp
    ${PROJECT_SOURCE_DIR}/MenuBuy.h
    ${PROJECT_SOURCE_DIR}/MenuSell.cpp
    ${PROJECT_SOURCE_DIR}/MenuSell.h
    ${PROJECT_SOURCE_DIR}/MenuTrade.cpp
    ${PROJECT_SOURCE_DIR}/MenuTrade.h
    ${PROJECT_SOURCE_DIR}/MenuSpellCast.cpp
    ${PROJECT_SOURCE_DIR}/MenuSpellCast.h
    ${PROJECT_SOURCE_DIR}/ContextMenu.cpp
    ${PROJECT_SOURCE_DIR}/ContextMenu.h
    ${PROJECT_SOURCE_DIR}/DeathMenu.cpp
    ${PROJECT_SOURCE_DIR}/DeathMenu.h

    # ActorTypes
    ${PROJECT_SOURCE_DIR}/Monsters.cpp
    ${PROJECT_SOURCE_DIR}/Monsters.h
    ${PROJECT_SOURCE_DIR}/Player.cpp
    ${PROJECT_SOURCE_DIR}/Player.h
    ${PROJECT_SOURCE_DIR}/Spider.cpp
    ${PROJECT_SOURCE_DIR}/Spider.h

    # Gui
    ${PROJECT_SOURCE_DIR}/Gui.cpp
    ${PROJECT_SOURCE_DIR}/Gui.h
    ${PROJECT_SOURCE_DIR}/LogMessage.cpp
    ${PROJECT_SOURCE_DIR}/LogMessage.h

    # Map
    ${PROJECT_SOURCE_DIR}/Map.cpp
    ${PROJECT_SOURCE_DIR}/Map.h
    ${PROJECT_SOURCE_DIR}/TreasureRoom.cpp
    ${PROJECT_SOURCE_DIR}/TreasureRoom.h
    ${PROJECT_SOURCE_DIR}/TileDefinition.h
    ${PROJECT_SOURCE_DIR}/TileType.h
    ${PROJECT_SOURCE_DIR}/DungeonRoom.h
    ${PROJECT_SOURCE_DIR}/DungeonGenerator.cpp
    ${PROJECT_SOURCE_DIR}/DungeonGenerator.h
    ${PROJECT_SOURCE_DIR}/DungeonNames.cpp
    ${PROJECT_SOURCE_DIR}/DungeonNames.h
    ${PROJECT_SOURCE_DIR}/FovMap.cpp
    ${PROJECT_SOURCE_DIR}/FovMap.h
    ${PROJECT_SOURCE_DIR}/Minimap.cpp
    ${PROJECT_SOURCE_DIR}/Minimap.h
    ${PROJECT_SOURCE_DIR}/Decoration.h

    # Items - Updated paths
    ${PROJECT_SOURCE_DIR}/ItemClassification.cpp
    ${PROJECT_SOURCE_DIR}/ItemClassification.h
    ${PROJECT_SOURCE_DIR}/Weapons.cpp
    ${PROJECT_SOURCE_DIR}/Weapons.h
    ${PROJECT_SOURCE_DIR}/MagicalItemEffects.cpp
    ${PROJECT_SOURCE_DIR}/MagicalItemEffects.h

    # Systems - Updated paths
    ${PROJECT_SOURCE_DIR}/TargetingMenu.cpp
    ${PROJECT_SOURCE_DIR}/TargetingMenu.h
    ${PROJECT_SOURCE_DIR}/TargetingSystem.cpp
    ${PROJECT_SOURCE_DIR}/TargetingSystem.h
    ${PROJECT_SOURCE_DIR}/SpellAnimations.cpp
    ${PROJECT_SOURCE_DIR}/SpellAnimations.h
    ${PROJECT_SOURCE_DIR}/TargetMode.h
    ${PROJECT_SOURCE_DIR}/EncounterPlanner.cpp
    ${PROJECT_SOURCE_DIR}/EncounterPlanner.h
    ${PROJECT_SOURCE_DIR}/SpawnUtils.cpp
    ${PROJECT_SOURCE_DIR}/SpawnUtils.h
    ${PROJECT_SOURCE_DIR}/HungerSystem.cpp
    ${PROJECT_SOURCE_DIR}/HungerSystem.h
    ${PROJECT_SOURCE_DIR}/BuffSystem.cpp
    ${PROJECT_SOURCE_DIR}/BuffSystem.h
    ${PROJECT_SOURCE_DIR}/CurseSystem.cpp
    ${PROJECT_SOURCE_DIR}/CurseSystem.h
    ${PROJECT_SOURCE_DIR}/LevelUpSystem.cpp
    ${PROJECT_SOURCE_DIR}/LevelUpSystem.h
    ${PROJECT_SOURCE_DIR}/MessageSystem.cpp
    ${PROJECT_SOURCE_DIR}/MessageSystem.h
    ${PROJECT_SOURCE_DIR}/RenderingManager.cpp
    ${PROJECT_SOURCE_DIR}/RenderingManager.h
    ${PROJECT_SOURCE_DIR}/InputHandler.cpp
    ${PROJECT_SOURCE_DIR}/InputHandler.h
    ${PROJECT_SOURCE_DIR}/GameStateManager.cpp
    ${PROJECT_SOURCE_DIR}/GameStateManager.h
    ${PROJECT_SOURCE_DIR}/LevelManager.cpp
    ${PROJECT_SOURCE_DIR}/LevelManager.h
    ${PROJECT_SOURCE_DIR}/CreatureManager.cpp
    ${PROJECT_SOURCE_DIR}/CreatureManager.h
    ${PROJECT_SOURCE_DIR}/MenuManager.cpp
    ${PROJECT_SOURCE_DIR}/MenuManager.h
    ${PROJECT_SOURCE_DIR}/DisplayManager.cpp
    ${PROJECT_SOURCE_DIR}/DisplayManager.h
    ${PROJECT_SOURCE_DIR}/GameLoopCoordinator.cpp
    ${PROJECT_SOURCE_DIR}/GameLoopCoordinator.h
    ${PROJECT_SOURCE_DIR}/DataManager.cpp
    ${PROJECT_SOURCE_DIR}/DataManager.h
    ${PROJECT_SOURCE_DIR}/ShopKeeper.cpp
    ${PROJECT_SOURCE_DIR}/ShopKeeper.h
    ${PROJECT_SOURCE_DIR}/ShopkeeperFactory.cpp
    ${PROJECT_SOURCE_DIR}/ShopkeeperFactory.h
    ${PROJECT_SOURCE_DIR}/ItemEnhancements.cpp
    ${PROJECT_SOURCE_DIR}/ItemEnhancements.h
    ${PROJECT_SOURCE_DIR}/SpellRegistry.cpp
    ${PROJECT_SOURCE_DIR}/SpellRegistry.h
    ${PROJECT_SOURCE_DIR}/SpellSystem.cpp
    ${PROJECT_SOURCE_DIR}/SpellSystem.h
    ${PROJECT_SOURCE_DIR}/FloatingTextSystem.cpp
    ${PROJECT_SOURCE_DIR}/FloatingTextSystem.h
    ${PROJECT_SOURCE_DIR}/AnimationSystem.cpp
    ${PROJECT_SOURCE_DIR}/AnimationSystem.h
    ${PROJECT_SOURCE_DIR}/ContentRegistry.cpp
    ${PROJECT_SOURCE_DIR}/ContentRegistry.h
    ${PROJECT_SOURCE_DIR}/ContentRegistryIO.cpp
    ${PROJECT_SOURCE_DIR}/ContentRegistryIO.h
    ${PROJECT_SOURCE_DIR}/BodyPlanRegistry.cpp
    ${PROJECT_SOURCE_DIR}/BodyPlanRegistry.h
    ${PROJECT_SOURCE_DIR}/TileConfig.cpp
    ${PROJECT_SOURCE_DIR}/TileConfig.h

    # Tools
    ${PROJECT_SOURCE_DIR}/BalanceViewer.cpp
    ${PROJECT_SOURCE_DIR}/BalanceViewer.h
    ${PROJECT_SOURCE_DIR}/ContentEditor.cpp
    ${PROJECT_SOURCE_DIR}/ContentEditor.h
    ${PROJECT_SOURCE_DIR}/DecorEditor.cpp
    ${PROJECT_SOURCE_DIR}/DecorEditor.h
    ${PROJECT_SOURCE_DIR}/ItemEditor.cpp
    ${PROJECT_SOURCE_DIR}/ItemEditor.h
    ${PROJECT_SOURCE_DIR}/MonsterEditor.cpp
    ${PROJECT_SOURCE_DIR}/MonsterEditor.h
    ${PROJECT_SOURCE_DIR}/PrefabLibrary.cpp
    ${PROJECT_SOURCE_DIR}/PrefabLibrary.h
    ${PROJECT_SOURCE_DIR}/RoomEditor.cpp
    ${PROJECT_SOURCE_DIR}/RoomEditor.h
    ${PROJECT_SOURCE_DIR}/SpellEditor.cpp
    ${PROJECT_SOURCE_DIR}/SpellEditor.h

    # Factories - Updated paths
    ${PROJECT_SOURCE_DIR}/MonsterFactory.cpp
    ${PROJECT_SOURCE_DIR}/MonsterFactory.h
    ${PROJECT_SOURCE_DIR}/MonsterCreator.cpp
    ${PROJECT_SOURCE_DIR}/MonsterCreator.h
    ${PROJECT_SOURCE_DIR}/MonsterRegistry.cpp
    ${PROJECT_SOURCE_DIR}/MonsterRegistry.h
    ${PROJECT_SOURCE_DIR}/ItemFactory.cpp
    ${PROJECT_SOURCE_DIR}/ItemFactory.h
    ${PROJECT_SOURCE_DIR}/ItemCreator.cpp
    ${PROJECT_SOURCE_DIR}/ItemCreator.h
    ${PROJECT_SOURCE_DIR}/ItemRegistry.cpp
    ${PROJECT_SOURCE_DIR}/ItemRegistry.h

    # Objects - Updated paths
    ${PROJECT_SOURCE_DIR}/Web.cpp
    ${PROJECT_SOURCE_DIR}/Web.h
    ${PROJECT_SOURCE_DIR}/Trap.cpp
    ${PROJECT_SOURCE_DIR}/Trap.h
    ${PROJECT_SOURCE_DIR}/SpellTile.h

    # Utils - Updated paths
    ${PROJECT_SOURCE_DIR}/Dijkstra.cpp
    ${PROJECT_SOURCE_DIR}/Dijkstra.h
    ${PROJECT_SOURCE_DIR}/Vector2D.h
    ${PROJECT_SOURCE_DIR}/UniqueId.cpp
    ${PROJECT_SOURCE_DIR}/UniqueId.h

    # Combat - Weapon damage system
    ${PROJECT_SOURCE_DIR}/DeathHandler.cpp
    ${PROJECT_SOURCE_DIR}/GameContext.cpp
    ${PROJECT_SOURCE_DIR}/DeathHandler.h
    ${PROJECT_SOURCE_DIR}/WeaponDamageRegistry.cpp
    ${PROJECT_SOURCE_DIR}/WeaponDamageRegistry.h
    ${PROJECT_SOURCE_DIR}/AttackKind.h
    ${PROJECT_SOURCE_DIR}/DamageInfo.h
    ${PROJECT_SOURCE_DIR}/ArmorClass.cpp
    ${PROJECT_SOURCE_DIR}/ArmorClass.h
    ${PROJECT_SOURCE_DIR}/DamageResolver.cpp
    ${PROJECT_SOURCE_DIR}/DamageResolver.h
    ${PROJECT_SOURCE_DIR}/SavingThrow.cpp
    ${PROJECT_SOURCE_DIR}/SavingThrow.h
    ${PROJECT_SOURCE_DIR}/ConstitutionTracker.cpp
    ${PROJECT_SOURCE_DIR}/ConstitutionTracker.h
    ${PROJECT_SOURCE_DIR}/HealthPool.cpp
    ${PROJECT_SOURCE_DIR}/HealthPool.h
    ${PROJECT_SOURCE_DIR}/CombatStats.cpp
    ${PROJECT_SOURCE_DIR}/CombatStats.h
    ${PROJECT_SOURCE_DIR}/ExperienceReward.cpp
    ${PROJECT_SOURCE_DIR}/ExperienceReward.h
    ${PROJECT_SOURCE_DIR}/CorpseData.cpp
    ${PROJECT_SOURCE_DIR}/CorpseData.h

    # UI - New UI classes
    ${PROJECT_SOURCE_DIR}/CloseButtonArea.cpp
    ${PROJECT_SOURCE_DIR}/CloseButtonArea.h
    ${PROJECT_SOURCE_DIR}/InventoryUI.cpp
    ${PROJECT_SOURCE_DIR}/InventoryUI.h
    ${PROJECT_SOURCE_DIR}/LevelUpUI.cpp
    ${PROJECT_SOURCE_DIR}/LevelUpUI.h
    ${PROJECT_SOURCE_DIR}/CharacterSheetUI.cpp
    ${PROJECT_SOURCE_DIR}/CharacterSheetUI.h

    # Main
    ${PROJECT_SOURCE_DIR}/main.cpp
)

# Create an executable with the listed source files
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE
    <algorithm>
    <cassert>
    <format>
    <memory>
    <optional>
    <ranges>
    <string>
    <vector>
)

# Set output suffix based on platform
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX "${OUTPUT_SUFFIX}")

if(EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "index")
endif()

if(EMSCRIPTEN)
    # Include directories for all dependencies
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        raylib
        nlohmann_json::nlohmann_json
    )

else()
    # For native build, link against required libraries
    find_package(raylib CONFIG REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)

    target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
            raylib
            nlohmann_json::nlohmann_json
    )

    # Enforce UTF-8 encoding on MSVC
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /MP10 /utf-8)

        # Set the working directory for Visual Studio debugging
        set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    endif()

    # Copy JSON files to build directory
    set(JSON_FILES
        "${PROJECT_SOURCE_DIR}/json/strength.json"
        "${PROJECT_SOURCE_DIR}/json/dexterity.json"
        "${PROJECT_SOURCE_DIR}/json/constitution.json"
        "${PROJECT_SOURCE_DIR}/json/wisdom.json"
        "${PROJECT_SOURCE_DIR}/json/intelligence.json"
        "${PROJECT_SOURCE_DIR}/json/charisma.json"
    )

    # Ensure the data directory exists in the source tree (editor-authored files live here)
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/data")

    foreach(JSON_FILE ${JSON_FILES})
        get_filename_component(JSON_FILENAME ${JSON_FILE} NAME)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                  ${JSON_FILE}
                                  $<TARGET_FILE_DIR:${PROJECT_NAME}>/${JSON_FILENAME})
    endforeach()

    # Copy DawnLike tileset directory to build output
    if(EXISTS "${CMAKE_SOURCE_DIR}/DawnLike")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/DawnLike"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/DawnLike")
    endif()

    # Add tests subdirectory (only for native builds)
    option(BUILD_TESTS "Build unit tests" ON)
    if(BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()

endif()
