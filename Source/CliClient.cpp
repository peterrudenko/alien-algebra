
#include "Common.h"
#include "Game.h"
#include <iostream>
#include <ostream>

class CliClient final : public Game
{
public:

    void onShowLevelInfo(const Game::Stats &stats) override
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

    void onShowHint(const String &hint) override
    {
        std::cout << hint << std::endl;
    }

    void onShowQuestion(const String &question) override
    {
        std::cout << std::endl;
        std::cout << question << " = ?" << std::endl;
    }

    void onShowError(const String &error) override
    {
        std::cout << "!" << std::endl;
        std::cout << error << std::endl;
    }

    void onValidateAnswer(bool isValid, int score) override
    {
        std::cout << (isValid ? "Korrekt!" : "Oh noes.") << " Score: " << std::to_string(score) << std::endl;
    }

    void run()
    {
        Seed seed; //("ngdkjbke");
        this->generate(seed);

        while(!this->shouldStop)
        {
            // ask for the command or for the answer or for the hint
            String input;
            std::getline(std::cin, input);
            // todo commands
            this->validateAnswer(input);
        }
    }

    bool shouldStop = false;
};

int main(int argc, char **argv)
{
    CliClient game;
    game.run(); // run.game.run();
}
