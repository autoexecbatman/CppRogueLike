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

# Set the source directory
set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")

# Set source files
set(SOURCE_FILES # Add Source Files Here !

    # Actor
    ${PROJECT_SOURCE_DIR}/Actor/Actor.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Actor.h
    ${PROJECT_SOURCE_DIR}/Actor/Attacker.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Attacker.h
    ${PROJECT_SOURCE_DIR}/Actor/Confuser.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Confuser.h
    ${PROJECT_SOURCE_DIR}/Actor/Container.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Container.h
    ${PROJECT_SOURCE_DIR}/Actor/Destructible.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Destructible.h
    ${PROJECT_SOURCE_DIR}/Actor/Pickable.cpp
    ${PROJECT_SOURCE_DIR}/Actor/Pickable.h

    # Persistent
    ${PROJECT_SOURCE_DIR}/Persistent/Persistent.cpp
    ${PROJECT_SOURCE_DIR}/Persistent/Persistent.h

    # Ai
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

    # dnd_tables
    ${PROJECT_SOURCE_DIR}/dnd_tables/CalculatedTHAC0s.h

    # Random
    ${PROJECT_SOURCE_DIR}/Random/RandomDice.h

    # Attributes
    ${PROJECT_SOURCE_DIR}/Attributes/CharismaAttributes.cpp
    ${PROJECT_SOURCE_DIR}/Attributes/CharismaAttributes.h
    ${PROJECT_SOURCE_DIR}/Attributes/ConstitutionAttributes.cpp
    ${PROJECT_SOURCE_DIR}/Attributes/ConstitutionAttributes.h
    ${PROJECT_SOURCE_DIR}/Attributes/DexterityAttributes.cpp
    ${PROJECT_SOURCE_DIR}/Attributes/DexterityAttributes.h
    ${PROJECT_SOURCE_DIR}/Attributes/IntelligenceAttributes.cpp
    ${PROJECT_SOURCE_DIR}/Attributes/IntelligenceAttributes.h
    ${PROJECT_SOURCE_DIR}/Attributes/StrengthAttributes.cpp
    ${PROJECT_SOURCE_DIR}/Attributes/StrengthAttributes.h
    ${PROJECT_SOURCE_DIR}/Attributes/WisdomAttributes.cpp
    ${PROJECT_SOURCE_DIR}/Attributes/WisdomAttributes.h

    # Colors
    ${PROJECT_SOURCE_DIR}/Colors/Colors.cpp
    ${PROJECT_SOURCE_DIR}/Colors/Colors.h

    # Controls
    ${PROJECT_SOURCE_DIR}/Controls/Controls.h

    # Menu
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

    # ActorTypes
    ${PROJECT_SOURCE_DIR}/ActorTypes/Fireball.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Fireball.h
    ${PROJECT_SOURCE_DIR}/ActorTypes/Monsters.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Monsters.h
    ${PROJECT_SOURCE_DIR}/ActorTypes/Gold.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Gold.h
    ${PROJECT_SOURCE_DIR}/ActorTypes/Healer.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Healer.h
    ${PROJECT_SOURCE_DIR}/ActorTypes/LightningBolt.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/LightningBolt.h
    ${PROJECT_SOURCE_DIR}/ActorTypes/Player.cpp
    ${PROJECT_SOURCE_DIR}/ActorTypes/Player.h

    # Gui
    ${PROJECT_SOURCE_DIR}/Gui/Gui.cpp
    ${PROJECT_SOURCE_DIR}/Gui/Gui.h
    ${PROJECT_SOURCE_DIR}/Gui/Window.cpp
    ${PROJECT_SOURCE_DIR}/Gui/Window.h
    ${PROJECT_SOURCE_DIR}/Gui/LogMessage.cpp
    ${PROJECT_SOURCE_DIR}/Gui/LogMessage.h

    # Main
    ${PROJECT_SOURCE_DIR}/Game.cpp
    ${PROJECT_SOURCE_DIR}/Game.h
    ${PROJECT_SOURCE_DIR}/main_C++RogueLike.cpp
    ${PROJECT_SOURCE_DIR}/Items.h
    ${PROJECT_SOURCE_DIR}/Items.cpp
    ${PROJECT_SOURCE_DIR}/BaseMenu.h
    ${PROJECT_SOURCE_DIR}/BaseMenu.cpp
    ${PROJECT_SOURCE_DIR}/IMenuState.h
    ${PROJECT_SOURCE_DIR}/MenuTrade.h
    ${PROJECT_SOURCE_DIR}/MenuTrade.cpp
    ${PROJECT_SOURCE_DIR}/MenuBuy.h
    ${PROJECT_SOURCE_DIR}/MenuBuy.cpp
    ${PROJECT_SOURCE_DIR}/MenuSell.h
    ${PROJECT_SOURCE_DIR}/MenuSell.cpp
    ${PROJECT_SOURCE_DIR}/Dijkstra.h
    ${PROJECT_SOURCE_DIR}/Dijkstra.cpp

    # Map
    ${PROJECT_SOURCE_DIR}/Map/Map.cpp
    ${PROJECT_SOURCE_DIR}/Map/Map.h


    ${PROJECT_SOURCE_DIR}/Weapons.cpp
    ${PROJECT_SOURCE_DIR}/Weapons.h
)

# Create an executable with the listed source files
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Enforce UTF-8 encoding on MSVC.
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /MP /utf-8)
    set_target_properties(${PROJECT_NAME} PROPERTIES # Set the working directory for Visual Studio debugging.
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug"
    )
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
        nlohmann_json::nlohmann_json
)

# Copy JSON files to build directory.
set(JSON_FILES
    "${PROJECT_SOURCE_DIR}/json/strength.json"
    "${PROJECT_SOURCE_DIR}/json/dexterity.json"
    "${PROJECT_SOURCE_DIR}/json/constitution.json"
    "${PROJECT_SOURCE_DIR}/json/wisdom.json"
    "${PROJECT_SOURCE_DIR}/json/intelligence.json"
    "${PROJECT_SOURCE_DIR}/json/charisma.json"
    "${PROJECT_SOURCE_DIR}/json/weapons.json"
)

foreach(JSON_FILE ${JSON_FILES})
    get_filename_component(JSON_FILENAME ${JSON_FILE} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD 
                       COMMAND ${CMAKE_COMMAND} -E echo "Copying ${JSON_FILE} to build directory"
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                               ${JSON_FILE}
                               $<TARGET_FILE_DIR:${PROJECT_NAME}>/${JSON_FILENAME})
endforeach()