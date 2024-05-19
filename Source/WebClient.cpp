
#include "Common.h"
#include "Game.h"
#include "cheerp/client.h"
#include "cheerp/clientlib.h"

[[cheerp::genericjs]]
void initLevel(const String &levelId, const Vector<String> &hints,
    const String &question, const Vector<String> &suggestions,
    Game *client)
{
    using namespace client;

    auto *currentLevelElement = document.getElementById(levelId.c_str());
    assert(currentLevelElement != nullptr);

    currentLevelElement->set_className("level visited");

    auto *levelContentElements = currentLevelElement->getElementsByClassName("content");
    assert (levelContentElements->get_length() == 1);
    auto *levelContentElement = levelContentElements->item(0);

    while (levelContentElement->get_firstChild() != nullptr)
    {
        levelContentElement->removeChild(levelContentElement->get_firstChild());
    }

    for (int i = 0; i < hints.size(); ++i)
    {
        auto *hintNode = document.createElement("div");
        hintNode->set_className("question");
        hintNode->set_innerText(client::String::fromUtf8(hints[i].c_str(), hints[i].size()));
        levelContentElement->appendChild(hintNode);

        auto *questionNode = document.createElement("div");
        questionNode->set_className("question");
        questionNode->set_innerText(client::String::fromUtf8(question.c_str(), question.size()));
        levelContentElement->appendChild(questionNode);
    }

    for (int i = 0; i < suggestions.size(); ++i)
    {
        auto *button = document.createElement("button");
        button->setAttribute("class", "suggestion");
        button->set_innerText(client::String::fromUtf8(suggestions[i].c_str(), suggestions[i].size()));

        auto selectSuggestionCallback = [levelContentElement, button, client, i]() -> void {
            client->validateAnswer(i);
        };

        button->addEventListener("click", cheerp::Callback(selectSuggestionCallback));
        levelContentElement->appendChild(button);
    }
}

[[cheerp::genericjs]]
void finishLevel(const String &levelId,
    bool levelPassed, const Vector<bool> &answerIndices)
{
    using namespace client;

    auto *currentLevelElement = document.getElementById(levelId.c_str());
    currentLevelElement->set_className(levelPassed ? "level passed" : "level failed");

    auto *levelContentElements = currentLevelElement->getElementsByClassName("content");
    assert(levelContentElements->get_length() == 1);
    auto *levelContentElement = levelContentElements->item(0);

    auto *suggestions = currentLevelElement->getElementsByClassName("suggestion");
    for (int i = 0; i < suggestions->get_length(); ++i)
    {
        if (!answerIndices[i])
        {
            suggestions->item(i)->set_className("suggestion wrong");
        }
    }
}

[[cheerp::genericjs]]
void finishGame(bool win)
{
    using namespace client;

    document.get_body()->set_className(win ? "win" : "gameover");

    auto *levelDrafts = document.getElementsByClassName("level draft");
    while (levelDrafts->get_length() > 0)
    {
        levelDrafts->item(0)->get_parentNode()->removeChild(levelDrafts->item(0));
    }
}


class WebClient final : public Game
{
public:

    void onStartGame() override
    {
        this->currentLevel = 0;
    }

    void onStartLevel(int levelNumber,
        const Vector<String> &hints, const String &question,
        const Vector<String> &suggestions) override
    {
        this->currentLevel = levelNumber;
        initLevel(std::to_string(levelNumber), hints, question, suggestions, this);
    }

    void onEndLevel(bool passed, const Vector<bool> &answerIndices) override
    {
        finishLevel(std::to_string(this->currentLevel), passed, answerIndices);
    }
    
    void onEndGame(bool win) override
    {
        finishGame(win);
    }

    void run()
    {
        this->generate();
    }

    int currentLevel = 0;
};

[[cheerp::genericjs]]
void onLoad()
{
    static WebClient game;
    game.run();
}

[[cheerp::genericjs]]
void webMain()
{
    if (!client::document.get_readyState()->startsWith(client::String("loading")))
    {
        onLoad();
    }
    else
    {
        client::document.addEventListener("DOMContentLoaded", cheerp::Callback(onLoad));
    }
}
