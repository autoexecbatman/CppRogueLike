🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉

*This is a home for a mysterious roguelike...*

*The vision for this project is to have a fun fantasy adnd2e RL game.*

*Putting it simply Rouge with adnd2e mechanics and some inpiration from classics like ADOM, BG2 and Rogue.*

## :construction: *** UNDER CONSTRUCTION! *** :construction:
🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉

**Clone the repository**: Use Git to clone the repository or download it as a ZIP file.

## CMake Build Instructions With Visual Studio 2022
Build the project using CMake, follow these steps:
1. **Open a terminal** and navigate to the cloned repository directory.
2. **Create a build directory and navigate to it**:
   ```
   mkdir build
   cd build
   ```
3. **Run CMake** to configure the project please check Visual Studio docs for your version:
   ```
   cmake -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake ..
   ```
   Replace `[path_to_vcpkg]` with the actual path to your vcpkg installation.

4. **Build the project**:
   ```
   cmake --build . --config Release
   ```
5. **Navigate to the build directory** and open the generated solution file (`C++RogueLike.sln`) in Visual Studio.

## Required Dependencies
- [PDCurses (pdcurses)](https://github.com/wmcbrine/PDCurses)
- [libtcod (libtcod)](https://github.com/libtcod/libtcod)
- [nlohmann/json (nlohmann-json)](https://github.com/nlohmann/json)

## Setting Up Your Environment
1. **Install Visual Studio 2022**: Download and install for free from [VisualStudio2022](https://visualstudio.microsoft.com/downloads/).
2. **Install CMake**: Download and install CMake from [CMake.org](https://cmake.org/download/).
3. **Install vcpkg**:

    - official vcpkg github [vcpkg github page](https://github.com/microsoft/vcpkg)

    - Clone Microsoft's vcpkg:
      ```
      git clone https://github.com/microsoft/vcpkg.git
      ```
    - Navigate to the cloned directory:
      ```
      cd vcpkg
      ```
    - Bootstrap the vcpkg:
      ```
      .\bootstrap-vcpkg.bat
      ```
    - Integrate vcpkg system-wide:
      ```
      .\vcpkg integrate install
      ```

3. **Manually Install Dependencies Using vcpkg**:
    - For x86 architecture:
      ```
      .\vcpkg install pdcurses:x86-windows libtcod:x86-windows nlohmann-json:x86-windows
      ```
    - For x64 architecture:
      ```
      .\vcpkg install pdcurses:x64-windows libtcod:x64-windows nlohmann-json:x64-windows
      ```

6. **Building the project** Build ensuring it's set to use C++20.

## Additional Resources
- [ad&d 2e book](https://archive.org/details/advanced-dungeons-dragons-2nd-edition)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Git and GitHub Handbook](https://guides.github.com/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

# Contributing to CppRogueLike

#### OPEN TO CONTRIBUTION !!! 🤗
Thank you for considering a contribution to CppRogueLike.