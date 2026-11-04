// file: RecordEditorTest.cpp
// The shared editor frame, driven through EditorInput snapshots once for each
// editor's Model: a click selects a list row, Tab and the arrows move the
// cursors, Ctrl steps a number by ten, Enter types into a text field, A and
// Delete add and remove a custom record, and Esc commits and leaves. While a
// field is being typed into or the tile picker is open, Esc only cancels and
// Tab and Ctrl+S do nothing. A builtin spell's effect ignores every edit key.
// Every expected value is read from the Model's own field table and bounds.
//
// Saving writes the data files Paths::resolve finds. The one test that
// presses Ctrl+S first moves its child into a probe folder with an empty
// data/content, and checks the redirect before any key.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RecordEditorTest*

#include <gtest/gtest.h>

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "src/EditorField.h"
#include "src/EditorInput.h"
#include "src/ItemEditor.h"
#include "src/MonsterEditor.h"
#include "src/Paths.h"
#include "src/RecordEditor.h"
#include "src/SpellEditor.h"
#include "src/SpellRegistry.h"
#include "src/Vector2D.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/MockGameContext.h"
#include "tests/mocks/TempTestPath.h"

namespace CppRL
{

	namespace
	{
		// The height the tests give the window: tall enough that every row a
		// test reaches is on screen.
		constexpr int SCREEN_HEIGHT = 900;
		constexpr int SCREEN_WIDTH = 1280;
		// The stand-in font's line height: a row is then 26, the tile's floor.
		constexpr int LINE_HEIGHT = 16;

		// A stand-in font: every character is 8 pixels wide.
		int eight_per_character(std::string_view text)
		{
			return static_cast<int>(text.size()) * 8;
		}

		// One frame's input with these keys pressed and these held.
		//
		// Example:
		//   const EditorInput step =
		//       frame({ EditorKey::RIGHT }, { EditorKey::CONTROL });
		//   step.is_held(EditorKey::CONTROL);   // -> true
		EditorInput frame(std::set<EditorKey> pressed, std::set<EditorKey> held)
		{
			EditorInput input{};
			input.pressed = std::move(pressed);
			input.held = std::move(held);
			input.screenWidth = SCREEN_WIDTH;
			input.screenHeight = SCREEN_HEIGHT;
			input.measure = &eight_per_character;
			input.lineHeight = LINE_HEIGHT;
			return input;
		}

		// A click at a screen point.
		EditorInput click_at(Vector2D point)
		{
			EditorInput input = frame({ EditorKey::CLICK }, {});
			input.mouse = point;
			return input;
		}

		// The middle of list row n, counted from the top of the list panel.
		Vector2D list_row(size_t row)
		{
			return Vector2D{
				EditorLayout::LIST_PAD,
				EditorLayout::row_top(
					EditorLayout::header_height(LINE_HEIGHT),
					static_cast<int>(row),
					EditorLayout::row_height(LINE_HEIGHT)) +
					EditorLayout::row_height(LINE_HEIGHT) / 2,
			};
		}

		// The middle of field row n, counted from the top of the field panel,
		// at the screen's right side, where no list reaches.
		Vector2D field_row(size_t row)
		{
			return Vector2D{
				SCREEN_WIDTH - 40,
				EditorLayout::row_top(
					EditorLayout::header_height(LINE_HEIGHT),
					static_cast<int>(row),
					EditorLayout::row_height(LINE_HEIGHT)) +
					EditorLayout::row_height(LINE_HEIGHT) / 2,
			};
		}

		// The integer a plain number field shows.
		int shown_number(const std::string& text)
		{
			int value = 0;
			const auto [end, error] =
				std::from_chars(text.data(), text.data() + text.size(), value);
			EXPECT_EQ(error, std::errc{}) << "not a number: \"" << text << "\"";
			EXPECT_EQ(end, text.data() + text.size())
				<< "not a number: \"" << text << "\"";
			return value;
		}

		// The first field of a kind in a Model's table, by position.
		template <typename Model>
		std::optional<size_t> first_field_of_kind(FieldKind kind)
		{
			const auto is_of_kind = [kind](const auto& descriptor)
			{
				return descriptor.kind == kind;
			};
			const auto found = std::ranges::find_if(Model::FIELDS, is_of_kind);
			if (found == Model::FIELDS.end())
			{
				return std::nullopt;
			}
			return static_cast<size_t>(found - Model::FIELDS.begin());
		}

		// The first plain number field in a Model's table, by position.
		template <typename Model>
		std::optional<size_t> first_plain_number()
		{
			const auto is_plain_number = [](const auto& descriptor)
			{
				return descriptor.plain.has_value();
			};
			const auto found =
				std::ranges::find_if(Model::FIELDS, is_plain_number);
			if (found == Model::FIELDS.end())
			{
				return std::nullopt;
			}
			return static_cast<size_t>(found - Model::FIELDS.begin());
		}

		// A field's row in a Model's table, found by its id.
		template <typename Model>
		size_t row_of(typename Model::FieldId field)
		{
			const auto is_field = [field](const auto& descriptor)
			{
				return descriptor.field == field;
			};
			return static_cast<size_t>(
				std::ranges::find_if(Model::FIELDS, is_field) -
				Model::FIELDS.begin());
		}

		// Esc after a Ctrl+Right leaves the editor and opens the main menu, and
		// the record it held is committed, so the editor opened again shows the
		// stepped number. Runs in a HiddenRenderer child.
		template <typename Model>
		[[noreturn]] void escape_commits_and_leaves()
		{
			HiddenRenderer hidden{};
			MockGameContext mock{};
			GameContext ctx = mock.to_game_context();
			ctx.renderer = &hidden.renderer;
			RecordEditor<Model> editor{};
			editor.enter(ctx);
			const std::optional<size_t> field = first_plain_number<Model>();
			exit_unless(field.has_value(), "the model has no plain number");
			const auto fieldId = Model::FIELDS.at(*field).field;
			editor.handle_input(click_at(field_row(*field)), ctx);
			editor.handle_input(
				frame({ EditorKey::RIGHT }, { EditorKey::CONTROL }),
				ctx);
			const std::string edited =
				editor.record_model().field_value(fieldId);

			editor.handle_input(frame({ EditorKey::ESCAPE }, {}), ctx);

			exit_unless(!editor.is_active(), "Esc left the editor active");
			exit_unless(mock.menus.size() == 1, "Esc opened no main menu");
			RecordEditor<Model> reopened{};
			reopened.enter(ctx);
			exit_unless(
				reopened.record_model().field_value(fieldId) == edited,
				"the stepped number was not committed");
			finish_the_child();
		}

		// Every field of the selected record as the editor shows it, in table
		// order.
		template <typename Model>
		std::vector<std::string> field_values(const Model& model)
		{
			std::vector<std::string> values{};
			for (const auto& descriptor : Model::FIELDS)
			{
				values.push_back(model.field_value(descriptor.field));
			}
			return values;
		}

		// Esc while typing discards the text and returns to the fields: the
		// editor stays open and the field keeps its value. A second Esc leaves.
		// Runs in a HiddenRenderer child, because an Esc that leaves opens the
		// main menu.
		template <typename Model>
		[[noreturn]] void escape_while_typing_cancels()
		{
			HiddenRenderer hidden{};
			MockGameContext mock{};
			GameContext ctx = mock.to_game_context();
			ctx.renderer = &hidden.renderer;
			RecordEditor<Model> editor{};
			editor.enter(ctx);
			const std::optional<size_t> field =
				first_field_of_kind<Model>(FieldKind::TEXT);
			exit_unless(field.has_value(), "the model has no text field");
			const auto fieldId = Model::FIELDS.at(*field).field;
			editor.handle_input(click_at(field_row(*field)), ctx);
			const std::string before =
				editor.record_model().field_value(fieldId);
			editor.handle_input(frame({ EditorKey::ENTER }, {}), ctx);
			EditorInput typing = frame({}, {});
			typing.typed = "Qz";
			editor.handle_input(typing, ctx);

			editor.handle_input(frame({ EditorKey::ESCAPE }, {}), ctx);

			exit_unless(editor.is_active(), "Esc while typing left the editor");
			exit_unless(mock.menus.empty(), "Esc while typing opened a menu");
			exit_unless(
				editor.editor_mode() == EditorMode::NORMAL,
				"Esc while typing kept typing");
			exit_unless(
				editor.record_model().field_value(fieldId) == before,
				"Esc while typing kept the typed text");
			editor.handle_input(frame({ EditorKey::ESCAPE }, {}), ctx);
			exit_unless(
				!editor.is_active(),
				"the second Esc left the editor active");
			exit_unless(
				mock.menus.size() == 1,
				"the second Esc opened no main menu");
			finish_the_child();
		}

		// Esc in the tile picker returns to the fields with the record
		// unchanged, and the editor stays open. Runs in a HiddenRenderer child:
		// the picker reads the renderer's sheets, and an Esc that leaves opens
		// the main menu.
		template <typename Model>
		[[noreturn]] void escape_in_the_picker_returns()
		{
			HiddenRenderer hidden{};
			MockGameContext mock{};
			GameContext ctx = mock.to_game_context();
			ctx.renderer = &hidden.renderer;
			RecordEditor<Model> editor{};
			editor.enter(ctx);
			editor.handle_input(frame({ EditorKey::TAB }, {}), ctx);
			const std::vector<std::string> before =
				field_values(editor.record_model());
			editor.handle_input(frame({ EditorKey::PICKER }, {}), ctx);
			exit_unless(
				editor.editor_mode() == EditorMode::TILE_PICKER,
				"F2 opened no picker");

			editor.handle_input(frame({ EditorKey::ESCAPE }, {}), ctx);

			exit_unless(
				editor.is_active(),
				"Esc in the picker left the editor");
			exit_unless(mock.menus.empty(), "Esc in the picker opened a menu");
			exit_unless(
				editor.editor_mode() == EditorMode::NORMAL,
				"Esc in the picker kept the picker open");
			exit_unless(
				field_values(editor.record_model()) == before,
				"Esc in the picker changed the record");
			finish_the_child();
		}

		// Ctrl+S while typing saves nothing and keeps the typing. Saving writes
		// wherever Paths::resolve finds data/, so the child first moves into a
		// probe folder holding an empty data/content, and stops before any key
		// unless the items file would land there. The real data files are never
		// reachable.
		template <typename Model>
		[[noreturn]] void ctrl_s_while_typing_saves_nothing()
		{
			HiddenRenderer hidden{};
			MockGameContext mock{};
			GameContext ctx = mock.to_game_context();
			ctx.renderer = &hidden.renderer;
			RecordEditor<Model> editor{};
			editor.enter(ctx);
			const std::filesystem::path probe = temp_test_path("probe");
			const std::filesystem::path content = probe / "data" / "content";
			std::filesystem::remove_all(probe);
			std::filesystem::create_directories(content);
			std::filesystem::current_path(probe);
			exit_unless(
				std::filesystem::equivalent(
					Paths::resolve(Paths::ITEMS).parent_path(),
					content),
				"a save would not land in the probe folder");
			const std::optional<size_t> field =
				first_field_of_kind<Model>(FieldKind::TEXT);
			exit_unless(field.has_value(), "the model has no text field");
			editor.handle_input(click_at(field_row(*field)), ctx);
			editor.handle_input(frame({ EditorKey::ENTER }, {}), ctx);
			EditorInput typing = frame({}, {});
			typing.typed = "Qz";
			editor.handle_input(typing, ctx);

			editor.handle_input(
				frame({ EditorKey::SAVE }, { EditorKey::CONTROL }),
				ctx);

			exit_unless(
				editor.editor_mode() == EditorMode::EDIT_STRING,
				"Ctrl+S ended the typing");
			exit_unless(
				std::filesystem::is_empty(content),
				"Ctrl+S while typing saved");
			// The probe is named for this child's process, so no later run
			// clears it; step out of it so Windows lets it go, then remove it.
			std::filesystem::current_path(probe.parent_path());
			std::filesystem::remove_all(probe);
			finish_the_child();
		}
	} // namespace

	template <typename Model>
	class RecordEditorTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ctx = mock.to_game_context();
			editor.enter(ctx);
			keys = editor.record_model().keys(ctx);
			ASSERT_GE(keys.size(), 3u) << "too few records to click the third";
		}

		// The selected record's field n as the editor shows it.
		std::string shown(size_t field) const
		{
			return editor.record_model().field_value(
				Model::FIELDS.at(field).field);
		}

		MockGameContext mock{};
		GameContext ctx{};
		RecordEditor<Model> editor{};
		std::vector<std::string> keys{};
	};

	using EditorModels =
		::testing::Types<ItemEditorModel, MonsterEditorModel, SpellEditorModel>;
	TYPED_TEST_SUITE(RecordEditorTest, EditorModels);

	TYPED_TEST(RecordEditorTest, AClickOnAListRowSelectsThatRecord)
	{
		this->editor.handle_input(click_at(list_row(2)), this->ctx);

		EXPECT_EQ(this->editor.current_key(), this->keys[2]);
		EXPECT_EQ(this->editor.editor_focus(), EditorFocus::LIST);
	}

	// The working copy shown in the fields is the clicked record, as the
	// registry holds it.
	TYPED_TEST(RecordEditorTest, AClickOnAListRowLoadsThatRecord)
	{
		this->editor.handle_input(click_at(list_row(2)), this->ctx);

		EXPECT_EQ(
			this->editor.record_model().working_row_label(this->keys[2]),
			this->editor.record_model().row_label(this->ctx, this->keys[2]));
		EXPECT_NE(
			this->editor.record_model().working_row_label(this->keys[2]),
			this->editor.record_model().row_label(this->ctx, this->keys[0]));
	}

	// Above the list the row count is negative, which would round to the first
	// row.
	TYPED_TEST(RecordEditorTest, AClickInTheHeaderSelectsNothing)
	{
		this->editor.handle_input(click_at(list_row(2)), this->ctx);

		this->editor.handle_input(click_at(Vector2D{ 4, 4 }), this->ctx);

		EXPECT_EQ(this->editor.current_key(), this->keys[2]);
	}

	TYPED_TEST(RecordEditorTest, DownInTheListSelectsTheNextRecord)
	{
		this->editor.handle_input(frame({ EditorKey::DOWN }, {}), this->ctx);

		EXPECT_EQ(this->editor.current_key(), this->keys[1]);
	}

	TYPED_TEST(RecordEditorTest, TabMovesFocusToTheFieldsAndBack)
	{
		this->editor.handle_input(frame({ EditorKey::TAB }, {}), this->ctx);
		EXPECT_EQ(this->editor.editor_focus(), EditorFocus::FIELDS);

		this->editor.handle_input(frame({ EditorKey::TAB }, {}), this->ctx);
		EXPECT_EQ(this->editor.editor_focus(), EditorFocus::LIST);
	}

	TYPED_TEST(RecordEditorTest, DownInTheFieldsMovesTheFieldCursor)
	{
		this->editor.handle_input(frame({ EditorKey::TAB }, {}), this->ctx);
		this->editor.handle_input(frame({ EditorKey::DOWN }, {}), this->ctx);

		EXPECT_EQ(this->editor.current_field(), TypeParam::FIELDS.at(1).field);
	}

	TYPED_TEST(
		RecordEditorTest,
		ACtrlArrowStepsAPlainNumberByTenAndABareOneByOne)
	{
		const std::optional<size_t> field = first_plain_number<TypeParam>();
		ASSERT_TRUE(field.has_value()) << "the model has no plain number";
		const auto& plain = *TypeParam::FIELDS[*field].plain;
		this->editor.handle_input(click_at(field_row(*field)), this->ctx);
		const int before = shown_number(this->shown(*field));

		this->editor.handle_input(
			frame({ EditorKey::RIGHT }, { EditorKey::CONTROL }),
			this->ctx);
		const int afterTen = std::min(before + 10, plain.maximum);
		EXPECT_EQ(shown_number(this->shown(*field)), afterTen);

		this->editor.handle_input(frame({ EditorKey::LEFT }, {}), this->ctx);
		EXPECT_EQ(
			shown_number(this->shown(*field)),
			std::max(afterTen - 1, plain.minimum));
	}

	TYPED_TEST(RecordEditorTest, EnterOnATextFieldTypesIntoIt)
	{
		const std::optional<size_t> field =
			first_field_of_kind<TypeParam>(FieldKind::TEXT);
		ASSERT_TRUE(field.has_value()) << "the model has no text field";
		this->editor.handle_input(click_at(field_row(*field)), this->ctx);
		const std::string before = this->shown(*field);

		this->editor.handle_input(frame({ EditorKey::ENTER }, {}), this->ctx);
		EXPECT_EQ(this->editor.editor_mode(), EditorMode::EDIT_STRING);
		EditorInput typing = frame({}, {});
		typing.typed = "Qz";
		this->editor.handle_input(typing, this->ctx);
		this->editor.handle_input(
			frame({ EditorKey::BACKSPACE }, {}),
			this->ctx);
		this->editor.handle_input(frame({ EditorKey::ENTER }, {}), this->ctx);

		EXPECT_EQ(this->editor.editor_mode(), EditorMode::NORMAL);
		EXPECT_EQ(this->shown(*field), before + "Q");
	}

	TYPED_TEST(RecordEditorTest, EscapeCommitsTheRecordAndLeaves)
	{
		// Leaving opens the main menu, which needs a window, so it runs in a
		// child.
		const int childExitCode =
			exit_code_in_child(&escape_commits_and_leaves<TypeParam>);
		if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
		{
			GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
		}
		EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED);
	}

	TYPED_TEST(RecordEditorTest, EscWhileTypingCancelsTheEditAndStays)
	{
		const int childExitCode =
			exit_code_in_child(&escape_while_typing_cancels<TypeParam>);
		if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
		{
			GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
		}
		EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED);
	}

	TYPED_TEST(RecordEditorTest, EscInThePickerReturnsToTheFieldsAndStays)
	{
		if constexpr (std::is_same_v<TypeParam, SpellEditorModel>)
		{
			GTEST_SKIP() << "spells have no tile picker";
		}
		const int childExitCode =
			exit_code_in_child(&escape_in_the_picker_returns<TypeParam>);
		if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
		{
			GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
		}
		EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED);
	}

	TYPED_TEST(RecordEditorTest, TabWhileTypingKeepsFocus)
	{
		const std::optional<size_t> field =
			first_field_of_kind<TypeParam>(FieldKind::TEXT);
		ASSERT_TRUE(field.has_value()) << "the model has no text field";
		this->editor.handle_input(click_at(field_row(*field)), this->ctx);
		this->editor.handle_input(frame({ EditorKey::ENTER }, {}), this->ctx);
		EditorInput typing = frame({}, {});
		typing.typed = "Qz";
		this->editor.handle_input(typing, this->ctx);
		ASSERT_EQ(this->editor.editor_focus(), EditorFocus::FIELDS);
		const auto typedInto = this->editor.current_field();

		this->editor.handle_input(frame({ EditorKey::TAB }, {}), this->ctx);

		EXPECT_EQ(this->editor.editor_focus(), EditorFocus::FIELDS);
		EXPECT_EQ(this->editor.editor_mode(), EditorMode::EDIT_STRING);
		EXPECT_EQ(this->editor.current_field(), typedInto);
	}

	TYPED_TEST(RecordEditorTest, CtrlSWhileTypingDoesNotSave)
	{
		// Saving needs the window's clock, and the child moves its working
		// folder into a probe, so this runs in a child.
		const int childExitCode =
			exit_code_in_child(&ctrl_s_while_typing_saves_nothing<TypeParam>);
		if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
		{
			GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
		}
		EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED);
	}

	TYPED_TEST(RecordEditorTest, AddSelectsTheNewRecordAndFocusesItsFields)
	{
		this->editor.handle_input(frame({ EditorKey::ADD }, {}), this->ctx);

		EXPECT_FALSE(this->editor.record_model().is_builtin(
			this->ctx,
			this->editor.current_key()));
		EXPECT_EQ(this->editor.editor_focus(), EditorFocus::FIELDS);
		EXPECT_EQ(
			this->editor.record_model().keys(this->ctx).size(),
			this->keys.size() + 1);
	}

	// Shipped records come first and custom ones after, so the line between
	// them is drawn once, above the first custom record, however many follow
	// it. A Model may ship custom records of its own, so the rows are read from
	// it.
	TYPED_TEST(
		RecordEditorTest,
		CustomRecordsAreListedLastWithOneDividerAboveThem)
	{
		auto divider_rows = [this]()
		{
			std::vector<size_t> rows{};
			const size_t count =
				this->editor.record_model().keys(this->ctx).size();
			for (size_t row = 0; row < count; ++row)
			{
				if (this->editor.has_divider_above(this->ctx, row))
				{
					rows.push_back(row);
				}
			}
			return rows;
		};
		this->editor.handle_input(frame({ EditorKey::ADD }, {}), this->ctx);
		const std::string first = this->editor.current_key();
		this->editor.handle_input(frame({ EditorKey::TAB }, {}), this->ctx);
		this->editor.handle_input(frame({ EditorKey::ADD }, {}), this->ctx);

		const std::vector<std::string> after =
			this->editor.record_model().keys(this->ctx);
		ASSERT_EQ(after.size(), this->keys.size() + 2);
		EXPECT_EQ(
			std::ranges::count(after, first) +
				std::ranges::count(after, this->editor.current_key()),
			2);
		std::vector<size_t> customRows{};
		for (size_t row = 0; row < after.size(); ++row)
		{
			if (!this->editor.record_model().is_builtin(this->ctx, after[row]))
			{
				customRows.push_back(row);
			}
		}
		ASSERT_GE(customRows.size(), 2u) << "the two added records are custom";
		ASSERT_GT(customRows.front(), 0u) << "no shipped record before them";
		// Every row from the first custom one to the end is custom.
		EXPECT_EQ(customRows.size(), after.size() - customRows.front());
		EXPECT_EQ(divider_rows(), std::vector<size_t>{ customRows.front() });
	}

	TYPED_TEST(RecordEditorTest, DeleteRemovesACustomRecordAndKeepsAShippedOne)
	{
		this->editor.handle_input(frame({ EditorKey::REMOVE }, {}), this->ctx);
		EXPECT_EQ(this->editor.record_model().keys(this->ctx), this->keys);

		this->editor.handle_input(frame({ EditorKey::ADD }, {}), this->ctx);
		const std::string added = this->editor.current_key();
		this->editor.handle_input(frame({ EditorKey::TAB }, {}), this->ctx);
		this->editor.handle_input(frame({ EditorKey::REMOVE }, {}), this->ctx);

		const std::vector<std::string> after =
			this->editor.record_model().keys(this->ctx);
		EXPECT_EQ(std::ranges::count(after, added), 0);
		EXPECT_EQ(after.size(), this->keys.size());
	}

	// F2 opens the tile picker for items and monsters, which have tiles, and
	// leaves a spell, which has none, in the field list.
	TYPED_TEST(RecordEditorTest, F2OpensThePickerOnlyForRecordsWithTiles)
	{
		constexpr bool hasTiles = !std::is_same_v<TypeParam, SpellEditorModel>;
		this->editor.handle_input(frame({ EditorKey::TAB }, {}), this->ctx);
		this->editor.handle_input(frame({ EditorKey::PICKER }, {}), this->ctx);

		const EditorMode expected =
			hasTiles ? EditorMode::TILE_PICKER : EditorMode::NORMAL;
		EXPECT_EQ(this->editor.editor_mode(), expected);
	}

	// A spell row carries its class, "C" for a cleric spell and "W" otherwise;
	// an item or a monster row carries no tag.
	TYPED_TEST(RecordEditorTest, OnlyASpellRowCarriesItsClass)
	{
		constexpr bool isSpell = std::is_same_v<TypeParam, SpellEditorModel>;
		for (const std::string& key : this->keys)
		{
			const std::optional<RowTag> tag =
				this->editor.record_model().row_tag(this->ctx, key);
			ASSERT_EQ(tag.has_value(), isSpell) << key;
			if constexpr (isSpell)
			{
				const bool isCleric =
					this->ctx.spellRegistry->get_by_key(key).spellClass ==
					SpellClass::CLERIC;
				EXPECT_EQ(tag->text, isCleric ? "C" : "W") << key;
			}
		}
		EXPECT_EQ(
			this->editor.record_model().working_row_tag().has_value(),
			isSpell);
	}

	// The XP field edits one hit dice count of the selected monster. Choosing
	// another monster selects its first count, so a mimic's second count is not
	// carried to a goblin that has one.
	TEST(
		RecordEditorMonsterTest,
		ChoosingAnotherMonsterSelectsItsFirstHitDiceCount)
	{
		using Field = MonsterEditorModel::FieldId;
		MockGameContext mock{};
		GameContext ctx = mock.to_game_context();
		MonsterEditor editor{};
		editor.enter(ctx);
		const std::vector<std::string> keys = editor.record_model().keys(ctx);
		const auto mimic = std::ranges::find(keys, "mimic");
		ASSERT_NE(mimic, keys.end()) << "the shipped monsters have no mimic";
		editor.handle_input(
			click_at(list_row(static_cast<size_t>(mimic - keys.begin()))),
			ctx);
		editor.handle_input(
			click_at(field_row(row_of<MonsterEditorModel>(Field::XP_COUNT))),
			ctx);
		editor.handle_input(frame({ EditorKey::RIGHT }, {}), ctx);
		ASSERT_EQ(editor.record_model().field_value(Field::XP_COUNT), "10 HD");

		editor.handle_input(click_at(list_row(0)), ctx);

		EXPECT_EQ(editor.current_key(), "goblin");
		EXPECT_EQ(editor.record_model().field_value(Field::XP_COUNT), "1 HD");
		EXPECT_EQ(editor.record_model().field_value(Field::XP), "105");
	}

	// A custom spell's Effect is cycled: Right and Enter step it forward, Left
	// steps it back. A new spell starts at no effect, and the first effect is
	// Cure Light Wounds.
	TEST(RecordEditorSpellTest, ACustomSpellsEffectStillCycles)
	{
		using Field = SpellEditorModel::FieldId;
		MockGameContext mock{};
		GameContext ctx = mock.to_game_context();
		SpellEditor editor{};
		editor.enter(ctx);
		editor.handle_input(frame({ EditorKey::ADD }, {}), ctx);
		ASSERT_FALSE(
			editor.record_model().is_builtin(ctx, editor.current_key()));
		editor.handle_input(
			click_at(field_row(row_of<SpellEditorModel>(Field::EFFECT_TYPE))),
			ctx);
		ASSERT_EQ(
			editor.record_model().field_value(Field::EFFECT_TYPE),
			"None");

		editor.handle_input(frame({ EditorKey::RIGHT }, {}), ctx);
		EXPECT_EQ(
			editor.record_model().field_value(Field::EFFECT_TYPE),
			"Cure Light Wounds");
		editor.handle_input(frame({ EditorKey::LEFT }, {}), ctx);
		EXPECT_EQ(
			editor.record_model().field_value(Field::EFFECT_TYPE),
			"None");
		editor.handle_input(frame({ EditorKey::ENTER }, {}), ctx);
		EXPECT_EQ(
			editor.record_model().field_value(Field::EFFECT_TYPE),
			"Cure Light Wounds");
	}

	// A builtin spell's effect is compiled in and is the spell's identity, so
	// no key changes it, and committing the record leaves the registry's effect
	// as it was. "Knock" is the shipped spells.json name of the knock spell.
	TEST(RecordEditorSpellTest, ABuiltinSpellsEffectIgnoresEveryEditKey)
	{
		using Field = SpellEditorModel::FieldId;
		MockGameContext mock{};
		GameContext ctx = mock.to_game_context();
		SpellEditor editor{};
		editor.enter(ctx);
		const std::vector<std::string> keys = editor.record_model().keys(ctx);
		const auto knock = std::ranges::find(keys, "knock");
		ASSERT_NE(knock, keys.end()) << "the builtin spells have no knock";
		editor.handle_input(
			click_at(list_row(static_cast<size_t>(knock - keys.begin()))),
			ctx);
		ASSERT_EQ(editor.current_key(), "knock");
		editor.handle_input(
			click_at(field_row(row_of<SpellEditorModel>(Field::EFFECT_TYPE))),
			ctx);

		const std::vector<EditorInput> everyEditKey{
			frame({ EditorKey::RIGHT }, {}),
			frame({ EditorKey::LEFT }, {}),
			frame({ EditorKey::RIGHT }, { EditorKey::CONTROL }),
			frame({ EditorKey::ENTER }, {}),
		};
		for (const EditorInput& press : everyEditKey)
		{
			editor.handle_input(press, ctx);
			EXPECT_EQ(
				editor.record_model().field_value(Field::EFFECT_TYPE),
				"Knock");
		}
		editor.handle_input(click_at(list_row(0)), ctx);
		EXPECT_EQ(
			ctx.spellRegistry->get_by_key("knock").effect_type,
			SpellEffectType::KNOCK);
	}

	// The spell list is as wide as its widest label and its class tag, each
	// measured as drawn: a pad, no tile column, the label, the tag and its pad,
	// and a pad.
	TEST(RecordEditorSpellTest, TheListIsAsWideAsItsWidestLabelAndTag)
	{
		MockGameContext mock{};
		GameContext ctx = mock.to_game_context();
		SpellEditor editor{};
		editor.enter(ctx);
		int widestLabel = 0;
		for (const std::string& key : ctx.spellRegistry->get_all_keys())
		{
			const SpellDefinition& spell = ctx.spellRegistry->get_by_key(key);
			std::string label =
				"[" + std::to_string(spell.level) + "] " + spell.name;
			if (!ctx.spellRegistry->is_builtin_key(key))
			{
				label += " *";
			}
			widestLabel = std::max(widestLabel, eight_per_character(label));
		}
		const int classTag = eight_per_character("C");

		EXPECT_EQ(
			editor.list_width(frame({}, {}), ctx),
			EditorLayout::LIST_PAD + widestLabel + classTag +
				EditorLayout::LIST_PAD + EditorLayout::LIST_PAD);
	}

	// An item list keeps a tile column: with a custom item named wider than any
	// other, the list is a pad, the tile column, that name with its " *", and a
	// pad, as items carry no tag.
	TEST(RecordEditorItemTest, TheListMakesRoomForTheTileColumn)
	{
		using Field = ItemEditorModel::FieldId;
		MockGameContext mock{};
		GameContext ctx = mock.to_game_context();
		ItemEditor editor{};
		editor.enter(ctx);
		editor.handle_input(frame({ EditorKey::ADD }, {}), ctx);
		editor.handle_input(
			click_at(field_row(row_of<ItemEditorModel>(Field::NAME))),
			ctx);
		editor.handle_input(frame({ EditorKey::ENTER }, {}), ctx);
		EditorInput typing = frame({}, {});
		typing.typed = std::string(50, 'W');
		editor.handle_input(typing, ctx);
		editor.handle_input(frame({ EditorKey::ENTER }, {}), ctx);
		const std::string name = editor.record_model().field_value(Field::NAME);
		const int customLabel = eight_per_character(name + " *");

		EXPECT_EQ(
			editor.list_width(frame({}, {}), ctx),
			EditorLayout::LIST_PAD + EditorLayout::LIST_TILE_COLUMN +
				customLabel + EditorLayout::LIST_PAD);
	}

	// A custom record's label is measured as drawn, " *" included: with a
	// custom spell named wider than any other, the list is a pad, "[level] name
	// *", the class tag and its pad, and a pad.
	TEST(RecordEditorSpellTest, TheListMakesRoomForACustomRecordsMark)
	{
		using Field = SpellEditorModel::FieldId;
		MockGameContext mock{};
		GameContext ctx = mock.to_game_context();
		SpellEditor editor{};
		editor.enter(ctx);
		editor.handle_input(frame({ EditorKey::ADD }, {}), ctx);
		editor.handle_input(
			click_at(field_row(row_of<SpellEditorModel>(Field::NAME))),
			ctx);
		editor.handle_input(frame({ EditorKey::ENTER }, {}), ctx);
		EditorInput typing = frame({}, {});
		typing.typed = std::string(50, 'W');
		editor.handle_input(typing, ctx);
		editor.handle_input(frame({ EditorKey::ENTER }, {}), ctx);
		const std::string label = "[" +
			editor.record_model().field_value(Field::LEVEL) + "] " +
			editor.record_model().field_value(Field::NAME) + " *";
		const int classTag = eight_per_character("C");

		EXPECT_EQ(
			editor.list_width(frame({}, {}), ctx),
			EditorLayout::LIST_PAD + eight_per_character(label) + classTag +
				EditorLayout::LIST_PAD + EditorLayout::LIST_PAD);
	}

	// Only a builtin spell's Effect is fixed: its other fields are edited as
	// any spell's are, and a custom spell's Effect is edited too.
	TEST(RecordEditorSpellTest, OnlyABuiltinSpellsEffectIsReadOnly)
	{
		using Field = SpellEditorModel::FieldId;
		MockGameContext mock{};
		GameContext ctx = mock.to_game_context();
		SpellEditorModel model{};

		model.load(ctx, "knock");
		EXPECT_TRUE(model.is_read_only(Field::EFFECT_TYPE));
		EXPECT_FALSE(model.is_read_only(Field::NAME));
		const std::string custom = model.add_record(ctx);
		model.load(ctx, custom);
		EXPECT_FALSE(model.is_read_only(Field::EFFECT_TYPE));
	}

	// The list ends where its width says: a click on its last pixel selects a
	// row, and a click one pixel further is in the fields.
	TYPED_TEST(
		RecordEditorTest,
		AClickOnTheListsLastPixelIsInTheListAndNextInTheFields)
	{
		const int listWidth = this->editor.list_width(frame({}, {}), this->ctx);
		ASSERT_GT(listWidth, EditorLayout::LIST_PAD * 2) << "no list to click";
		this->editor.handle_input(frame({ EditorKey::TAB }, {}), this->ctx);
		ASSERT_EQ(this->editor.editor_focus(), EditorFocus::FIELDS);

		this->editor.handle_input(
			click_at(Vector2D{ listWidth - 1, list_row(2).y }),
			this->ctx);
		EXPECT_EQ(this->editor.editor_focus(), EditorFocus::LIST);
		EXPECT_EQ(this->editor.current_key(), this->keys[2]);

		this->editor.handle_input(
			click_at(Vector2D{ listWidth, field_row(1).y }),
			this->ctx);
		EXPECT_EQ(this->editor.editor_focus(), EditorFocus::FIELDS);
		EXPECT_EQ(this->editor.current_key(), this->keys[2]);
	}

	// A list row ends where its derived height says: a click on its last pixel
	// selects it, and one pixel lower selects the row below.
	TYPED_TEST(
		RecordEditorTest,
		AClickOnAListRowsLastPixelSelectsItAndTheNextTheRowBelow)
	{
		ASSERT_GE(this->keys.size(), 4u) << "too few records for a fourth row";
		const int rowHeight = EditorLayout::row_height(LINE_HEIGHT);
		const int bodyTop = EditorLayout::header_height(LINE_HEIGHT);
		const int lastPixel =
			EditorLayout::row_top(bodyTop, 2, rowHeight) + rowHeight - 1;

		this->editor.handle_input(
			click_at(Vector2D{ EditorLayout::LIST_PAD, lastPixel }),
			this->ctx);
		EXPECT_EQ(this->editor.current_key(), this->keys[2]);
		this->editor.handle_input(
			click_at(Vector2D{ EditorLayout::LIST_PAD, lastPixel + 1 }),
			this->ctx);
		EXPECT_EQ(this->editor.current_key(), this->keys[3]);
	}

	// A field row ends where its derived height says, as a list row does.
	TYPED_TEST(
		RecordEditorTest,
		AClickOnAFieldRowsLastPixelSelectsItAndTheNextTheRowBelow)
	{
		ASSERT_GE(TypeParam::FIELDS.size(), 4u)
			<< "too few fields for a fourth row";
		const int rowHeight = EditorLayout::row_height(LINE_HEIGHT);
		const int bodyTop = EditorLayout::header_height(LINE_HEIGHT);
		const int lastPixel =
			EditorLayout::row_top(bodyTop, 2, rowHeight) + rowHeight - 1;

		this->editor.handle_input(
			click_at(Vector2D{ SCREEN_WIDTH - 40, lastPixel }),
			this->ctx);
		EXPECT_EQ(this->editor.current_field(), TypeParam::FIELDS.at(2).field);
		this->editor.handle_input(
			click_at(Vector2D{ SCREEN_WIDTH - 40, lastPixel + 1 }),
			this->ctx);
		EXPECT_EQ(this->editor.current_field(), TypeParam::FIELDS.at(3).field);
	}

	// Items and monsters save every field of every record, shipped or not, so
	// none of their fields is read-only.
	TYPED_TEST(RecordEditorTest, AnItemOrMonsterHasNoReadOnlyField)
	{
		if constexpr (std::is_same_v<TypeParam, SpellEditorModel>)
		{
			GTEST_SKIP() << "a builtin spell's effect is read-only";
		}
		else
		{
			for (const auto& descriptor : TypeParam::FIELDS)
			{
				EXPECT_FALSE(
					this->editor.record_model().is_read_only(descriptor.field))
					<< descriptor.label;
			}
		}
	}

	// The Class is toggled by Enter alone; the arrows leave it as it is.
	TEST(RecordEditorSpellTest, TheArrowsLeaveTheClassAsItIs)
	{
		using Field = SpellEditorModel::FieldId;
		MockGameContext mock{};
		GameContext ctx = mock.to_game_context();
		SpellEditor editor{};
		editor.enter(ctx);
		editor.handle_input(
			click_at(field_row(row_of<SpellEditorModel>(Field::CLASS))),
			ctx);
		const std::string before =
			editor.record_model().field_value(Field::CLASS);

		editor.handle_input(frame({ EditorKey::RIGHT }, {}), ctx);
		editor.handle_input(
			frame({ EditorKey::LEFT }, { EditorKey::CONTROL }),
			ctx);
		EXPECT_EQ(editor.record_model().field_value(Field::CLASS), before);
		editor.handle_input(frame({ EditorKey::ENTER }, {}), ctx);
		EXPECT_NE(editor.record_model().field_value(Field::CLASS), before);
	}

	// end of file: RecordEditorTest.cpp

} // namespace CppRL
