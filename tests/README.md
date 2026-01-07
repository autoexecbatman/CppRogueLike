# C++ RogueLike Unit Tests

This directory contains unit tests for the C++ RogueLike project using Google Test framework.

## Prerequisites

- CMake 3.13 or higher
- C++20 compatible compiler (MSVC, GCC, Clang)
- vcpkg (for dependency management)
- Google Test (installed via vcpkg)

## Building Tests

### Using CMake

From the project root directory:

```bash
# Configure with tests enabled
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# Build the tests
cmake --build build --config Release --target run_tests
```

### Using vcpkg

Make sure you have vcpkg installed and these packages:

```bash
vcpkg install gtest:x64-windows
vcpkg install pdcurses:x64-windows
vcpkg install sdl3:x64-windows
vcpkg install libtcod:x64-windows
vcpkg install nlohmann-json:x64-windows
```

## Running Tests

After building, run the tests:

```bash
# Windows
cd build/tests/Release
./run_tests.exe

# Linux/Mac
cd build/tests
./run_tests
```

Or use CMake's test runner:

```bash
cd build
ctest --output-on-failure
```

## Test Structure

Tests are organized to mirror the source code structure:

```
tests/
├── Factories/
│   └── ItemCreatorTest.cpp
├── Items/
│   └── (item tests)
├── Utils/
│   ├── UniqueIdTest.cpp
│   └── Vector2DTest.cpp
└── CMakeLists.txt
```

## Writing New Tests

1. Create a new test file in the appropriate subdirectory
2. Include Google Test: `#include <gtest/gtest.h>`
3. Write test cases using TEST() or TEST_F() macros
4. CMake will automatically discover and register new tests

Example:

```cpp
#include <gtest/gtest.h>
#include "src/YourClass.h"

class YourClassTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }
};

TEST_F(YourClassTest, TestSomething) {
    // Arrange
    YourClass obj;

    // Act
    int result = obj.doSomething();

    // Assert
    EXPECT_EQ(result, expected_value);
}
```

## CI/CD

Tests run automatically on GitHub Actions for every commit to master, main, or develop branches.

See `.github/workflows/ci-tests.yml` for the CI configuration.

## Using Local AI for Test Generation

This project uses a local AI model (deepseek-coder-v2) to help generate tests. The AI assists in:

- Converting existing assert-based tests to Google Test format
- Generating new test cases
- Suggesting edge cases

To use the local AI for test generation, ensure Ollama is running with the deepseek-coder-v2:16b-lite-instruct-q4_K_M model.
