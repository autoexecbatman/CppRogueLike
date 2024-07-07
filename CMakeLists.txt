cmake_minimum_required (VERSION 3.13...3.21)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "${ENV:VCPKG_ROOT}/scripts/cmake" # copy your vcpkg path on your machine
        CACHE STRING "Vcpkg toolchain file")
endif()

project(
    C++RogueLike # Project name, change this as needed.
    LANGUAGES C CXX
)

# Ensure the C++20 standard is available.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

# Set source files
set(SOURCE_FILES # Add Source Files Here !

    # Actor
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Actor.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Actor.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Attacker.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Attacker.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Confuser.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Confuser.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Container.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Container.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Destructible.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Destructible.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Pickable.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor/Pickable.h

    # Persistent
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Persistent/Persistent.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Persistent/Persistent.h

    # Ai
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai/Ai.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai/Ai.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai/AiMonster.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai/AiMonster.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai/AiMonsterConfused.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai/AiMonsterConfused.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai/AiPlayer.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai/AiPlayer.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai/AiShopkeeper.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai/AiShopkeeper.h

    # dnd_tables
    ${PROJECT_SOURCE_DIR}/C++RogueLike/dnd_tables/CalculatedTHAC0s.h

    # Random
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Random/RandomDice.h

    # Attributes
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/CharismaAttributes.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/CharismaAttributes.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/ConstitutionAttributes.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/ConstitutionAttributes.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/DexterityAttributes.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/DexterityAttributes.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/IntelligenceAttributes.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/IntelligenceAttributes.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/StrengthAttributes.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/StrengthAttributes.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/WisdomAttributes.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attributes/WisdomAttributes.h

    # Colors
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Colors/Colors.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Colors/Colors.h

    # Controls
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Controls/Controls.h

    # Menu
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu/Menu.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu/Menu.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu/MenuClass.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu/MenuClass.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu/MenuGender.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu/MenuGender.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu/MenuName.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu/MenuName.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu/MenuRace.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu/MenuRace.h

    # ActorTypes
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Dagger.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Dagger.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Fireball.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Fireball.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Monsters.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Monsters.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Gold.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Gold.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Healer.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Healer.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/LightningBolt.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/LightningBolt.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/LongSword.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/LongSword.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Player.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ActorTypes/Player.h

    # Gui
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Gui/Gui.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Gui/Gui.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Gui/Window.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Gui/Window.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Gui/LogMessage.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Gui/LogMessage.h

    # Main
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Game.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Game.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/main_C++RogueLike.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Items.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Items.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/BaseMenu.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/BaseMenu.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/IMenuState.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuTrade.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuTrade.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuBuy.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuBuy.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuSell.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuSell.cpp

    # Map
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Map/Map.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Map/Map.h


    ${PROJECT_SOURCE_DIR}/C++RogueLike/Weapons.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Weapons.h
)

# Create an executable with the listed source files
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Enforce UTF-8 encoding on MSVC.
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# Get from Vcpkg.
find_package(unofficial-pdcurses CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(libtcod CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        unofficial::pdcurses::pdcurses
        SDL2::SDL2
        SDL2::SDL2main
        libtcod::libtcod
        nlohmann_json nlohmann_json::nlohmann_json
)

# Copy JSON files to build directory.
set(JSON_FILES
    "${PROJECT_SOURCE_DIR}/C++RogueLike/json/strength.json"
    "${PROJECT_SOURCE_DIR}/C++RogueLike/json/dexterity.json"
    "${PROJECT_SOURCE_DIR}/C++RogueLike/json/constitution.json"
    "${PROJECT_SOURCE_DIR}/C++RogueLike/json/wisdom.json"
    "${PROJECT_SOURCE_DIR}/C++RogueLike/json/intelligence.json"
    "${PROJECT_SOURCE_DIR}/C++RogueLike/json/charisma.json"
    "${PROJECT_SOURCE_DIR}/C++RogueLike/json/weapons.json"
)

foreach(JSON_FILE ${JSON_FILES})
    get_filename_component(JSON_FILENAME ${JSON_FILE} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD 
                       COMMAND ${CMAKE_COMMAND} -E echo "Copying ${JSON_FILE} to build directory"
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                               ${JSON_FILE}
                               $<TARGET_FILE_DIR:${PROJECT_NAME}>/${JSON_FILENAME})
endforeach()