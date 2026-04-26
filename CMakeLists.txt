cmake_minimum_required(VERSION 3.13...3.21)

# Project definition
project(
    C++RogueLike
    LANGUAGES C CXX
)

# Export compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ensure the C++20 standard is available
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable parallel builds for MSVC
if(MSVC)
    add_compile_options(/MP)
endif()

# Check if we're compiling with Emscripten
if(EMSCRIPTEN)
    message(STATUS "Building with Emscripten")

    add_definitions(-DEMSCRIPTEN)

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
    ${PROJECT_SOURCE_DIR}/Core/Paths.h
    ${PROJECT_SOURCE_DIR}/Core/GameContext.h

    # Renderer (raylib)
    ${PROJECT_SOURCE_DIR}/Renderer/Renderer.cpp
    ${PROJECT_SOURCE_DIR}/Renderer/Renderer.h
    ${PROJECT_SOURCE_DIR}/Renderer/InputSystem.cpp
    ${PROJECT_SOURCE_DIR}/Renderer/InputSystem.h
    ${PROJECT_SOURCE_DIR}/Renderer/Panel.cpp
    ${PROJECT_SOURCE_DIR}/Renderer/Panel.h

    # Actor
    ${PROJECT_SOURCE_DIR}/Actor/Actor.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Actor.h
    ${PROJECT_SOURCE_DIR}/Actor/Attacker.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Attacker.h
    ${PROJECT_SOURCE_DIR}/Actor/MonsterAttacker.cpp
    ${PROJECT_SOURCE_DIR}/Actor/MonsterAttacker.h
    ${PROJECT_SOURCE_DIR}/Actor/PlayerAttacker.cpp
    ${PROJECT_SOURCE_DIR}/Actor/PlayerAttacker.h
    ${PROJECT_SOURCE_DIR}/Actor/Container.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Container.h
    ${PROJECT_SOURCE_DIR}/Actor/Pickable.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Pickable.h
    ${PROJECT_SOURCE_DIR}/Actor/InventoryOperations.cpp
    ${PROJECT_SOURCE_DIR}/Actor/InventoryOperations.h
    ${PROJECT_SOURCE_DIR}/Actor/InventoryData.h
    ${PROJECT_SOURCE_DIR}/Actor/Creature.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Creature.h
    ${PROJECT_SOURCE_DIR}/Actor/Item.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Item.h
    ${PROJECT_SOURCE_DIR}/Actor/NPC.h
    ${PROJECT_SOURCE_DIR}/Actor/Object.h
    ${PROJECT_SOURCE_DIR}/Actor/Stairs.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Stairs.h

    # Persistent
    ${PROJECT_SOURCE_DIR}/Persistent/Persistent.cpp
    ${PROJECT_SOURCE_DIR}/Persistent/Persistent.h

    # Ai - Updated paths
    ${PROJECT_SOURCE_DIR}/Ai/Ai.cpp
    ${PROJECT_SOURCE_DIR}/Ai/Ai.h
    ${PROJECT_SOURCE_DIR}/Ai/AiMonster.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiMonster.h
    ${PROJECT_SOURCE_DIR}/Ai/AiMonsterConfused.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiMonsterConfused.h
    ${PROJECT_SOURCE_DIR}/Ai/PlayerController.cpp
    ${PROJECT_SOURCE_DIR}/Ai/PlayerController.h
    ${PROJECT_SOURCE_DIR}/Ai/AiShopkeeper.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiShopkeeper.h
    ${PROJECT_SOURCE_DIR}/Ai/AiMimic.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiMimic.h
    ${PROJECT_SOURCE_DIR}/Ai/AiMonsterRanged.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiMonsterRanged.h
    ${PROJECT_SOURCE_DIR}/Ai/AiSpider.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiSpider.h
    ${PROJECT_SOURCE_DIR}/Ai/AiWebSpinner.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiWebSpinner.h

    # dnd_tables
    ${PROJECT_SOURCE_DIR}/dnd_tables/CombatProgressionTables.h

    # Random
    ${PROJECT_SOURCE_DIR}/Random/RandomDice.h

    # Attributes
    ${PROJECT_SOURCE_DIR}/Attributes/CharismaAttributes.h
    ${PROJECT_SOURCE_DIR}/Attributes/ConstitutionAttributes.h
    ${PROJECT_SOURCE_DIR}/Attributes/DexterityAttributes.h
    ${PROJECT_SOURCE_DIR}/Attributes/IntelligenceAttributes.h
    ${PROJECT_SOURCE_DIR}/Attributes/StrengthAttributes.h
    ${PROJECT_SOURCE_DIR}/Attributes/WisdomAttributes.h

    # Colors
    ${PROJECT_SOURCE_DIR}/Colors/Colors.cpp
    ${PROJECT_SOURCE_DIR}/Colors/Colors.h

    # Controls
    ${PROJECT_SOURCE_DIR}/Controls/Controls.h

    # Menu
    ${PROJECT_SOURCE_DIR}/Menu/BaseMenu.cpp
    ${PROJECT_SOURCE_DIR}/Menu/BaseMenu.h
    ${PROJECT_SOURCE_DIR}/Menu/MenuEntry.h
    ${PROJECT_SOURCE_DIR}/Menu/ListMenu.cpp
    ${PROJECT_SOURCE_DIR}/Menu/ListMenu.h
    ${PROJECT_SOURCE_DIR}/Menu/NotificationMenu.cpp
    ${PROJECT_SOURCE_DIR}/Menu/NotificationMenu.h
    ${PROJECT_SOURCE_DIR}/Menu/Menu.cpp
    ${PROJECT_SOURCE_DIR}/Menu/Menu.h
    ${PROJECT_SOURCE_DIR}/Menu/MenuClass.cpp
    ${PROJECT_SOURCE_DIR}/Menu/MenuClass.h
    ${PROJECT_SOURCE_DIR}/Menu/MenuGender.cpp
    ${PROJECT_SOURCE_DIR}/Menu/MenuGender.h
    ${PROJECT_SOURCE_DIR}/Menu/MenuName.cpp
    ${PROJECT_SOURCE_DIR}/Menu/MenuName.h
    ${PROJECT_SOURCE_DIR}/Menu/MenuRace.cpp
    ${PROJECT_SOURCE_DIR}/Menu/MenuRace.h
    ${PROJECT_SOURCE_DIR}/Menu/MenuBuy.cpp
    ${PROJECT_SOURCE_DIR}/Menu/MenuBuy.h
    ${PROJECT_SOURCE_DIR}/Menu/MenuSell.cpp
    ${PROJECT_SOURCE_DIR}/Menu/MenuSell.h
    ${PROJECT_SOURCE_DIR}/Menu/MenuTrade.cpp
    ${PROJECT_SOURCE_DIR}/Menu/MenuTrade.h
    ${PROJECT_SOURCE_DIR}/Menu/MenuSpellCast.cpp
    ${PROJECT_SOURCE_DIR}/Menu/MenuSpellCast.h
    ${PROJECT_SOURCE_DIR}/Menu/ContextMenu.cpp
    ${PROJECT_SOURCE_DIR}/Menu/ContextMenu.h
    ${PROJECT_SOURCE_DIR}/Menu/DeathMenu.cpp
    ${PROJECT_SOURCE_DIR}/Menu/DeathMenu.h

    # ActorTypes
    ${PROJECT_SOURCE_DIR}/ActorTypes/Monsters.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Monsters.h
    ${PROJECT_SOURCE_DIR}/ActorTypes/Player.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Player.h
    ${PROJECT_SOURCE_DIR}/ActorTypes/Monsters/Spider.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Monsters/Spider.h

    # Gui
    ${PROJECT_SOURCE_DIR}/Gui/Gui.cpp
    ${PROJECT_SOURCE_DIR}/Gui/Gui.h
    ${PROJECT_SOURCE_DIR}/Gui/LogMessage.cpp
    ${PROJECT_SOURCE_DIR}/Gui/LogMessage.h

    # Map
    ${PROJECT_SOURCE_DIR}/Map/Map.cpp
    ${PROJECT_SOURCE_DIR}/Map/Map.h
    ${PROJECT_SOURCE_DIR}/Map/DungeonRoom.h
    ${PROJECT_SOURCE_DIR}/Map/DungeonGenerator.cpp
    ${PROJECT_SOURCE_DIR}/Map/DungeonGenerator.h
    ${PROJECT_SOURCE_DIR}/Map/DungeonNames.cpp
    ${PROJECT_SOURCE_DIR}/Map/DungeonNames.h
    ${PROJECT_SOURCE_DIR}/Map/FovMap.cpp
    ${PROJECT_SOURCE_DIR}/Map/FovMap.h
    ${PROJECT_SOURCE_DIR}/Map/Minimap.cpp
    ${PROJECT_SOURCE_DIR}/Map/Minimap.h
    ${PROJECT_SOURCE_DIR}/Map/Decoration.h

    # Items - Updated paths
    ${PROJECT_SOURCE_DIR}/Items/ItemClassification.cpp
    ${PROJECT_SOURCE_DIR}/Items/ItemClassification.h
    ${PROJECT_SOURCE_DIR}/Items/Weapons.cpp
    ${PROJECT_SOURCE_DIR}/Items/Weapons.h
    ${PROJECT_SOURCE_DIR}/Items/MagicalItemEffects.cpp
    ${PROJECT_SOURCE_DIR}/Items/MagicalItemEffects.h

    # Systems - Updated paths
    ${PROJECT_SOURCE_DIR}/Systems/TargetingMenu.cpp
    ${PROJECT_SOURCE_DIR}/Systems/TargetingMenu.h
    ${PROJECT_SOURCE_DIR}/Systems/TargetingSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/TargetingSystem.h
    ${PROJECT_SOURCE_DIR}/Systems/SpellAnimations.cpp
    ${PROJECT_SOURCE_DIR}/Systems/SpellAnimations.h
    ${PROJECT_SOURCE_DIR}/Systems/TargetMode.h
    ${PROJECT_SOURCE_DIR}/Systems/EncounterPlanner.cpp
    ${PROJECT_SOURCE_DIR}/Systems/EncounterPlanner.h
    ${PROJECT_SOURCE_DIR}/Systems/HungerSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/HungerSystem.h
    ${PROJECT_SOURCE_DIR}/Systems/BuffSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/BuffSystem.h
    ${PROJECT_SOURCE_DIR}/Systems/CurseSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/CurseSystem.h
    ${PROJECT_SOURCE_DIR}/Systems/LevelUpSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/LevelUpSystem.h
    ${PROJECT_SOURCE_DIR}/Systems/MessageSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/MessageSystem.h
    ${PROJECT_SOURCE_DIR}/Systems/RenderingManager.cpp
    ${PROJECT_SOURCE_DIR}/Systems/RenderingManager.h
    ${PROJECT_SOURCE_DIR}/Systems/InputHandler.cpp
    ${PROJECT_SOURCE_DIR}/Systems/InputHandler.h
    ${PROJECT_SOURCE_DIR}/Systems/GameStateManager.cpp
    ${PROJECT_SOURCE_DIR}/Systems/GameStateManager.h
    ${PROJECT_SOURCE_DIR}/Systems/LevelManager.cpp
    ${PROJECT_SOURCE_DIR}/Systems/LevelManager.h
    ${PROJECT_SOURCE_DIR}/Systems/CreatureManager.cpp
    ${PROJECT_SOURCE_DIR}/Systems/CreatureManager.h
    ${PROJECT_SOURCE_DIR}/Systems/MenuManager.cpp
    ${PROJECT_SOURCE_DIR}/Systems/MenuManager.h
    ${PROJECT_SOURCE_DIR}/Systems/DisplayManager.cpp
    ${PROJECT_SOURCE_DIR}/Systems/DisplayManager.h
    ${PROJECT_SOURCE_DIR}/Systems/GameLoopCoordinator.cpp
    ${PROJECT_SOURCE_DIR}/Systems/GameLoopCoordinator.h
    ${PROJECT_SOURCE_DIR}/Systems/ObjectManager.cpp
    ${PROJECT_SOURCE_DIR}/Systems/ObjectManager.h
    ${PROJECT_SOURCE_DIR}/Systems/DataManager.cpp
    ${PROJECT_SOURCE_DIR}/Systems/DataManager.h
    ${PROJECT_SOURCE_DIR}/Systems/ShopKeeper.cpp
    ${PROJECT_SOURCE_DIR}/Systems/ShopKeeper.h
    ${PROJECT_SOURCE_DIR}/Systems/Shopkeepers/ShopkeeperFactory.cpp
    ${PROJECT_SOURCE_DIR}/Systems/Shopkeepers/ShopkeeperFactory.h
    ${PROJECT_SOURCE_DIR}/Systems/ItemEnhancements/ItemEnhancements.cpp
    ${PROJECT_SOURCE_DIR}/Systems/ItemEnhancements/ItemEnhancements.h
    ${PROJECT_SOURCE_DIR}/Systems/SpellSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/SpellSystem.h
    ${PROJECT_SOURCE_DIR}/Systems/FloatingTextSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/FloatingTextSystem.h
    ${PROJECT_SOURCE_DIR}/Systems/AnimationSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/AnimationSystem.h
    ${PROJECT_SOURCE_DIR}/Systems/ContentRegistry.cpp
    ${PROJECT_SOURCE_DIR}/Systems/ContentRegistry.h
    ${PROJECT_SOURCE_DIR}/Systems/ContentRegistryIO.cpp
    ${PROJECT_SOURCE_DIR}/Systems/ContentRegistryIO.h
    ${PROJECT_SOURCE_DIR}/Systems/TileConfig.cpp
    ${PROJECT_SOURCE_DIR}/Systems/TileConfig.h

    # Tools
    ${PROJECT_SOURCE_DIR}/Tools/BalanceViewer.cpp
    ${PROJECT_SOURCE_DIR}/Tools/BalanceViewer.h
    ${PROJECT_SOURCE_DIR}/Tools/ContentEditor.cpp
    ${PROJECT_SOURCE_DIR}/Tools/ContentEditor.h
    ${PROJECT_SOURCE_DIR}/Tools/DecorEditor.cpp
    ${PROJECT_SOURCE_DIR}/Tools/DecorEditor.h
    ${PROJECT_SOURCE_DIR}/Tools/ItemEditor.cpp
    ${PROJECT_SOURCE_DIR}/Tools/ItemEditor.h
    ${PROJECT_SOURCE_DIR}/Tools/MonsterEditor.cpp
    ${PROJECT_SOURCE_DIR}/Tools/MonsterEditor.h
    ${PROJECT_SOURCE_DIR}/Tools/PrefabLibrary.cpp
    ${PROJECT_SOURCE_DIR}/Tools/PrefabLibrary.h
    ${PROJECT_SOURCE_DIR}/Tools/RoomEditor.cpp
    ${PROJECT_SOURCE_DIR}/Tools/RoomEditor.h
    ${PROJECT_SOURCE_DIR}/Tools/SpellEditor.cpp
    ${PROJECT_SOURCE_DIR}/Tools/SpellEditor.h

    # Factories - Updated paths
    ${PROJECT_SOURCE_DIR}/Factories/MonsterFactory.cpp
    ${PROJECT_SOURCE_DIR}/Factories/MonsterFactory.h
    ${PROJECT_SOURCE_DIR}/Factories/MonsterCreator.cpp
    ${PROJECT_SOURCE_DIR}/Factories/MonsterCreator.h
    ${PROJECT_SOURCE_DIR}/Factories/ItemFactory.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemFactory.h
    ${PROJECT_SOURCE_DIR}/Factories/ItemCreator.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemCreator.h

    # Objects - Updated paths
    ${PROJECT_SOURCE_DIR}/Objects/Web.cpp
    ${PROJECT_SOURCE_DIR}/Objects/Web.h
    ${PROJECT_SOURCE_DIR}/Objects/Trap.cpp
    ${PROJECT_SOURCE_DIR}/Objects/Trap.h

    # Utils - Updated paths
    ${PROJECT_SOURCE_DIR}/Utils/Dijkstra.cpp
    ${PROJECT_SOURCE_DIR}/Utils/Dijkstra.h
    ${PROJECT_SOURCE_DIR}/Utils/Vector2D.h
    ${PROJECT_SOURCE_DIR}/Utils/UniqueId.cpp
    ${PROJECT_SOURCE_DIR}/Utils/UniqueId.h

    # Combat - Weapon damage system
    ${PROJECT_SOURCE_DIR}/Combat/DeathHandler.cpp
    ${PROJECT_SOURCE_DIR}/Combat/DeathHandler.h
    ${PROJECT_SOURCE_DIR}/Combat/WeaponDamageRegistry.cpp
    ${PROJECT_SOURCE_DIR}/Combat/WeaponDamageRegistry.h
    ${PROJECT_SOURCE_DIR}/Combat/DamageInfo.h
    ${PROJECT_SOURCE_DIR}/Combat/ArmorClass.cpp
    ${PROJECT_SOURCE_DIR}/Combat/ArmorClass.h
    ${PROJECT_SOURCE_DIR}/Combat/DamageResolver.cpp
    ${PROJECT_SOURCE_DIR}/Combat/DamageResolver.h
    ${PROJECT_SOURCE_DIR}/Combat/ConstitutionTracker.cpp
    ${PROJECT_SOURCE_DIR}/Combat/ConstitutionTracker.h
    ${PROJECT_SOURCE_DIR}/Combat/HealthPool.cpp
    ${PROJECT_SOURCE_DIR}/Combat/HealthPool.h
    ${PROJECT_SOURCE_DIR}/Combat/CombatStats.cpp
    ${PROJECT_SOURCE_DIR}/Combat/CombatStats.h
    ${PROJECT_SOURCE_DIR}/Combat/ExperienceReward.cpp
    ${PROJECT_SOURCE_DIR}/Combat/ExperienceReward.h
    ${PROJECT_SOURCE_DIR}/Combat/CorpseData.cpp
    ${PROJECT_SOURCE_DIR}/Combat/CorpseData.h

    # UI - New UI classes
    ${PROJECT_SOURCE_DIR}/UI/InventoryUI.cpp
    ${PROJECT_SOURCE_DIR}/UI/InventoryUI.h
    ${PROJECT_SOURCE_DIR}/UI/LevelUpUI.cpp
    ${PROJECT_SOURCE_DIR}/UI/LevelUpUI.h
    ${PROJECT_SOURCE_DIR}/UI/CharacterSheetUI.cpp
    ${PROJECT_SOURCE_DIR}/UI/CharacterSheetUI.h

    # Main
    ${PROJECT_SOURCE_DIR}/main.cpp
)

# Create an executable with the listed source files
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

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
        target_compile_options(${PROJECT_NAME} PRIVATE /MP /utf-8)

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
