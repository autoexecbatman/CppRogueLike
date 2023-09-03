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
    - Integrate vcpkg system-wide **Use this after finishing installation** (optional but recommended):
      ```
      .\vcpkg integrate install
      ```

3. **Install Dependencies Using vcpkg**:
    - For x86 architecture:
      ```
      .\vcpkg install ms-gsl:x86-windows pdcurses:x86-windows libtcod:x86-windows nlohmann-json:x86-windows fmt:x86-windows
      ```
    - For x64 architecture:
      ```
      .\vcpkg install ms-gsl:x64-windows pdcurses:x64-windows libtcod:x64-windows nlohmann-json:x64-windows fmt:x64-windows
      ```

   **Note**: Ensure the names of the packages are correct in vcpkg. Some package names might differ slightly from their GitHub repository names.

4. **Clone the repository**. Either using `Visual Studio 2022` or using the command line.

5. **Open C++RogueLike.sln in Visual Studio**: Ensure vcpkg's toolchain file is used for CMake. This can be set in CMake settings in Visual Studio.

6. **Build the project** ensuring it's set to use C++20.

## Required Dependencies

- [microsoft/GSL (ms-gsl)](https://github.com/microsoft/GSL/tree/main)
- [PDCurses (pdcurses)](https://github.com/wmcbrine/PDCurses)
- [libtcod (libtcod)](https://github.com/libtcod/libtcod)
- [nlohmann/json (nlohmann-json)](https://github.com/nlohmann/json)
- [fmtlib/fmt (fmt)](https://github.com/fmtlib/fmt)

**Note IMPORTANT**: Will not build currently due to missing additional ChatGPT API related headers. Please comment out ChatGPT API related `#include` in `Game.h` and a few lines in `main()`. This issue will be addressed soon.


# Contributing to CppRogueLike

Thank you for considering a contribution to CppRogueLike.

## Development Environment

I primarily use **Visual Studio 2022** for development. However, if you're comfortable with other environments like VS Code, feel free to use them. If you need assistance with setting up, reach out, and i'll help.

## Code Standards

Ensure your contributions adhere to the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines).

## Additional Resources
- [ad&d 2e book](https://archive.org/details/advanced-dungeons-dragons-2nd-edition)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Git and GitHub Handbook](https://guides.github.com/)
