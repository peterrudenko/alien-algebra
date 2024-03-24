#pragma once

#include "Common.h"
#include <random>

class Random final
{
public:

    // todo generate seed

    template <typename T>
    T pickRandomElement(const Vector<T> &origin)
    {
        assert(!origin.empty());
        return this->pickRandomElements(origin, 1).front();
    }

    template <typename T>
    Vector<T> pickRandomElements(const Vector<T> &origin, int numElements = 1)
    {
        assert(numElements <= origin.size());

        HashSet<T> set;
        while (set.size() != numElements)
        {
            const auto element = origin[this->getRandomInt(origin.size() - 1)];
            set.insert(element);
        }

        Vector<T> result;
        result.insert(result.begin(), set.begin(), set.end());
        return result;
    }

    int getRandomInt(int max)
    {
        std::uniform_int_distribution<int> distribution(0, max);
        return distribution(this->rng);
    }

    bool getRandomBool()
    {
        return this->getRandomInt(1) == 1;
    }

private:

    std::mt19937 rng;
};
