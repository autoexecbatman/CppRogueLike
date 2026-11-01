// file: EditorLayoutTest.cpp
// The editors' list panel sizes itself from what it holds: a pad, the tile
// column, the widest label, the tag column and a pad, at most half the
// screen. A label wider than the room left is cut to a prefix and "...", so
// no name runs under a row's tag. Every expected value is the rule's
// arithmetic worked by hand, measuring 8 pixels a character.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=EditorLayoutTest.*

#include <gtest/gtest.h>

#include <format>
#include <functional>
#include <string>
#include <string_view>

#include <raylib.h>

#include "src/EditorInput.h"
#include "src/ListCursor.h"
#include "src/Paths.h"
#include "src/RecordEditor.h"
#include "tests/mocks/HiddenRenderer.h"

namespace CppRL
{

	namespace
	{
		// A stand-in font: every character is 8 pixels wide.
		int eight_per_character(std::string_view text)
		{
			return static_cast<int>(text.size()) * 8;
		}

		const std::function<int(std::string_view)> MEASURE{
			&eight_per_character
		};

		// Reads a frame's input from a hidden window and checks it measures
		// text in the renderer's font.
		//
		// Example:
		//   exit_code_in_child(&the_real_input_measures_in_the_game_font);
		//   // -> 44, or 43 naming the check that failed
		[[noreturn]] void the_real_input_measures_in_the_game_font()
		{
			HiddenRenderer hidden{};
			const EditorInput input = read_editor_input(hidden.renderer);
			exit_unless(
				static_cast<bool>(input.measure),
				"read_editor_input left measure unset");
			exit_unless(
				input.measure("Protection From Evil") ==
					hidden.renderer.measure_text("Protection From Evil"),
				"measure is not the renderer's measure_text");
			finish_the_child();
		}

		// Reads a frame's input from a hidden window and checks it carries the
		// renderer's font size as its line height.
		//
		// Example:
		//   exit_code_in_child(&the_real_input_carries_the_font_size);
		//   // -> 44, or 43 when the line height is not the font size
		[[noreturn]] void the_real_input_carries_the_font_size()
		{
			HiddenRenderer hidden{};
			const EditorInput input = read_editor_input(hidden.renderer);
			exit_unless(
				input.lineHeight == hidden.renderer.get_font_size(),
				std::format(
					"lineHeight {} is not the font size {}",
					input.lineHeight,
					hidden.renderer.get_font_size()));
			finish_the_child();
		}

		// Loads the game font as Renderer::load_font does and checks that one
		// line's box is as tall as the size it is drawn at, at the shipped size
		// and another.
		//
		// Example:
		//   exit_code_in_child(&a_line_is_as_tall_as_its_font_size);
		//   // -> 44, or 43 naming the size whose box differs
		[[noreturn]] void a_line_is_as_tall_as_its_font_size()
		{
			HiddenRenderer hidden{};
			const std::string path =
				Paths::resolve(Paths::DAWNLIKE_FONT).string();
			for (const int size : { HIDDEN_RENDERER_FONT_SIZE, 16 })
			{
				const Font font = LoadFontEx(path.c_str(), size, nullptr, 256);
				const Vector2 box = MeasureTextEx(
					font,
					"[Ag] Up/Down",
					static_cast<float>(size),
					1.0f);
				UnloadFont(font);
				exit_unless(
					static_cast<int>(box.y) == size,
					std::format(
						"a {}-pixel line's box is {} tall",
						size,
						box.y));
			}
			finish_the_child();
		}
	} // namespace

	TEST(EditorLayoutTest, TheListIsItsPadsColumnsAndWidestLabel)
	{
		// 6 + 26 + 200 + 30 + 6
		EXPECT_EQ(EditorLayout::list_panel_width(200, 26, 30, 1920), 268);
	}

	TEST(EditorLayoutTest, TheListIsAtMostHalfTheScreen)
	{
		EXPECT_EQ(EditorLayout::list_panel_width(2000, 0, 0, 1920), 960);
	}

	TEST(EditorLayoutTest, TheTagColumnIsTheWidestTagAndAPad)
	{
		EXPECT_EQ(EditorLayout::tag_column(10), 16);
	}

	TEST(EditorLayoutTest, ListsWithoutTagsKeepNoTagColumn)
	{
		EXPECT_EQ(EditorLayout::tag_column(0), 0);
	}

	TEST(EditorLayoutTest, ATagStartsAtTheTagColumnsLeftEdge)
	{
		EXPECT_EQ(EditorLayout::tag_left(218, 14), 204);
	}

	TEST(EditorLayoutTest, ABandIsItsLinesAndTwoPads)
	{
		EXPECT_EQ(EditorLayout::text_band_height(30, 1), 34);
		EXPECT_EQ(EditorLayout::text_band_height(30, 2), 64);
		EXPECT_EQ(EditorLayout::text_band_height(16, 1), 20);
	}

	TEST(EditorLayoutTest, TheHeaderHoldsTwoLinesAndTheHintOne)
	{
		EXPECT_EQ(EditorLayout::header_height(30), 64);
		EXPECT_EQ(EditorLayout::hint_height(30), 34);
	}

	TEST(EditorLayoutTest, ARowHoldsALineOrATileWhicheverIsTaller)
	{
		EXPECT_EQ(EditorLayout::row_height(30), 34);
		// A 16-pixel line needs 20; the 22-pixel tile and its pads need 26.
		EXPECT_EQ(EditorLayout::row_height(16), 26);
	}

	TEST(EditorLayoutTest, TheBodyIsTheScreenLessHeaderAndHint)
	{
		// 1061 - 64 - 34, which holds 28 rows of 34.
		EXPECT_EQ(EditorLayout::body_height(1061, 30), 963);
		EXPECT_EQ(whole_rows_within(963, EditorLayout::row_height(30)), 28u);
	}

	TEST(EditorLayoutTest, HeaderLinesStackBelowAPad)
	{
		EXPECT_EQ(EditorLayout::header_line_top(0, 30), 2);
		EXPECT_EQ(EditorLayout::header_line_top(1, 30), 32);
	}

	TEST(EditorLayoutTest, RowsStackDownFromThePanelsTop)
	{
		EXPECT_EQ(EditorLayout::row_top(64, 3, 34), 166);
	}

	TEST(EditorLayoutTest, ALineIsCentredInItsBand)
	{
		// (30 - 16) / 2 = 7 below the band's top.
		EXPECT_EQ(EditorLayout::centred_text_top(0, 30, 16), 7);
		EXPECT_EQ(EditorLayout::centred_text_top(100, 30, 16), 107);
	}

	TEST(EditorLayoutTest, ALineTallerThanItsBandOverhangsItEqually)
	{
		// A 30-pixel line in a 28-pixel band at 1033: one pixel above, one
		// below.
		EXPECT_EQ(EditorLayout::centred_text_top(1033, 28, 30), 1032);
	}

	TEST(EditorLayoutTest, ALabelThatFitsIsKeptWhole)
	{
		EXPECT_EQ(EditorLayout::fitted_label("Bless", 80, MEASURE), "Bless");
		// Exactly the room: 10 characters in 80 pixels.
		EXPECT_EQ(
			EditorLayout::fitted_label("Protection", 80, MEASURE),
			"Protection");
	}

	TEST(EditorLayoutTest, ALongLabelKeepsTheLongestPrefixThatFitsWithDots)
	{
		// 80 pixels hold 10 characters: 7 of the name and "...".
		EXPECT_EQ(
			EditorLayout::fitted_label("Protection From Evil", 80, MEASURE),
			"Protect...");
	}

	TEST(EditorLayoutTest, NoLabelWhenNotEvenTheDotsFit)
	{
		// "..." is 24 pixels.
		EXPECT_EQ(EditorLayout::fitted_label("Bless", 23, MEASURE), "");
	}

	// The real frame's input carries the font's size, which every band is
	// sized from.
	TEST(EditorLayoutTest, TheRealInputCarriesTheFontSize)
	{
		const int childExitCode =
			exit_code_in_child(&the_real_input_carries_the_font_size);

		if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
		{
			GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
		}
		EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
			<< "the child's check that failed is in its output above";
	}

	// The editor's bands are sized from the font size, which holds only while a
	// line's box is exactly that tall.
	TEST(EditorLayoutTest, ALineIsAsTallAsItsFontSize)
	{
		const int childExitCode =
			exit_code_in_child(&a_line_is_as_tall_as_its_font_size);

		if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
		{
			GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
		}
		EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
			<< "the child's check that failed is in its output above";
	}

	// The real frame's input measures text in the font the list draws with.
	TEST(EditorLayoutTest, TheRealInputMeasuresInTheGameFont)
	{
		const int childExitCode =
			exit_code_in_child(&the_real_input_measures_in_the_game_font);

		if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
		{
			GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
		}
		EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
			<< "the child's check that failed is in its output above";
	}

} // namespace CppRL
