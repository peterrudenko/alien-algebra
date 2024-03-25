#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <stack>
#include <unordered_map>
#include <unordered_set>

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

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
using HashMap = std::unordered_map<K, V, H, E>;

template <typename K, typename H = std::hash<K>, typename E = std::equal_to<K>>
using HashSet = std::unordered_set<K, H, E>;

template <typename T>
using Optional = std::optional<T>;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

class SteadyClock final
{
public:

    SteadyClock() = default;

    int getMillisecondsElapsed() const
    {
        const auto now = std::chrono::steady_clock::now();
        const auto result = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->startTime).count();
        return result;
    }

private:

    const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
};
