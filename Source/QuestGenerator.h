#pragma once

#include "HintsExtractor.h"
#include "AlienAlgebra.h"
#include "Parser.h"
#include "EGraph.h"

using e::ClassId;
using e::PatternTerm;
using e::RewriteRule;
using e::Symbol;

struct Level final
{
    Hint::Ptr hintLeftHand;
    Hint::Ptr hintRightHand;

    String getFormattedHint()  const noexcept
    {
        return this->hintLeftHand->formatted +  " " + Symbols::equalsSign + " " + this->hintRightHand->formatted;
    }

    Vector<Hint::Ptr> allUsedExpressions;
    HashMap<ClassId, Vector<Hint::Ptr>> allUsedExpressionsByClass;

    HashSet<Symbol> allKnownTerms;
    HashSet<Symbol> allKnownOperations;

    Hint::Ptr question;

    HashSet<String> answers;
    HashSet<String> allTermSymbolsInAnswers;
    HashSet<String> allOperationSymbolsInAnswers;

    HashSet<String> wrongAnswers;

    Vector<String> suggestions;

    Symbol operation;
};

struct QuestGenerator final
{
    QuestGenerator(e::Graph &eGraph, Random &random) :
        eGraph(eGraph), random(random) {}

    bool tryGenerate(Vector<Level> &outLevels)
    {
        // all expressions we've collected:
        HashMap<ClassId, Vector<Hint::Ptr>> allExpressions;

        // each level will contain a number of expressions to work with:
        Vector<HashSet<ClassId>> questClasses;

        if (!this->buildEGraph(allExpressions, questClasses))
        {
            //assert(false);
            return false;
        }

        // keep track of which operations were shown, so we don't introduce
        // unknown operations at each new level:
        HashSet<String> shownOperations;

        for (int levelNumber = 0; levelNumber < QuestGenerator::numLevels; ++levelNumber)
        {
            Level level;

            {
                for (const auto &it : allExpressions)
                {
                    const auto classId = it.first;
                    assert(this->eGraph.find(classId) == classId);
                    if (!contains(questClasses[levelNumber], classId))
                    {
                        continue;
                    }

                    for (const auto &hint : it.second)
                    {
                        // pick hints that have at most 1 new "unknown" operation
                        if (hint->getNumNewOperations(shownOperations) <= 1)
                        {
                            level.allUsedExpressionsByClass[classId].push_back(hint);
                            level.allUsedExpressions.push_back(hint);
                        }
                    }
                }
            }

            if (level.allUsedExpressions.empty())
            {
                //assert(false);
                return false;
            }

            Vector<Hint::Ptr> expressionsForQuestion;
            Vector<Hint::Ptr> expressionsForSuggestions;
            {
                int largestClassSize = 0;
                ClassId classIdForQuestion = -1;

                for (const auto &it : level.allUsedExpressionsByClass)
                {
                    const auto classSize = int(it.second.size());
                    if (largestClassSize < classSize)
                    {
                        largestClassSize = classSize;
                        classIdForQuestion = it.first;
                    }
                }

                for (const auto &it : level.allUsedExpressionsByClass)
                {
                    if (it.first == classIdForQuestion)
                    {
                        expressionsForQuestion = it.second;
                    }
                    else
                    {
                        append(expressionsForSuggestions, it.second);
                    }
                }
            }

            // pick the question:
            {
                std::sort(expressionsForQuestion.begin(), expressionsForQuestion.end(),
                    [&](const Hint::Ptr &a, const Hint::Ptr &b)
                    {
                        const auto getScore = [&](const Hint::Ptr &hint)
                        {
                            int score = 0;
                            // must have at least one op which wasn't shown yet:
                            score -= int(hint->getNumNewOperations(shownOperations) != 1) * 100;
                            score += int(hint->usedLeafIds.size()) * 10;
                            score += hint->astDepth;
                            return score;
                        };

                        return getScore(a) > getScore(b);
                    });

                level.question = expressionsForQuestion.front();

                if (level.question->getNumNewOperations(shownOperations) != 1)
                {
                    // we need a question with exactly 1 unknown operation
                    //assert(false);
                    return false;
                }

                const auto newOperations = level.question->getNewOperations(shownOperations);
                assert(newOperations.size() == 1);
                level.operation = newOperations.front();
                shownOperations.insert(level.operation);

                for (const auto &hint : expressionsForQuestion)
                {
                    if (hint != level.question &&
                        hint->getNumNewOperations(shownOperations) == 0)
                    {
                        level.answers.insert(hint->formatted);

                        append(level.allTermSymbolsInAnswers, hint->usedTermSymbols);
                        append(level.allOperationSymbolsInAnswers, hint->usedOperationSymbols);

                        const auto wrongAnswer = make<Hint>(*hint);
                        wrongAnswer->replaceRandomNode(this->random,
                            this->random.pickOne(level.allTermSymbolsInAnswers));

                        wrongAnswer->collectInfo();
                        level.wrongAnswers.insert(wrongAnswer->formatted);
                    }
                }

                if (level.answers.empty())
                {
                    //assert(false);
                    return false;
                }
            }

            // pick the left-hand and right-hand sides for the hint:
            auto rankedHintsForLeftHandSide = level.allUsedExpressions;
            std::sort(rankedHintsForLeftHandSide.begin(), rankedHintsForLeftHandSide.end(),
                [&](const Hint::Ptr &a, const Hint::Ptr &b)
                {
                    const auto getScore = [&](const Hint::Ptr &hint)
                    {
                        int score = 0;

                        // penalty for all hints from question's class
                        // (but sometimes it's all we got)
                        score -= int(level.question == hint) * 10000;
                        score -= int(level.question->rootId == hint->rootId) * 1000;

                        // should not show hints with operations "unknown" to user
                        score -= hint->getNumNewOperations(shownOperations) * 100;

                        score -= int(hint->astDepth == level.question->astDepth) * 20;
                        score += hint->astDepth * 10;

                        score += int(hint->usedOperationSymbols.size()) * 10;

                        score += int(contains(hint->usedOperationSymbols, level.operation));

                        return score;
                    };

                    return getScore(a) > getScore(b);
                });

            level.hintLeftHand = rankedHintsForLeftHandSide.front();

            auto rankedHintsForRightHandSide = level.allUsedExpressions;
            std::sort(rankedHintsForRightHandSide.begin(), rankedHintsForRightHandSide.end(),
                [&](const Hint::Ptr &a, const Hint::Ptr &b)
                {
                    const auto getScore = [&](const Hint::Ptr &hint)
                    {
                        int score = 0;

                        score -= int(level.question == hint || level.hintLeftHand == hint) * 10000;
                        score -= int(level.question->rootId == hint->rootId) * 1000;

                        score -= hint->getNumNewOperations(shownOperations) * 100;

                        score -= int(hint->astDepth == level.question->astDepth) * 20;
                        //score += hint->astDepth * 10;

                        score += int(hint->usedOperationSymbols.size()) * 10;

                        score += int(contains(hint->usedOperationSymbols, level.operation));

                        if (hint->astDepth > 1 && level.hintLeftHand->astDepth > 1 &&
                            (hint->rootNode.children.front().name == level.hintLeftHand->rootNode.children.front().name ||
                                hint->rootNode.children.back().name == level.hintLeftHand->rootNode.children.back().name))
                        {
                            score -= 10;
                        }

                        score -= int(hint->usedTermSymbols == level.hintLeftHand->usedTermSymbols);
                        score -= int(hint->astDepth == level.hintLeftHand->astDepth);

                        return score;
                    };

                    return getScore(a) > getScore(b);
                });

            level.hintRightHand = rankedHintsForRightHandSide.front();

            if (!contains(level.hintLeftHand->usedOperationSymbols, level.operation) &&
                !contains(level.hintRightHand->usedOperationSymbols, level.operation))
            {
                return false;
            }

            if (level.hintRightHand->astDepth != level.question->astDepth && this->random.rollD5())
            {
                std::swap(level.hintLeftHand, level.hintRightHand);
            }

            // pick the suggestions:
            {
                std::sort(expressionsForSuggestions.begin(), expressionsForSuggestions.end(),
                    [&](const Hint::Ptr &a, const Hint::Ptr &b)
                    {
                        const auto getScore = [&](const Hint::Ptr &hint)
                        {
                            int score = 0;
                            score -= (hint->formatted == level.hintLeftHand->formatted ||
                                hint->formatted == level.hintRightHand->formatted) * 10;
                            // prioritize "similar look" (having as much of the same symbols as in the answers)
                            score -= hint->getNumNewTerms(level.allTermSymbolsInAnswers);
                            return score;
                        };

                        return getScore(a) > getScore(b);
                    });

                HashSet<String> uniqueSuggestions;
                {
                    HashSet<String> usedAnswers;

                    // level 0 is introductory,
                    // level 1 should have more valid answers in suggestions (bait)
                    // last level shoud have less valid answers and more wrong answers (boss fight)
                    const auto isLastLevel = levelNumber == QuestGenerator::numLevels - 1;
                    for (int i = 0; i < std::min(3 + int(levelNumber == 1) - int(isLastLevel),
                        int(level.answers.size())); ++i)
                    {
                        uniqueSuggestions.insert(this->random.pickOneUnique(level.answers, usedAnswers));
                    }

                    for (int i = 0; i < std::min(3 + int(isLastLevel), int(level.wrongAnswers.size())); ++i)
                    {
                        uniqueSuggestions.insert(this->random.pickOneUnique(level.wrongAnswers, usedAnswers));
                    }
                }

                for (int i = 0; i < std::min(3, int(expressionsForSuggestions.size())); ++i)
                {
                    uniqueSuggestions.insert(expressionsForSuggestions[i]->formatted);
                }

                level.suggestions.insert(level.suggestions.end(),
                    uniqueSuggestions.begin(), uniqueSuggestions.end());

                this->random.shuffle(level.suggestions);
            }

            // finally, collect info about used and "known" terms on this level
            if (!outLevels.empty())
            {
                level.allKnownTerms = outLevels.back().allKnownTerms;
                level.allKnownOperations = outLevels.back().allKnownOperations;
            }

            for (const auto &hint : level.allUsedExpressions)
            {
                append(level.allKnownTerms, hint->usedTermSymbols);
                append(level.allKnownOperations, hint->usedOperationSymbols);
            }

            outLevels.push_back(move(level));
        }

        return true;
    }

    bool buildEGraph(HashMap<ClassId, Vector<Hint::Ptr>> &outHints, Vector<HashSet<ClassId>> &outQuestClasses)
    {
        HashSet<OperationProperty> usedProperties;
        Vector<OperationProperty> propertiesByLevel;

        for (int levelNumber = 0; levelNumber < numLevels; ++levelNumber)
        {
            HashSet<OperationProperty> availableForThisLevel;
            for (const auto &property : AlienAlgebra::allProperties)
            {
                if (contains(property.levels, levelNumber))
                {
                    availableForThisLevel.insert(property);
                }
            }

            propertiesByLevel.push_back(this->random.pickOneUnique(availableForThisLevel, usedProperties));
        }

        Vector<RewriteRule> allRewriteRules;

        HashSet<Symbol> usedTerms;
        HashSet<int> usedOperationGroups;

        HashSet<ClassId> recycledTermIds;
        HashSet<ClassId> recycledTermIdsForNextStep;
        HashSet<ClassId> usedRecycledTermIds;

        Vector<HashSet<ClassId>> questsLeafIds;

        enum class ExpressionShape
        {
            xx,
            xy,
            xyzLeftAssociative,
            xyzRightAssociative
        };

        // a hack to determine what kind of expression shape we want to generate:
        const auto getLeftHandSideShapeType = [](const e::RewriteRule &rule)
        {
            assert(rule.leftHand.term != nullptr); // expects an operation
            const auto &children = rule.leftHand.term->arguments;

            assert(children.size() == 2);
            if (children.front().variable != nullptr &&
                children.back().variable != nullptr &&
                *children.front().variable == *children.back().variable)
            {
                return ExpressionShape::xx;
            }
            else if (children.front().term != nullptr)
            {
                return ExpressionShape::xyzLeftAssociative;
            }
            else if (children.back().term != nullptr)
            {
                return ExpressionShape::xyzRightAssociative;
            }

            return ExpressionShape::xy;
        };

        for (const auto &operationProperty : propertiesByLevel)
        {
            const auto operationSymbol = this->random.pickOne(
                this->random.pickOneUnique(this->allOperations, usedOperationGroups));

            const auto makeRandomTerm = [&]()
            {
                const auto termSymbol = this->random.pickOneUnique(this->allTerms, usedTerms);
                const auto termId = this->eGraph.addTerm(termSymbol);
                return termId;
            };

            const auto makeRandomTermOrReuse = [&]()
            {
                if (recycledTermIds.empty())
                {
                    return makeRandomTerm();
                }

                return this->random.pickOneUnique(recycledTermIds, usedRecycledTermIds);
            };

            const auto makeRandomTermOrReuseD2 = [&]()
            {
                if (recycledTermIds.empty() || this->random.rollD2())
                {
                    return makeRandomTerm();
                }

                return this->random.pickOneUnique(recycledTermIds, usedRecycledTermIds);
            };


            // the 1st pair of terms will be used as an example, e.g.
            // a = a ~> b
            // the 2nd pair of terms will be used for the question, e.g.
            // q ~> w = ?
            // the 3rd and the 4th pairs of terms will not be displayed immediately,
            // they are intended to be used as parts of another expression later,
            // so that known operations show up from time to time, but
            // with different symbols (it also makes hints more predictable)
            ClassId termL1, termR1, termL2, termR2, termL3, termR3, termL4, termR4;

            const auto levelRewriteRule = Parser::makeRewriteRule(operationProperty.rewriteTemplate, operationSymbol);

            switch (getLeftHandSideShapeType(levelRewriteRule))
            {
            case ExpressionShape::xx:
                termL1 = termR1 = makeRandomTermOrReuse();
                termL2 = termR2 = makeRandomTermOrReuse();
                termL3 = termR3 = makeRandomTermOrReuse();
                termL4 = termR4 = makeRandomTermOrReuse();
                break;
            case ExpressionShape::xy:
                termL1 = makeRandomTermOrReuse();
                termR1 = makeRandomTermOrReuse();
                termL2 = makeRandomTermOrReuse();
                termR2 = makeRandomTerm();
                termL3 = makeRandomTermOrReuse();
                termR3 = makeRandomTermOrReuse();
                termL4 = makeRandomTerm();
                termR4 = makeRandomTermOrReuse();
                break;
            case ExpressionShape::xyzLeftAssociative:
                termL1 = this->eGraph.addOperation(operationSymbol, { makeRandomTermOrReuseD2(), makeRandomTerm() });
                termR1 = makeRandomTermOrReuse();
                termL2 = this->eGraph.addOperation(operationSymbol, { makeRandomTerm(), makeRandomTermOrReuseD2() });
                termR2 = makeRandomTermOrReuse();
                termL3 = this->eGraph.addOperation(operationSymbol, { makeRandomTermOrReuseD2(), makeRandomTermOrReuseD2() });
                termR3 = makeRandomTermOrReuse();
                termL4 = this->eGraph.addOperation(operationSymbol, { makeRandomTermOrReuseD2(), makeRandomTermOrReuseD2() });
                termR4 = makeRandomTermOrReuse();
                break;
            case ExpressionShape::xyzRightAssociative:
                termL1 = makeRandomTermOrReuse();
                termR1 = this->eGraph.addOperation(operationSymbol, { makeRandomTermOrReuseD2(), makeRandomTerm() });
                termL2 = makeRandomTermOrReuse();
                termR2 = this->eGraph.addOperation(operationSymbol, { makeRandomTerm(), makeRandomTermOrReuseD2() });
                termL3 = makeRandomTermOrReuse();
                termR3 = this->eGraph.addOperation(operationSymbol, { makeRandomTermOrReuseD2(), makeRandomTermOrReuseD2() });
                termL4 = makeRandomTermOrReuse();
                termR4 = this->eGraph.addOperation(operationSymbol, { makeRandomTermOrReuseD2(), makeRandomTermOrReuseD2() });
                break;
            default:
                assert(false);
            }

            const auto operationId1 = this->eGraph.addOperation(operationSymbol, {termL1, termR1});
            const auto operationId2 = this->eGraph.addOperation(operationSymbol, {termL2, termR2});
            const auto operationId3 = this->eGraph.addOperation(operationSymbol, {termL3, termR3});
            const auto operationId4 = this->eGraph.addOperation(operationSymbol, {termL4, termR4});

            recycledTermIds.insert({operationId3, termL3, operationId4});
            recycledTermIds.insert(recycledTermIdsForNextStep.begin(), recycledTermIdsForNextStep.end());
            recycledTermIdsForNextStep = {operationId1, termL4, operationId2};

            questsLeafIds.push_back({operationId1, operationId2});

            allRewriteRules.push_back(levelRewriteRule);
        }

        {
            for (int i = 0; i < 32; ++i)
            {
                for (const auto &rule : allRewriteRules)
                {
                    this->eGraph.rewrite(rule);

                    if (this->eGraph.classes.size() > (usedTerms.size() + usedOperationGroups.size()) * 5)
                    {
                        //assert(false); // something has gone terribly wrong
                        return false;
                    }
                }
            }
        }

        outQuestClasses.clear();
        for (const auto &leafIds : questsLeafIds)
        {
            HashSet<ClassId> rootClasses;
            for (const auto &leafId : leafIds)
            {
                rootClasses.insert(this->eGraph.find(leafId));
            }
            outQuestClasses.push_back(move(rootClasses));
        }

        {
            HintsExtractor hintsExtractor(this->eGraph);
            outHints = hintsExtractor.extract();
        }

        return true;
    }

private:

#if WEB_CLIENT

    const HashSet<Symbol> allTerms = {
        "a", "d", "e", "f", "n", "o", "q", "s", "u", "v", "w", "x", "y", "z",
        "0", "1", "3", "4", "5", "7", "9"
    };

    const Vector<HashSet<Symbol>> allOperations = {
        {"\xe2\x87\x8c", "\xe2\xa5\xa2", "\xe2\xa5\xa4"}, // ⇌ ⥢ ⥤
        {"\xe2\xa5\x83", "\xe2\xa5\x84"}, // ⥃ ⥄
        {"\xe2\xa4\x9d", "\xe2\xa4\x9e"}, // ⤝ ⤞
        //{"\xe2\xac\xb7", "\xe2\xa4\x90"}, // ⬷ ⤐
        //{"\xe2\x87\x9c", "\xe2\x87\x9d"}, // ⇜ ⇝
        //{"\xe2\x86\x9c", "\xe2\x86\x9d"}, // ↜ ↝
        {"\xe2\x86\xab", "\xe2\x86\xac"}, // ↫ ↬
        {"\xe2\xac\xb8", "\xe2\xa4\x91"}, // ⬸ ⤑
        {"\xe2\xa4\x99", "\xe2\xa4\x9a", "\xe2\xa4\x9c"}, // ⤙ ⤚ ⤜
        {"\xe2\xa5\x8a", "\xe2\xa5\x90", "\xe2\x86\xbd", "\xe2\x87\x80"}, // ⥊ ⥐ ↽ ⇀
        {"\xe2\xa4\xbe", "\xe2\xa4\xbf", "\xe2\xa4\xb8", "\xe2\xa4\xb9", "\xe2\xa4\xbb"}, // ⤾ ⤿ ⤸ ⤹ ⤻
        {"\xe2\x88\xb7", "\xe2\x88\xb4", "\xe2\x88\xb5"}, // ∷ ∴ ∵
        {"\xe2\xa0\x94", "\xe2\xa0\xa2"}, // ⠔ ⠢
        {"\xe2\x88\xba", "\xe2\x88\xbb"}, // ∺ ∻
        {"\xe2\x89\x80"}, // ≀
        //{"\xe2\xa5\x88"}, // ⥈
        //{"\xe2\x9e\xbb"} // ➻
    };

#else

    const HashSet<Symbol> allTerms = {
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
        "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"
    };

    const Vector<HashSet<Symbol>> allOperations = {
        {":>", "|>"}, {"-<", ">-"}, {"=<<", "-<<"}, {"|-", "-|"}, {"~>"},
        {"?"}, {"!"}, {"~"}, {"::"}, {"@"}, {"#"}, {"$"}, {"&"}, {"."}
    };

#endif

private:

    static constexpr auto numLevels = 4;

    e::Graph &eGraph;

    Random &random;
};
