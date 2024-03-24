#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <set>

using String = std::string;

template <typename T>
using UniquePointer = std::unique_ptr<T>;

template <typename T>
using SharedPointer = std::shared_ptr<T>;

template <typename T, typename... Args>
inline UniquePointer<T> make(Args &&...args)
{
    return UniquePointer<T>(new T(std::forward<Args>(args)...));
}

using std::move;

template <typename T>
using Vector = std::vector<T>;

template <typename T>
using Stack = std::stack<T>;

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
using HashMap = std::unordered_map<K, V, H, E>;

template <typename K, typename H = std::hash<K>, typename E = std::equal_to<K>>
using HashSet = std::unordered_set<K, H, E>;

template <typename T>
using Optional = std::optional<T>;
using std::nullopt;
