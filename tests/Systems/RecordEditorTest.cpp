// file: RecordEditorTest.cpp
// The shared editor frame, driven through EditorInput snapshots once for each
// editor's Model: a click selects a list row, Tab and the arrows move the
// cursors, Ctrl steps a number by ten, Enter types into a text field, A and
// Delete add and remove a custom record, and Esc commits and leaves. Every
// expected value is read from the Model's own field table and bounds.
//
// No test presses Ctrl+S: saving writes the real data files.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RecordEditorTest*

#include <gtest/gtest.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "src/EditorField.h"
#include "src/EditorInput.h"
#include "src/ItemEditor.h"
#include "src/MonsterEditor.h"
#include "src/RecordEditor.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// The height the tests give the window: tall enough that every row a test
// reaches is on screen.
constexpr int SCREEN_HEIGHT = 900;

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
	input.screenWidth = 1280;
	input.screenHeight = SCREEN_HEIGHT;
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
		EditorLayout::HEADER_HEIGHT +
			static_cast<int>(row) * EditorLayout::ITEM_HEIGHT +
			EditorLayout::ITEM_HEIGHT / 2,
	};
}

// The middle of field row n, counted from the top of the field panel.
Vector2D field_row(size_t row)
{
	return Vector2D{
		EditorLayout::LIST_WIDTH + 40,
		EditorLayout::HEADER_HEIGHT +
			static_cast<int>(row) * EditorLayout::FIELD_HEIGHT +
			EditorLayout::FIELD_HEIGHT / 2,
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
	const auto found = std::ranges::find_if(Model::FIELDS, is_plain_number);
	if (found == Model::FIELDS.end())
	{
		return std::nullopt;
	}
	return static_cast<size_t>(found - Model::FIELDS.begin());
}

// Esc after a Ctrl+Right leaves the editor and opens the main menu, and the
// record it held is committed, so the editor opened again shows the stepped
// number. Runs in a HiddenRenderer child.
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
	const std::string edited = editor.record_model().field_value(fieldId);

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
		return editor.record_model().field_value(Model::FIELDS.at(field).field);
	}

	MockGameContext mock{};
	GameContext ctx{};
	RecordEditor<Model> editor{};
	std::vector<std::string> keys{};
};

using EditorModels = ::testing::Types<ItemEditorModel, MonsterEditorModel>;
TYPED_TEST_SUITE(RecordEditorTest, EditorModels);

TYPED_TEST(RecordEditorTest, AClickOnAListRowSelectsThatRecord)
{
	this->editor.handle_input(click_at(list_row(2)), this->ctx);

	EXPECT_EQ(this->editor.current_key(), this->keys[2]);
	EXPECT_EQ(this->editor.editor_focus(), EditorFocus::LIST);
}

// The working copy shown in the fields is the clicked record, as the registry
// holds it.
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

// Above the list the row count is negative, which would round to the first row.
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

TYPED_TEST(RecordEditorTest, ACtrlArrowStepsAPlainNumberByTenAndABareOneByOne)
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
	this->editor.handle_input(frame({ EditorKey::BACKSPACE }, {}), this->ctx);
	this->editor.handle_input(frame({ EditorKey::ENTER }, {}), this->ctx);

	EXPECT_EQ(this->editor.editor_mode(), EditorMode::NORMAL);
	EXPECT_EQ(this->shown(*field), before + "Q");
}

TYPED_TEST(RecordEditorTest, EscapeCommitsTheRecordAndLeaves)
{
	// Leaving opens the main menu, which needs a window, so it runs in a child.
	const int childExitCode =
		exit_code_in_child(&escape_commits_and_leaves<TypeParam>);
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

// Shipped records come first and custom ones after, so the line between them
// is drawn once, above the first custom record, however many follow it. A
// Model may ship custom records of its own, so the rows are read from it.
TYPED_TEST(RecordEditorTest, CustomRecordsAreListedLastWithOneDividerAboveThem)
{
	auto divider_rows = [this]()
	{
		std::vector<size_t> rows{};
		const size_t count = this->editor.record_model().keys(this->ctx).size();
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

// The XP field edits one hit dice count of the selected monster. Choosing
// another monster selects its first count, so a mimic's second count is not
// carried to a goblin that has one.
TEST(RecordEditorMonsterTest, ChoosingAnotherMonsterSelectsItsFirstHitDiceCount)
{
	using Field = MonsterEditorModel::FieldId;
	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	MonsterEditor editor{};
	editor.enter(ctx);
	const std::vector<std::string> keys = editor.record_model().keys(ctx);
	const auto mimic = std::ranges::find(keys, "mimic");
	ASSERT_NE(mimic, keys.end()) << "the shipped monsters have no mimic";
	const auto row_of = [](Field field)
	{
		const auto is_field = [field](const auto& descriptor)
		{
			return descriptor.field == field;
		};
		return static_cast<size_t>(
			std::ranges::find_if(MonsterEditorModel::FIELDS, is_field) -
			MonsterEditorModel::FIELDS.begin());
	};
	editor.handle_input(
		click_at(list_row(static_cast<size_t>(mimic - keys.begin()))),
		ctx);
	editor.handle_input(click_at(field_row(row_of(Field::XP_COUNT))), ctx);
	editor.handle_input(frame({ EditorKey::RIGHT }, {}), ctx);
	ASSERT_EQ(editor.record_model().field_value(Field::XP_COUNT), "10 HD");

	editor.handle_input(click_at(list_row(0)), ctx);

	EXPECT_EQ(editor.current_key(), "goblin");
	EXPECT_EQ(editor.record_model().field_value(Field::XP_COUNT), "1 HD");
	EXPECT_EQ(editor.record_model().field_value(Field::XP), "105");
}

// end of file: RecordEditorTest.cpp
