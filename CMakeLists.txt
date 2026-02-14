cmake_minimum_required(VERSION 3.13...3.21)

# Project definition
project(
    C++RogueLike
    LANGUAGES C CXX
)

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

    # Fetch raylib for Emscripten build
    include(FetchContent)
    set(PLATFORM Web CACHE INTERNAL "")
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
    )
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
    set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)

    # Emscripten flags: ASYNCIFY enables blocking menu loops to yield to browser
    set(EMCC_FLAGS "-sWASM=1 -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=['cwrap'] -sASSERTIONS=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 -sEXPORTED_FUNCTIONS=['_main','_malloc'] -sNO_DISABLE_EXCEPTION_CATCHING -sUSE_GLFW=3 -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sGL_ENABLE_GET_PROC_ADDRESS=1")

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
    ${PROJECT_SOURCE_DIR}/Game.h

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
    ${PROJECT_SOURCE_DIR}/Actor/Container.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Container.h
    ${PROJECT_SOURCE_DIR}/Actor/Destructible.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Destructible.h
    ${PROJECT_SOURCE_DIR}/Actor/Pickable.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Pickable.h
    ${PROJECT_SOURCE_DIR}/Actor/InventoryOperations.cpp
    ${PROJECT_SOURCE_DIR}/Actor/InventoryOperations.h
    ${PROJECT_SOURCE_DIR}/Actor/InventoryData.h

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
    ${PROJECT_SOURCE_DIR}/Ai/AiPlayer.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiPlayer.h
    ${PROJECT_SOURCE_DIR}/Ai/AiShopkeeper.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiShopkeeper.h
    ${PROJECT_SOURCE_DIR}/Ai/AiMimic.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiMimic.h
    ${PROJECT_SOURCE_DIR}/Ai/AiMonsterRanged.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiMonsterRanged.h
    ${PROJECT_SOURCE_DIR}/Ai/AiSpider.cpp
    ${PROJECT_SOURCE_DIR}/Ai/AiSpider.h

    # dnd_tables
    ${PROJECT_SOURCE_DIR}/dnd_tables/CalculatedTHAC0s.h

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
    ${PROJECT_SOURCE_DIR}/Menu/IMenuState.h
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

    # ActorTypes
    ${PROJECT_SOURCE_DIR}/ActorTypes/Monsters.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Monsters.h
    ${PROJECT_SOURCE_DIR}/ActorTypes/Gold.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Gold.h
    ${PROJECT_SOURCE_DIR}/ActorTypes/Healer.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Healer.h
    ${PROJECT_SOURCE_DIR}/ActorTypes/Teleporter.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Teleporter.h
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

    # Items - Updated paths
    ${PROJECT_SOURCE_DIR}/Items/Items.cpp
    ${PROJECT_SOURCE_DIR}/Items/Items.h
    ${PROJECT_SOURCE_DIR}/Items/ItemClassification.cpp
    ${PROJECT_SOURCE_DIR}/Items/ItemClassification.h
    ${PROJECT_SOURCE_DIR}/Items/Weapons.cpp
    ${PROJECT_SOURCE_DIR}/Items/Weapons.h
    ${PROJECT_SOURCE_DIR}/Items/Armor.cpp
    ${PROJECT_SOURCE_DIR}/Items/Armor.h
    ${PROJECT_SOURCE_DIR}/Items/Jewelry.cpp
    ${PROJECT_SOURCE_DIR}/Items/Jewelry.h
    ${PROJECT_SOURCE_DIR}/Items/MagicalItemEffects.cpp
    ${PROJECT_SOURCE_DIR}/Items/MagicalItemEffects.h
    ${PROJECT_SOURCE_DIR}/Items/Food.cpp
    ${PROJECT_SOURCE_DIR}/Items/Food.h
    ${PROJECT_SOURCE_DIR}/Items/CorpseFood.cpp
    ${PROJECT_SOURCE_DIR}/Items/CorpseFood.h
    ${PROJECT_SOURCE_DIR}/Items/Amulet.cpp
    ${PROJECT_SOURCE_DIR}/Items/Amulet.h

    # Systems - Updated paths
    ${PROJECT_SOURCE_DIR}/Systems/TargetingSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/TargetingSystem.h
    ${PROJECT_SOURCE_DIR}/Systems/SpellAnimations.cpp
    ${PROJECT_SOURCE_DIR}/Systems/SpellAnimations.h
    ${PROJECT_SOURCE_DIR}/Systems/TargetMode.h
    ${PROJECT_SOURCE_DIR}/Systems/HungerSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/HungerSystem.h
    ${PROJECT_SOURCE_DIR}/Systems/BuffSystem.cpp
    ${PROJECT_SOURCE_DIR}/Systems/BuffSystem.h
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

    # Factories - Updated paths
    ${PROJECT_SOURCE_DIR}/Factories/MonsterFactory.cpp
    ${PROJECT_SOURCE_DIR}/Factories/MonsterFactory.h
    ${PROJECT_SOURCE_DIR}/Factories/MonsterCreator.cpp
    ${PROJECT_SOURCE_DIR}/Factories/MonsterCreator.h
    ${PROJECT_SOURCE_DIR}/Factories/ItemFactory.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemFactory.h
    ${PROJECT_SOURCE_DIR}/Factories/ItemCreator.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemCreator.h
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/PotionItems.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/ScrollItems.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/MeleeWeaponItems.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/RangedWeaponItems.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/ArmorItems.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/HelmItems.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/RingItems.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/JewelryItems.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/GirdleItems.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/FoodItems.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/EnhancedWeaponRules.cpp
    ${PROJECT_SOURCE_DIR}/Factories/ItemRegistries/EnhancedArmorRules.cpp

    # Objects - Updated paths
    ${PROJECT_SOURCE_DIR}/Objects/Web.cpp
    ${PROJECT_SOURCE_DIR}/Objects/Web.h

    # Utils - Updated paths
    ${PROJECT_SOURCE_DIR}/Utils/Dijkstra.cpp
    ${PROJECT_SOURCE_DIR}/Utils/Dijkstra.h
    ${PROJECT_SOURCE_DIR}/Utils/Vector2D.h
    ${PROJECT_SOURCE_DIR}/Utils/UniqueId.cpp
    ${PROJECT_SOURCE_DIR}/Utils/UniqueId.h

    # Combat - Weapon damage system
    ${PROJECT_SOURCE_DIR}/Combat/WeaponDamageRegistry.cpp
    ${PROJECT_SOURCE_DIR}/Combat/WeaponDamageRegistry.h
    ${PROJECT_SOURCE_DIR}/Combat/DamageInfo.h

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
    # Check if we need to download nlohmann_json
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/include/nlohmann/json.hpp")
        message(STATUS "Downloading nlohmann/json...")
        file(DOWNLOAD
            "https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp"
            "${CMAKE_BINARY_DIR}/include/nlohmann/json.hpp"
            SHOW_PROGRESS
        )
    endif()

    # Check if libtcod.h exists directly in the include directory
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/libtcod/libtcod.h")
        message(FATAL_ERROR "libtcod.h not found. Please make sure it exists at ${CMAKE_SOURCE_DIR}/libtcod/libtcod.h")
    endif()

    # Include directories for all dependencies
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/libtcod/
    )

    file(GLOB LIBTCOD_SRC
    "${CMAKE_SOURCE_DIR}/libtcod/*.h"
    "${CMAKE_SOURCE_DIR}/libtcod/*.hpp"
    "${CMAKE_SOURCE_DIR}/libtcod/*/*.h"
    "${CMAKE_SOURCE_DIR}/libtcod/*/*.hpp"
    )

    set(LIBTCOD_LIBRARIES "${CMAKE_SOURCE_DIR}/libtcod/lib/libtcod.a;")

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${LIBTCOD_LIBRARIES}
        raylib
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        TCOD_SDL2
        TCOD_EMSCRIPTEN
    )

else()
    # For native build, link against required libraries
    find_package(raylib CONFIG REQUIRED)
    find_package(libtcod CONFIG REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)

    target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
            raylib
            libtcod::libtcod
            nlohmann_json::nlohmann_json
    )

    # Enforce UTF-8 encoding on MSVC
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /MP /utf-8)

        # Set the working directory for Visual Studio debugging
        set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
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

    foreach(JSON_FILE ${JSON_FILES})
        get_filename_component(JSON_FILENAME ${JSON_FILE} NAME)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                          COMMAND ${CMAKE_COMMAND} -E echo "Copying ${JSON_FILE} to build directory"
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
