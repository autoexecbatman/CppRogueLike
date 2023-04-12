#include "pch.h"
#include "CppUnitTest.h"
#include <curses.h>
#include <iostream>
#include <sstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
    TEST_CLASS(UnitTest1)
    {
    public:

        TEST_METHOD(TestInputAndOutput)
        {

            // Prepare input data
            std::string input = "Alice\n";
            std::stringstream input_stream(input);

            // Redirect input and output to use stringstream
            std::streambuf* old_cin_buf = std::cin.rdbuf(input_stream.rdbuf());
            std::streambuf* old_cout_buf = std::cout.rdbuf();

            // Redirect output to a stringstream
            std::stringstream output_stream;
            std::cout.rdbuf(output_stream.rdbuf());

            // Call the function to be tested
            initscr();  // initialize the curses library
            cbreak();   // disable line buffering

            printw("Enter your name: ");
            char name[50];
            getnstr(name, sizeof(name));  // get user input

            printw("Hello, %s!", name);
            refresh();  // redraw the screen

            getch();    // wait for user input before exiting
            endwin();   // clean up the curses library

            // Restore old input and output streams
            std::cin.rdbuf(old_cin_buf);
            std::cout.rdbuf(old_cout_buf);

            // Check the output
            std::string expected_output = "Enter your name: Hello, Alice!";
            std::string actual_output = output_stream.str();
            Assert::AreEqual(expected_output, actual_output);
        }
    };
}


//#include "pch.h"
//#include "CppUnitTest.h"
//#include <curses.h>
//#include <iostream>
//#include <sstream>
//#include "../UT_PDCurses_input/main_UT_PDCurses_input.h"
//
//using namespace Microsoft::VisualStudio::CppUnitTestFramework;
//
//namespace UnitTest1
//{
//    TEST_CLASS(UnitTest1)
//    {
//    public:
//
//        TEST_METHOD(TestInputAndOutput)
//        {
//
//            // Prepare input data
//            std::string input = "Alice\n";
//            std::stringstream input_stream(input);
//
//            // Redirect input and output to use stringstream
//            std::streambuf* old_cin_buf = std::cin.rdbuf(input_stream.rdbuf());
//            std::streambuf* old_cout_buf = std::cout.rdbuf();
//
//            // Redirect output to a stringstream
//            std::stringstream output_stream;
//            std::cout.rdbuf(output_stream.rdbuf());
//
//            // Call the function to be tested
//            test();
//
//            // Restore old input and output streams
//            std::cin.rdbuf(old_cin_buf);
//            std::cout.rdbuf(old_cout_buf);
//
//            // Check the output
//            std::string expected_output = "Enter your name: Hello, Alice!";
//            std::string actual_output = output_stream.str();
//            Assert::AreEqual(expected_output, actual_output);
//        }
//    };
//}
