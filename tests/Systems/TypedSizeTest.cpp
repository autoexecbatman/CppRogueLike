// file: TypedSizeTest.cpp
//
// parse_typed_size's contract: the room editor's width and height prompts read a whole
// number, and any text that is not one - empty, trailing or leading junk, too large for
// an int - reads as nothing, so the prompt keeps the current size instead of taking a
// prefix or throwing. The prompt admits only digits today; these cases are what the
// parser itself promises if that guard ever widens.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=TypedSizeTest.*

#include <gtest/gtest.h>

#include <optional>

#include "src/RoomEditor.h"

// A whole number reads as itself, including the bounds the prompts clamp to.
TEST(TypedSizeTest, AWholeNumberReadsAsItself)
{
	EXPECT_EQ(parse_typed_size("12"), std::optional<int>{ 12 });
	EXPECT_EQ(parse_typed_size("3"), std::optional<int>{ 3 });
	EXPECT_EQ(parse_typed_size("060"), std::optional<int>{ 60 });
}

// Nothing typed is not a size, so the prompt keeps the one it has.
TEST(TypedSizeTest, NothingTypedReadsAsNothing)
{
	EXPECT_EQ(parse_typed_size(""), std::nullopt);
}

// A number with anything after or before it is refused whole, never read as its prefix.
TEST(TypedSizeTest, APartialNumberReadsAsNothing)
{
	EXPECT_EQ(parse_typed_size("12x"), std::nullopt);
	EXPECT_EQ(parse_typed_size("x12"), std::nullopt);
	EXPECT_EQ(parse_typed_size("1 2"), std::nullopt);
}

// A number too large for an int is refused rather than wrapped or thrown.
TEST(TypedSizeTest, ANumberPastAnIntReadsAsNothing)
{
	EXPECT_EQ(parse_typed_size("99999999999"), std::nullopt);
}
