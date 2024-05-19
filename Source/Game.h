#pragma once

#include "Common.h"
#include "Random.h"
#include "Parser.h"
#include "QuestGenerator.h"
#include "EGraph.h"

class Game
{
public:

    Game() = default;
    virtual ~Game() = default;

    virtual void onStartGame() = 0;

    virtual void onStartLevel(int levelNumber,
        const Vector<String> &hints, const String &question,
        const Vector<String> &suggestions) = 0;

    virtual void onEndLevel(bool passed,
        const Vector<bool> &answerIndices) = 0;

    virtual void onEndGame(bool win) = 0;

    void validateAnswer(int suggestionIndex)
    {
        bool isValidPick = false;
        Vector<bool> answersIndices;
        const auto &currentLevel = this->getCurrentLevel();

        for (int i = 0; i < currentLevel.suggestions.size(); ++i)
        {
            const auto suggestion = currentLevel.suggestions[i];
            const bool suggestionIsValidAnswer = this->isValidAnswer(suggestion);
            
            isValidPick = isValidPick ||
                (suggestionIndex == i && suggestionIsValidAnswer);

            answersIndices[i] = suggestionIsValidAnswer;
        }

        this->onEndLevel(isValidPick, answersIndices);

        if (isValidPick)
        {
            this->proceedToLevel(this->currentLevelNumber + 1);
        }
        else
        {
            this->onEndGame(false);
        }
    }

    void validateAnswer(const String &expression)
    {
        const bool isValidAnswer = this->isValidAnswer(expression);
        this->onEndLevel(isValidAnswer, {});

        if (isValidAnswer)
        {
            this->proceedToLevel(this->currentLevelNumber + 1);
        }
        else
        {
            this->onEndGame(false);
        }
    }

    bool isValidAnswer(const String &expression) const
    {
        try
        {
            const auto pattern = Parser::makePattern(expression);
            if (!pattern.term)
            {
                return false;
            }

            // shouldn't accept the question itself as an answer:
            const auto formattedAnswer = Parser::formatPatternTerm(*pattern.term, false);
            if (this->getCurrentLevel().question->formatted == formattedAnswer) // todo should compare ASTs here instead but whatever
            {
                return false;
            }

            bool isValidAnswer = false;
            for (const auto &[classId, _classPtr] : this->eGraph.classes)
            {
                isValidAnswer = isValidAnswer ||
                    (classId == this->getCurrentLevel().question->rootId &&
                        this->matchPatternTerm(*pattern.term, classId));
            }

            return isValidAnswer;
        }
        catch (...) {}

        return false;
    }

protected:

    const Level &getCurrentLevel() const
    {
        assert(this->currentLevelNumber >= 0 && this->currentLevelNumber < this->levels.size());
        return this->levels[this->currentLevelNumber];
    }

    void proceedToLevel(int levelNumber)
    {
        this->currentLevelNumber = levelNumber;
        if (this->currentLevelNumber < this->levels.size())
        {
            const auto &currentLevel = this->getCurrentLevel();
            this->onStartLevel(this->currentLevelNumber,
                {currentLevel.getFormattedHint()},
                currentLevel.question->formatted + " " + Symbols::equalsSign,
                currentLevel.suggestions);
        }
        else
        {
            this->onEndGame(true);
        }
    }

    void generate()
    {
        bool hasResult = false;
        int numAttempts = 0;

        while (!hasResult)
        {
            this->eGraph = {};
            this->levels = {};

            QuestGenerator generator(this->eGraph, this->random);
            hasResult = generator.tryGenerate(levels);

            numAttempts++;
            assert(numAttempts < 10); // probably stuck forever
        }

        this->onStartGame();
        this->proceedToLevel(0);
    }

private:

    bool matchPatternTerm(const PatternTerm &patternTerm, ClassId classId) const
    {
        const auto rootId = this->eGraph.find(classId);
        assert(contains(this->eGraph.classes, rootId));

        for (const auto &term : this->eGraph.classes.at(rootId)->terms)
        {
            if (term->name != patternTerm.name ||
                term->childrenIds.size() != patternTerm.arguments.size())
            {
                continue;
            }

            bool childrenMatch = true;
            for (int i = 0; i < patternTerm.arguments.size(); ++i)
            {
                assert(patternTerm.arguments[i].term.get() != nullptr);
                childrenMatch = childrenMatch &&
                    this->matchPatternTerm(*patternTerm.arguments[i].term, term->childrenIds[i]);
            }

            if (childrenMatch)
            {
                return true;
            }
        }

        return false;
    }

private:

    Vector<Level> levels;

    int currentLevelNumber = 0;

    e::Graph eGraph;

    Random random;
};
