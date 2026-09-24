#include <gtest/gtest.h>

#include <cstdio>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

int main(int argc, char** argv)
{
#ifdef _MSC_VER
    // A failed assert reports to stderr and aborts. The default is a dialog,
    // which in a headless run is a hang with no output.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif
    // Unbuffered, so the log ends where the process died. Redirected to a file or a CI
    // step, stdout is block-buffered, and a kill that never unwinds - an abort, a
    // heap-corruption fastfail - takes the buffer with it, leaving the last line printed
    // a flush boundary somewhere before the fault.
    setvbuf(stdout, nullptr, _IONBF, 0);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
