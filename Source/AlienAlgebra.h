#pragma once

#include "Common.h"

// Looking up all possible properties here in the source code
// should not be considered cheating, the game is expecting
// that you should consider doing it at some point:

struct OperationProperty final
{
    const String rewriteTemplate;
    const HashSet<int> levels;

    friend bool operator==(const OperationProperty &l, const OperationProperty &r)
    {
        return l.rewriteTemplate == r.rewriteTemplate;
    }
};

namespace std
{
    template <>
    struct hash<OperationProperty>
    {
        auto operator()(const OperationProperty &x) const
        {
            return hash<String>()(x.rewriteTemplate);
        }
    };
} // namespace std

namespace AlienAlgebra
{
    // most of these properties are just made up
    static HashSet<OperationProperty> allProperties =
    {
        // the first level is intended to be introductory
        {"$x . $x => $x", {0}}, // idempotence
        {"$x . $y => $x", {0}}, // right identity-like
        {"$x . $y => $y", {0}}, // left identity-like
        {"$x . $y => $x . $x", {0}},
        {"$x . $y => $y . $y", {0}},
        {"$x . $y => $y . $x", {0, 1}}, // commutativity

        // associativity with variations
        {"($x . $y) . $z => $x . ($y . $z)", {1}},
        {"$x . ($y . $z) => ($x . $y) . $z", {1}},
        {"($x . $y) . $z => $z . ($y . $x)", {1, 3}},
        {"$x . ($y . $z) => ($z . $y) . $x", {1, 3}},
        {"($x . $y) . $z => $z . ($x . $y)", {1, 3}},
        {"$x . ($y . $z) => ($y . $z) . $x", {1, 3}},
        {"$x . ($y . $z) => ($z . $x) . $y", {1, 3}},
        {"$x . ($y . $z) => ($x . $z) . $y", {1, 3}},
        {"($x . $y) . $z => $y . ($z . $x)", {1, 3}},
        {"($x . $y) . $z => $y . ($x . $z)", {1, 3}},

        // more identity-like
        {"($x . $y) . $z => $x . $y", {2}},
        {"$x . ($y . $z) => $x . $y", {2}},
        {"($x . $y) . $z => $y . $x", {2}},
        {"$x . ($y . $z) => $y . $x", {2}},
        {"($x . $y) . $z => $x . $z", {2}},
        {"$x . ($y . $z) => $x . $z", {2}},
        {"($x . $y) . $z => $z . $x", {2}},
        {"$x . ($y . $z) => $z . $x", {2}},
        {"($x . $y) . $z => $y . $z", {2}},
        {"$x . ($y . $z) => $y . $z", {2}},
        {"($x . $y) . $z => $z . $y", {2}},
        {"$x . ($y . $z) => $z . $y", {2}},
        {"($x . $y) . $z => $x . $x", {2}},
        {"$x . ($y . $z) => $x . $x", {2}},
        {"($x . $y) . $z => $y . $y", {2}},
        {"$x . ($y . $z) => $y . $y", {2}},
        {"($x . $y) . $z => $z . $z", {2}},
        {"$x . ($y . $z) => $z . $z", {2}},

        // weird stuff
        {"($x . $y) . $z => ($x . $y) . ($y . $x)", {2, 3}},
        {"$x . ($y . $z) => ($y . $z) . ($z . $y)", {2, 3}},
        {"($x . $y) . $z => ($y . $x) . ($x . $y)", {2, 3}},
        {"$x . ($y . $z) => ($z . $y) . ($y . $z)", {2, 3}},
        {"($x . $y) . $z => ($x . $y) . ($x . $x)", {2, 3}},
        {"$x . ($y . $z) => ($y . $z) . ($y . $y)", {2, 3}},
        {"($x . $y) . $z => ($y . $x) . ($x . $x)", {2, 3}},
        {"$x . ($y . $z) => ($z . $y) . ($y . $y)", {2, 3}},
        {"($x . $y) . $z => ($x . $y) . ($y . $y)", {2, 3}},
        {"$x . ($y . $z) => ($y . $z) . ($z . $z)", {2, 3}},
        {"($x . $y) . $z => ($y . $x) . ($y . $y)", {2, 3}},
        {"$x . ($y . $z) => ($z . $y) . ($z . $z)", {2, 3}},
        {"($x . $y) . $z => ($x . $x) . ($x . $y)", {2, 3}},
        {"$x . ($y . $z) => ($y . $y) . ($y . $z)", {2, 3}},
        {"($x . $y) . $z => ($x . $x) . ($y . $x)", {2, 3}},
        {"$x . ($y . $z) => ($y . $y) . ($z . $y)", {2, 3}},
        {"($x . $y) . $z => ($y . $y) . ($x . $y)", {2, 3}},
        {"$x . ($y . $z) => ($z . $z) . ($y . $z)", {2, 3}},
        {"($x . $y) . $z => ($y . $y) . ($y . $x)", {2, 3}},
        {"$x . ($y . $z) => ($z . $z) . ($z . $y)", {2, 3}},

        // warning: when adding new properties make sure properties make sure
        // they will not cause an infinite loop of rewriting, e.g. "x . y => (y . x) . x",
        // or "$x . ($y . $z) => ($x . $y) . ($y . $x)"
    };
} // namespace AlienAlgebra
