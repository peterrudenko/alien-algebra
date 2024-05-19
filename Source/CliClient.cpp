
#include "Common.h"
#include "Game.h"
#include <iostream>
#include <ostream>

// for debugging purposes
class CliClient final : public Game
{
public:

    void onStartGame() override {}

    void onStartLevel(int levelNumber, const Vector<String> &hints,
        const String &question, const Vector<String> &) override
    {
        std::cout << "Level " << std::to_string(levelNumber) + ":" << std::endl;
        for (const auto &hint : hints)
        {
            std::cout << hint << std::endl;
        }

        std::cout << question << std::endl;
    }

    void onEndLevel(bool passed, const Vector<bool> &answerIndices) override {}

    void onEndGame(bool win) override
    {
        this->shouldStop = true;
        std::cout << (win ? "Win!" : "Oh no.") << std::endl;
    }

    void run()
    {
        this->generate();

        while (!this->shouldStop)
        {
            String input;
            std::getline(std::cin, input);
            this->validateAnswer(input.c_str());
        }
    }

    bool shouldStop = false;
};

int main(int argc, char **argv)
{
    CliClient game;
    game.run(); // run.game.run();
}
