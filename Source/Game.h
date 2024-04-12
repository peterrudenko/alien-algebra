#pragma once

#include "Common.h"
#include "Random.h"
#include "Seed.h"
#include "Parser.h"
#include "QuestGenerator.h"
#include "EGraph.h"

class Game
{
public:

    Game() = default;
    virtual ~Game() = default;

    struct Stats
    {
        String seed;

        //Vector<Symbol> terms;
        //Vector<Symbol> operations;

        String internalState;

        int numRootClasses = 0;
        int numLevels = 0;

        int numGenerationAttempts = 0;
        int generationTimeMs = 0;
    };

    virtual void onShowLevelInfo(const Stats &stats) = 0;
    virtual void onShowHint(const String &hint) = 0;
    virtual void onShowQuestion(const String &question) = 0;
    virtual void onShowError(const String &error) = 0;
    virtual void onValidateAnswer(bool isValid, int score) = 0;

protected:

    void generate(const Seed &seed)
    {
        this->random.init(seed.seed);

        Stats stats;
        stats.seed = seed.encode();
        //const auto testSeedDecoding = Seed(stats.seed);
        //const auto testSeedEncoding = testSeedDecoding.encode();

        SteadyClock timer;
        bool hasResult = false;
        int numAttempts = 0;

        while (!hasResult)
        {
            this->eGraph = {};
            this->levels = {};

            QuestGenerator generator(this->eGraph, this->random);
            hasResult = generator.tryGenerate(levels);

            numAttempts++;
        }

        stats.numGenerationAttempts = numAttempts;
        stats.generationTimeMs = timer.getMillisecondsElapsed();
        stats.numRootClasses = this->eGraph.classes.size();
        stats.numLevels = levels.size();

        //stats.internalState = Serialization::encode(Serialization::serialize(this->eGraph));

        this->onShowLevelInfo(stats);

        this->showCurrentLevel();
    }

    void showCurrentLevel()
    {
        const auto &currentLevel = this->levels[this->currentLevelNumber];
        this->onShowHint(currentLevel.hintLeftHand->formatted + String(" = ") + currentLevel.hintRightHand->formatted);
        this->onShowQuestion(currentLevel.question->formatted);
    }

    void validateAnswer(const String &expression)
    {
        // debug mode
        const auto &currentLevel = this->levels[this->currentLevelNumber];

        if (expression.empty())
        {
            this->onShowHint("-----------");

            for (const auto &answer : currentLevel.answers)
            {
                this->onShowHint(answer->formatted);
            }

            this->currentLevelNumber++;
            if (this->currentLevelNumber < this->levels.size())
            {
                this->showCurrentLevel();
            }
            else
            {
                //assert(false); // todo win condition
            }

            return;
        }

        try
        {
            const auto pattern = Parser::makePattern(expression);
            if (!pattern.term)
            {
                // todo what?
                return;
            }

            const auto formattedAnswer = Parser::formatPatternTerm(*pattern.term, false);
            if (contains(this->acceptedAnswers, formattedAnswer))
            {
                // todo give a hint?
                return;
            }

            bool isValidAnswer = false;
            for (const auto &[classId, _classPtr] : this->eGraph.classes)
            {
                isValidAnswer = isValidAnswer ||
                    (classId == currentLevel.question->rootId && this->matchPatternTerm(*pattern.term, classId));
            }

            if (isValidAnswer)
            {
                //this->acceptedAnswers.insert(formattedAnswer);
                this->score += this->validAnswerScore;
                this->onValidateAnswer(isValidAnswer, this->score);

                this->currentLevelNumber++;
                if (this->currentLevelNumber < this->levels.size())
                {
                    this->showCurrentLevel();
                }
                else
                {
                    assert(false); // todo win condition
                }

                return;
            }

            this->score += this->failedAttemptScore;
            this->onValidateAnswer(isValidAnswer, this->score);

            // todo not a valid answer, try to give a hint?
            /*
            for (const auto &[classId, _classPtr] : this->eGraph.classes)
            {
                if (!this->matchPatternTerm(patternTerm, classId))
                {
                    continue;
                }

                if (!contains(this->hints, classId))
                {
                    continue;
                }

                const auto displayedHints = this->hints[classId];
                if (auto otherHint = pickHintPair(formattedAnswer, displayedHints))
                {
                    this->onShowHint(formattedAnswer + " = " + (*otherHint)->formatted);
                }
            }
            */
        }
        catch (const std::exception &e)
        {
            this->onShowError(e.what()); // todo something more sensible
        }
    }

private:

    static Optional<Hint::Ptr> pickHintPair(const String &targetFormatted, const Vector<Hint::Ptr> &allHints)
    {
        // fixme: same logic as in pickHintRightHandSide
        for (const auto &otherHint : allHints)
        {
            if (otherHint->formatted == targetFormatted)
            {
                continue;
            }

            return otherHint;
        }

        return {};
    }

    bool matchPatternTerm(const PatternTerm &patternTerm, ClassId classId)
    {
        const auto rootId = this->eGraph.find(classId);
        assert(contains(this->eGraph.classes, rootId));

        for (const auto &term : this->eGraph.classes.at(rootId)->terms)
        {
            if (term->name != patternTerm.name || term->childrenIds.size() != patternTerm.arguments.size())
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

    int score = 0;
    const int failedAttemptScore = -1;
    const int validAnswerScore = 3;
    HashSet<String> acceptedAnswers;
};
