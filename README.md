# C++ RogueLike

A fantasy roguelike dungeon crawler with classic old-school RPG mechanics.

Inspired by classics: Rogue, ADOM, Baldur's Gate 2.

Play in browser: https://autoexecbatman.github.io/CppRogueLike/C++RogueLike.html

---

## Features

- Classic d20 combat, spells, and character progression
- Raylib renderer with Dawnlike tileset
- JSON-driven content pipeline — monsters, items, spells, and tiles are all data
- In-game editors for monsters, items, spells, and map decoration
- Emscripten web build

## Dependencies

| Library | Purpose |
|---|---|
| [Raylib](https://github.com/raysan5/raylib) | Rendering and input |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON data pipeline |

## Build — Windows / Visual Studio 2022

### Prerequisites

- Visual Studio 2022 with C++ workload
- CMake 3.20+
- [vcpkg](https://github.com/microsoft/vcpkg)

### Setup vcpkg

```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

### Configure and build

```
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --config Release
```

Open `C++RogueLike.sln` in Visual Studio to develop interactively.

## Build — Web (Emscripten)

```
mkdir build-web
cd build-web
emcmake cmake ..
cmake --build .
```

## Resources

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)

## Contributing

Open to contributions. See the build instructions above to get started.
