
#include "Common.h"
#include "Game.h"
#include <iostream>

class CLIGame final : public Game
{
public:

    void onShowHint(const String &hint) override
    {
        std::cout << hint << std::endl;
    }

    void onShowQuesion(const String &question)
    {
        std::cout << question << std::endl;
    }

    void onValidateAnswer(bool isValid) override
    {
        // todo pass more stuff here
        this->shouldStop = !isValid;
    }

    void run()
    {
        this->start();

        while(!this->shouldStop)
        {
            // ask for the command or for the answer or for the hint
            String input;
            std::getline(std::cin, input); 

            // todo parse

            this->showHints(input); // or this->validateAnswer(...);
        }
    }

    bool shouldStop = false;
};

int main(int argc, char **argv)
{
    CLIGame game;
    game.run(); // run.game.run();
}
