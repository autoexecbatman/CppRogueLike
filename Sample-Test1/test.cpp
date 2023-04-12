#include "pch.h"

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <curses.h>

#include "../UT_PDCurses_input/main_UT_PDCurses_input.cpp"

// Define the test fixture
class CursesTest : public ::testing::Test {
protected:
    // Set up the fixture
    virtual void SetUp() {
        // initialize the curses library
        initscr();

        // disable line buffering
        cbreak();

        // Redirect output to a stringstream
        std::cout.rdbuf(output_stream_.rdbuf());
    }

    // Tear down the fixture
    virtual void TearDown() {
        // clean up the curses library
        endwin();

        // Restore old output stream
        std::cout.rdbuf(old_cout_buf_);
    }

    // Declare variables used in the fixture
    std::stringstream output_stream_;
    std::streambuf* old_cout_buf_;
};

// Define the test case
TEST_F(CursesTest, TestInputAndOutput) {
    // Prepare input data
    std::string input = "Alice\n";
    std::stringstream input_stream(input);

    // Redirect input to use stringstream
    std::streambuf* old_cin_buf = std::cin.rdbuf(input_stream.rdbuf());

    // Call the function to be tested
    test();

    // Restore old input stream
    std::cin.rdbuf(old_cin_buf);

    // Check the output
    std::string expected_output = "Enter your name: Hello, Alice!";
    std::string actual_output = output_stream_.str();
    EXPECT_EQ(expected_output, actual_output);
}

//// Define the main function
//int main(int argc, char** argv) {
//    ::testing::InitGoogleTest(&argc, argv);
//    return RUN_ALL_TESTS();
//}