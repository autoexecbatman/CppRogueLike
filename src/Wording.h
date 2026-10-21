// file: Wording.h
// How a message puts a count beside a noun, so "1 item" and "2 items" are decided in one
// place rather than by a ternary at every message that counts something.
//
// Usage, from any message that reports a number of things:
//   Wording::counted(1, "item", "items");         // -> "1 item"
//   Wording::counted(3, "creature", "creatures"); // -> "3 creatures"
//   Wording::counted(0, "item", "items");         // -> "0 items", English takes the plural for none
// The plural is passed in rather than built by adding an "s", so "foot" and "feet" work too.
// A negative count is refused by an assertion: nothing in the game counts below none.
//
// Usage, from any message that says how long an effect lasts:
//   Wording::rounds(3);                           // -> "3 rounds"
#pragma once

#include <string>
#include <string_view>

namespace Wording
{

// The count and the noun that agrees with it: the singular for exactly one, the plural
// for any other count.
//
// Example:
//   counted(1, "item", "items"); // -> "1 item"
//   counted(2, "item", "items"); // -> "2 items"
[[nodiscard]] std::string counted(int count, std::string_view singular, std::string_view plural);

// How long something lasts, in rounds, the unit every effect's duration is kept in. Every
// message that states a duration words it here, so the unit is decided once.
// A negative count is refused, as counted refuses one.
//
// Example:
//   rounds(1); // -> "1 round"
//   rounds(3); // -> "3 rounds"
[[nodiscard]] std::string rounds(int count);

} // namespace Wording
