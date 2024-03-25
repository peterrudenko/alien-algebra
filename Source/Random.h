#pragma once

#include "Common.h"
#include <random>

class Random final
{
public:

    Random() = default;

    void init(int seed)
    {
        this->rng = std::mt19937(seed);
    }

    template <typename T>
    T pickOne(const HashSet<T> &origin)
    {
        assert(!origin.empty());
        return *std::next(origin.begin(), this->getRandomInt(0, origin.size() - 1));
    }

    template <typename T>
    T pickOneUnique(const HashSet<T> &origin, HashSet<T> &used)
    {
        assert(!origin.empty());
        if (origin.size() <= used.size())
        {
            //assert(false); even if this happens, make sure to return something
            return this->pickOne(origin);
        }
        while (true)
        {
            const auto element = this->pickOne(origin);
            if (!used.contains(element))
            {
                used.insert(element);
                return element;
            }
        };
    }

    template <typename T>
    Vector<T> pickUnique(const Vector<T> &origin, HashSet<T> &used, int numElements = 1)
    {
        assert(used.size() <= origin.size());
        assert(numElements <= (origin.size() - used.size()));

        HashSet<T> set;
        while (set.size() != numElements)
        {
            const auto element = origin[this->getRandomInt(0, origin.size() - 1)];
            if (!used.contains(element))
            {
                set.insert(element);
                used.insert(element);
            }
        }

        Vector<T> result;
        result.insert(result.begin(), set.begin(), set.end());
        return result;
    }

    template <typename T>
    Vector<T> pickUnique(const Vector<T> &origin, const HashSet<T> &used, int numElements = 1)
    {
        HashSet<T> tempUsed(used);
        return this->pickUnique(origin, tempUsed, numElements);
    }

    int getRandomInt(int min, int max)
    {
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(this->rng);
    }

private:

    std::mt19937 rng;
};
