#pragma once

// Constructs a visitor from a set of per-type handler lambdas for use with std::visit.
//
// Example:
//     std::visit(
//         VariantVisitor{
//             []([[maybe_unused]] Shield& shield) { return -1; },
//             [](Armor& armor) { return armor.armorClass; },
//             []([[maybe_unused]] auto& otherBehavior) { return 0; }
//         },
//         behavior);

template <typename... Handlers>
struct VariantVisitor : Handlers...
{
	using Handlers::operator()...;
};

template <typename... Handlers>
VariantVisitor(Handlers...) -> VariantVisitor<Handlers...>;
