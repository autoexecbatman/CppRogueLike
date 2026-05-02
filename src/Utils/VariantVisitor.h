#pragma once

/// Constructs a visitor from a set of per-type handler lambdas for use with std::visit.
///
/// Example:
///     std::visit(
///         VariantVisitor{
///             [](Shield&)  { return -1; },
///             [](Armor& a) { return a.armorClass; },
///             [](auto&)    { return 0; }
///         },
///         behavior);

template <typename... Handlers>
struct VariantVisitor : Handlers...
{
    using Handlers::operator()...;
};

template <typename... Handlers>
VariantVisitor(Handlers...) -> VariantVisitor<Handlers...>;
