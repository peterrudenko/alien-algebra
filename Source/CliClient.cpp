
#include "Common.h"
#include "Game.h"
#include <iostream>
#include <ostream>

class CliClient final : public Game
{
public:

    void onQuestGenerationDone(const Game::Stats &stats) override
    {
        std::cout << "Seed: " << stats.seed << std::endl;
        //std::cout << "State: " << stats.internalState << std::endl;
        std::cout << "===========" << std::endl;
        std::cout << "E-graph classes: " << stats.numRootClasses << std::endl;
        std::cout << "Num levels: " << stats.numLevels << std::endl;
        std::cout << "Generation: " << stats.numGenerationAttempts << std::endl;
        std::cout << "Gen time: " << stats.generationTimeMs << "ms" << std::endl;
        std::cout << "-----------" << std::endl;

        /*
        std::cout << "Terms: ";
        for (const auto &s : level.stats.terms) { std::cout << s << " "; }
        std::cout << std::endl;

        std::cout << "Operations: ";
        for (const auto &s : level.stats.operations) { std::cout << s << " "; }
        std::cout << std::endl;
        */

        std::cout << std::endl;
    }

    void onStartNewLevel(int levelNumber, const Vector<String> &hints) override
    {
        std::cout << "=========== (" << std::to_string(levelNumber) + ")" << std::endl;

        for (const auto &hint : hints)
        {
            std::cout << hint << std::endl;
        }
    }

    void onShowQuestion(const String &question, const Vector<String> &suggestions) override
    {
        this->answersToChooseFrom = suggestions;

        std::cout << question << std::endl;
        std::cout << "---" << std::endl;

        for (int i = 0; i < suggestions.size(); ++i)
        {
            std::cout << std::to_string(i) << ": " << suggestions[i] << std::endl;
        }
    }

    void onShowError(const String &error) override
    {
        std::cout << error << std::endl;
    }

    void onValidateAnswer(bool isValid, int score) override
    {
        std::cout << (isValid ? "Korrekt!" : "Oh noes.") << " Score: " << std::to_string(score) << std::endl;
    }

    void run()
    {
        Seed seed;
        this->generate(seed);

        while(!this->shouldStop)
        {
            String input;
            std::getline(std::cin, input);

            if (this->answersToChooseFrom.empty())
            {
                this->validateAnswer(input);
            }
            else
            {
                const auto choice = std::atoi(input.c_str());
                if (choice < (this->answersToChooseFrom.size() - 1))
                {
                    this->validateAnswer(this->answersToChooseFrom[choice]);
                }

                this->answersToChooseFrom.clear();
            }
        }
    }

    bool shouldStop = false;
    Vector<String> answersToChooseFrom;
};

int main(int argc, char **argv)
{
    CliClient game;
    game.run(); // run.game.run();
}
