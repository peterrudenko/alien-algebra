
#include <iostream>
#include "cheerp/clientlib.h"
#include "Game.h"

class WebClient final : public Game
{
public:

    void onShowLevelInfo(const Game::Stats &stats) override
    {
        std::cout << "Seed: " << stats.seed << std::endl;
        std::cout << "===========" << std::endl;
        std::cout << "E-graph classes: " << stats.numRootClasses << std::endl;
        std::cout << "Root classes in hints: " << stats.numRootClassesInHints << std::endl;
        std::cout << "Hints collected: " << stats.numHints << std::endl;
        std::cout << "Generation: " << stats.numGenerationAttempts << std::endl;
        std::cout << "Gen time: " << stats.generationTimeMs << "ms" << std::endl;
        std::cout << "-----------" << std::endl;

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
        Seed seed;
        this->generate(seed);

        while(!this->shouldStop)
        {
            String input;
            std::getline(std::cin, input);
            this->validateAnswer(input);
        }
    }

    bool shouldStop = false;
};

[[cheerp::genericjs]]
void webMain()
{
    WebClient game;
    game.run();
}
