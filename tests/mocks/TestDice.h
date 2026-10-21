#pragma once
// file: TestDice.h
// The one seed every test's dice start from, so a test draws the same numbers on every run
// and a mutation sweep's verdict does not depend on which run it was.
//
// RandomDice and Game have no default constructor, so every construction names its seed:
//
//     #include "tests/mocks/TestDice.h"
//     RandomDice dice{ TEST_DICE_SEED };   // the same sequence on every run
//     Game game{ TEST_DICE_SEED };         // its ctx.dice draws that sequence too
//
// A test that wants many samples - forty dungeons, say - names its own seeds and builds
// RandomDice{ seed } for each, so a failure can say which one. The value is chosen once and is
// not tuned to make a test pass.

inline constexpr unsigned int TEST_DICE_SEED = 20261021;

// end of file: TestDice.h
