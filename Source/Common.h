#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <unordered_set>

using String = std::string;

template <typename T>
using Vector = std::vector<T>;

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
using HashMap = std::unordered_map<K, V, H, E>;

template <typename K, typename H = std::hash<K>, typename E = std::equal_to<K>>
using HashSet = std::unordered_set<K, H, E>;

template <typename T>
using Optional = std::optional<T>;

using std::move;

template <typename C, typename K>
inline bool contains(const C &container, const K &key)
{
    return container.find(key) != container.end();
}

template <typename T>
inline void append(Vector<T> &v1, const Vector<T> &v2)
{
    v1.insert(v1.end(), v2.begin(), v2.end());
}

template <typename T>
inline void append(HashSet<T> &s1, const HashSet<T> &s2)
{
    s1.insert(s2.begin(), s2.end());
}

namespace Symbols
{
#if WEB_CLIENT

    static const String openingBracket = "\xe2\xb8\x84"; // ⸄
    static const String closingBracket = "\xe2\xb8\x85"; // ⸅
    static const String equalsSign = "\xe2\x89\x91"; // ≑

#else

    static const String openingBracket = "(";
    static const String closingBracket = ")";
    static const String equalsSign = "=";

#endif
}
