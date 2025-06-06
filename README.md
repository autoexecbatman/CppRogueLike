🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉

*This is a home for a mysterious roguelike...*

*The vision for this project is to have a fun fantasy adnd2e RL game.*

*Putting it simply Rouge with adnd2e mechanics and some inpiration from classics like ADOM, BG2 and Rogue.*

## :construction: *** UNDER CONSTRUCTION! *** :construction:

🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉⚔️🐉

## Running The Game From RAR
   - Clone the repository
   - Extract the folder in **cpprl pre-release-x64.rar**
   - Run the executable from the folder !

# Contributing to CppRogueLike

#### OPEN TO CONTRIBUTION !!! 🤗
Thank you for considering a contribution to CppRogueLike.

## Running Using Visual Studio 2022

1. **Install Visual Studio 2022**: Download and install for free from [VisualStudio2022](https://visualstudio.microsoft.com/downloads/).

2. **Install vcpkg**:


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

3. **Install Dependencies Using vcpkg**:
    - For x86 architecture:
      ```
      .\vcpkg install pdcurses:x86-windows libtcod:x86-windows nlohmann-json:x86-windows fmt:x86-windows
      ```
    - For x64 architecture:
      ```
      .\vcpkg install pdcurses:x64-windows libtcod:x64-windows nlohmann-json:x64-windows fmt:x64-windows
      ```

   **Note**: Ensure the names of the packages are correct in vcpkg. Some package names might differ slightly from their GitHub repository names.

4. **Clone the repository**. Either using `Visual Studio 2022` or using the command line.

5. **Open C++RogueLike.sln in Visual Studio**: Ensure vcpkg's toolchain file is used for CMake. This can be set in CMake settings in Visual Studio.

6. **Build the project** ensuring it's set to use C++20.

## Required Dependencies

- [PDCurses (pdcurses)](https://github.com/wmcbrine/PDCurses)
- [libtcod (libtcod)](https://github.com/libtcod/libtcod)
- [nlohmann/json (nlohmann-json)](https://github.com/nlohmann/json)

## Development Environment

I primarily use **Visual Studio 2022** for development. However, if you're comfortable with other environments like VS Code, feel free to use them. If you need assistance with setting up, reach out, and i'll help.

## Code Standards

Ensure your contributions adhere to the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines).

## Additional Resources
- [ad&d 2e book](https://archive.org/details/advanced-dungeons-dragons-2nd-edition)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Git and GitHub Handbook](https://guides.github.com/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
