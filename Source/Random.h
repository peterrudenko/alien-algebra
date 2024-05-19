#pragma once

#include "Common.h"
#include <random>

class Random final
{
public:

    Random() = default;

    bool rollD2()
    {
        return this->getRandomInt(1, 2) == 1;
    }

    bool rollD5()
    {
        return this->getRandomInt(1, 5) == 1;
    }

    template <typename T>
    void shuffle(Vector<T> &origin)
    {
        for (unsigned i = 0; i < origin.size() - 1; i++)
        {
            const auto randomIndex = this->getRandomInt(i, int(origin.size()) - 1);
            std::swap(origin[i], origin[randomIndex]);
        }
    }

    template <typename T>
    T pickOne(const HashSet<T> &origin)
    {
        assert(!origin.empty());
        return *std::next(origin.begin(), this->getRandomInt(0, int(origin.size()) - 1));
    }

    template <typename T>
    T pickOne(const Vector<T> &origin)
    {
        assert(!origin.empty());
        return origin[this->getRandomInt(0, int(origin.size()) - 1)];
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
            if (!contains(used, element))
            {
                used.insert(element);
                return element;
            }
        };
    }

    template <typename T>
    T pickOneUnique(const Vector<T> &origin, HashSet<int> &usedIndices)
    {
        assert(!origin.empty());
        if (origin.size() <= usedIndices.size())
        {
            //assert(false); even if this happens, make sure to return something
            return this->pickOne(origin);
        }
        while (true)
        {
            const auto elementIndex = this->getRandomInt(0, int(origin.size()) - 1);
            if (!contains(usedIndices, elementIndex))
            {
                usedIndices.insert(elementIndex);
                return origin[elementIndex];
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
            const auto element = origin[this->getRandomInt(0, int(origin.size()) - 1)];
            if (!contains(used, element))
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

    std::mt19937 rng = std::mt19937(std::random_device{}());
};
