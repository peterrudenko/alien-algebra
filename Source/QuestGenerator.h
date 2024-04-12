#pragma once

#include "HintsExtractor.h"
#include "EGraph.h"

using e::ClassId;
using e::PatternTerm;
using e::RewriteRule;
using e::Symbol;

struct Level final
{
    // todo more than one equality hint?
    Hint::Ptr hintLeftHand;
    Hint::Ptr hintRightHand;

    Vector<Hint::Ptr> allUsedExpressions;
    HashSet<Symbol> allKnownTerms;
    HashSet<Symbol> allKnownOperations;

    Hint::Ptr question;
    Vector<Hint::Ptr> answers;

    Symbol operation;
};

struct QuestGenerator final
{
    QuestGenerator(e::Graph &eGraph, Random &random) :
        eGraph(eGraph), random(random) {}

    bool tryGenerate(Vector<Level> &outLevels)
    {
        // all expressions we've collected:
        HashMap<ClassId, Vector<Hint::Ptr>> allHints;

        // each level will contain a number of expressions to work with:
        Vector<HashSet<ClassId>> questClasses;

        if (!this->buildEGraph(allHints, questClasses))
        {
            //assert(false);
            return false;
        }

        // keep track of which operations were shown, so we don't introduce
        // unknown operations at each new level:
        HashSet<String> shownOperations;
        // todo also HashSet<String> shownTerms;

        for (int levelNumber = 0; levelNumber < QuestGenerator::numLevels; ++levelNumber)
        {
            Level level;

            Vector<Hint::Ptr> expressionsForQuestion;

            {
                int largestClassSize = 0;
                int smallestClassSize = std::numeric_limits<int>::max();

                for (const auto &it : allHints)
                {
                    assert(this->eGraph.find(it.first) == it.first);
                    if (!contains(questClasses[levelNumber], it.first))
                    {
                        continue; // skip unrelated classes
                    }

                    for (const auto &hint : it.second)
                    {
                        // pick hints that have at most 1 new "unknown" operation
                        if (hint->getNumNewOperations(shownOperations) <= 1)
                        {
                            level.allUsedExpressions.push_back(hint);
                        }
                    }

                    const auto classSize = int(it.second.size());
                    if (largestClassSize < classSize)
                    {
                        largestClassSize = classSize;
                        // expressionsForQuestion = it.second;
                    }

                    if (classSize > 1 && smallestClassSize > classSize)
                    {
                        smallestClassSize = classSize;
                        expressionsForQuestion = it.second;
                    }
                }
            }

            if (level.allUsedExpressions.empty())
            {
                //assert(false);
                return false;
            }

            {
                int largestHintDepth = 0;
                for (const auto &hint : expressionsForQuestion)
                {
                    // try to pick a question with exactly 1 unknown operation
                    if (hint->getNumNewOperations(shownOperations) != 1)
                    {
                        continue;
                    }

                    // fixme: sometimes it picks expressions with unknown single terms

                    if (largestHintDepth < hint->astDepth)
                    {
                        largestHintDepth = hint->astDepth;
                        level.question = hint;
                    }
                }

                if (level.question == nullptr)
                {
                    //assert(false);
                    return false;
                }

                const auto newOperations = level.question->getNewOperations(shownOperations);
                assert(newOperations.size() == 1);
                level.operation = newOperations.front();
                shownOperations.insert(level.operation);

                for (const auto &hint : expressionsForQuestion)
                {
                    if (hint != level.question && hint->getNumNewOperations(shownOperations) == 0)
                    {
                        level.answers.push_back(hint);
                    }
                }

                if (level.answers.empty())
                {
                    //assert(false);
                    return false;
                }

                std::sort(level.answers.begin(), level.answers.end(),
                    [](const Hint::Ptr &a, const Hint::Ptr &b)
                    {
                        return a->astDepth < b->astDepth;
                    });
            }

            auto rankedHintsForLeftHandSide = level.allUsedExpressions;
            std::sort(rankedHintsForLeftHandSide.begin(), rankedHintsForLeftHandSide.end(),
                [&](const Hint::Ptr &a, const Hint::Ptr &b)
                {
                    const auto getScore = [&](const Hint::Ptr &hint)
                    {
                        int hintScore = 0;

                        // penalty for all hints from question's class
                        // (but sometimes it's all we got)
                        hintScore -= int(level.question == hint) * 10000;
                        hintScore -= int(level.question->rootId == hint->rootId) * 1000;

                        // should not show hints with operations "unknown" to user
                        hintScore -= hint->getNumNewOperations(shownOperations) * 100;

                        // prioritize smaller hints here on the left side
                        hintScore += (level.question->astDepth - hint->astDepth) * 10;

                        hintScore += int(contains(hint->usedOperationSymbols, level.operation));

                        // todo better scoring
                        return hintScore;
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
                        int hintScore = 0;

                        hintScore -= int(level.question == hint || level.hintLeftHand == hint) * 10000;
                        hintScore -= int(level.question->rootId == hint->rootId) * 1000;

                        hintScore -= hint->getNumNewOperations(shownOperations) * 100;

                        // prioritize bigger hints here on the right side
                        hintScore += (hint->astDepth - level.question->astDepth) * 10;

                        hintScore += int(contains(hint->usedOperationSymbols, level.operation));

                        if (hint->astDepth > 1 && level.hintLeftHand->astDepth > 1 &&
                            (hint->rootNode.children.front().name == level.hintLeftHand->rootNode.children.front().name ||
                                hint->rootNode.children.back().name == level.hintLeftHand->rootNode.children.back().name))
                        {
                            hintScore -= 10;
                        }

                        hintScore -= int(hint->usedTermSymbols == level.hintLeftHand->usedTermSymbols);

                        // todo better scoring
                        return hintScore;
                    };

                    return getScore(a) > getScore(b);
                });

            level.hintRightHand = rankedHintsForRightHandSide.front();

            if (!contains(level.hintLeftHand->usedOperationSymbols, level.operation) &&
                !contains(level.hintRightHand->usedOperationSymbols, level.operation))
            {
                return false;
            }

            // finally, collect the info about used and "known" terms on this level,
            if (!outLevels.empty())
            {
                level.allKnownTerms = outLevels.back().allKnownTerms;
                level.allKnownOperations = outLevels.back().allKnownOperations;
            }

            for (const auto &hint : level.allUsedExpressions)
            {
                level.allKnownTerms.insert(hint->usedTermSymbols.begin(), hint->usedTermSymbols.end());
                level.allKnownOperations.insert(hint->usedOperationSymbols.begin(), hint->usedOperationSymbols.end());
            }

            outLevels.push_back(move(level));
        }

        return true;
    }

    bool buildEGraph(HashMap<ClassId, Vector<Hint::Ptr>> &outHints, Vector<HashSet<ClassId>> &outQuestClasses)
    {
        enum OperationProperty
        {
            Associativity,
            Commutativity,
            Distributivity,
            Idempotence,
            Identity,
            Zero
        };

        // todo multiple properties oer operation?
        // todo test which operation properties work better on which stages
        const Vector<OperationProperty> questSteps = {
            // todo random
            OperationProperty::Identity,
            OperationProperty::Commutativity,
            OperationProperty::Zero,
            OperationProperty::Associativity,
            OperationProperty::Idempotence,
            OperationProperty::Distributivity,
            OperationProperty::Identity,
            OperationProperty::Commutativity,
            OperationProperty::Associativity,
            OperationProperty::Distributivity,
            OperationProperty::Identity,
            OperationProperty::Commutativity,
            OperationProperty::Idempotence,
            OperationProperty::Associativity,
            OperationProperty::Idempotence,
        };

        /*
        todo generate up to numLevels

        Vector<OperationProperty> questSteps;
        // first two steps choose from the hardcoded lists:
        questSteps.push_back(this->random.pickOne<OperationProperty>({Identity, Zero}));
        questSteps.push_back(this->random.pickOne<OperationProperty>({Associativity}));

        // the next should be more random and rely on previous choices
        questSteps.push_back(this->random.pickOne<OperationProperty>({Identity, Commutativity, Associativity}));
        questSteps.push_back(this->random.pickOne<OperationProperty>({Idempotence, Commutativity, Distributivity}));
        questSteps.push_back(this->random.pickOne<OperationProperty>({Commutativity, Associativity, Distributivity}));
        questSteps.push_back(this->random.pickOne<OperationProperty>({Identity, Zero, Idempotence}));
        */

        Vector<RewriteRule> rewriteRules;

        HashSet<Symbol> usedTerms;
        HashSet<Symbol> usedOperations;

        HashSet<ClassId> recycledTermIds;
        HashSet<ClassId> recycledTermIdsForNextStep;
        HashSet<ClassId> usedRecycledTermIds;

        HashSet<Symbol> recycledOperationSymbols;
        HashSet<Symbol> usedRecycledOperationSymbols;

        Vector<HashSet<ClassId>> questsLeafIds;

        for (const auto &questStep : questSteps)
        {
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

            // #1 pair of terms will be used as an example, e.g.
            // a = a ~> b
            // #2 pair of terms will be used for the question, e.g.
            // q ~> w = ?
            // #3 and #4 pairs of terms will not be displayed immediately,
            // they are intended to be used as a part of another expression,
            // so that known operations show up from time to time, but
            // with different symbols (it also makes hints more predictable)

            const auto useSameTermOnBothSides = questStep == OperationProperty::Idempotence;

            const auto termL1 = makeRandomTermOrReuse();
            const auto termR1 = useSameTermOnBothSides ? termL1 : makeRandomTermOrReuse();

            const auto termL2 = makeRandomTermOrReuse();
            const auto termR2 = useSameTermOnBothSides ? termL2 : makeRandomTerm();

            const auto termL3 = makeRandomTermOrReuse();
            const auto termR3 = useSameTermOnBothSides ? termL3 : makeRandomTermOrReuse();

            const auto termL4 = makeRandomTermOrReuse();
            const auto termR4 = useSameTermOnBothSides ? termL4 : makeRandomTerm();

            const auto operationSymbol = this->random.pickOneUnique(this->allOperations, usedOperations);
            const auto operationId1 = this->eGraph.addOperation(operationSymbol, {termL1, termR1});
            const auto operationId2 = this->eGraph.addOperation(operationSymbol, {termL2, termR2});
            const auto operationId3 = this->eGraph.addOperation(operationSymbol, {termL3, termR3});
            const auto operationId4 = this->eGraph.addOperation(operationSymbol, {termL4, termR4});

            switch (questStep)
            {
                case OperationProperty::Identity:
                    rewriteRules.push_back(Rewrites::makeIdentityRule(operationSymbol));
                    break;

                case OperationProperty::Zero:
                    rewriteRules.push_back(Rewrites::makeZeroRule(operationSymbol));
                    break;

                case OperationProperty::Idempotence:
                    rewriteRules.push_back(Rewrites::makeIdempotenceRule(operationSymbol));
                    break;

                case OperationProperty::Commutativity:
                    rewriteRules.push_back(Rewrites::makeCommutativityRule(operationSymbol));
                    break;

                case OperationProperty::Associativity:
                    rewriteRules.push_back(Rewrites::makeAssociativityRule(operationSymbol));
                    break;

                case OperationProperty::Distributivity:
                    assert(!recycledOperationSymbols.empty());
                    rewriteRules.push_back(Rewrites::makeDistributivityRule(operationSymbol,
                        this->random.pickOneUnique(recycledOperationSymbols, usedRecycledOperationSymbols)));
                    break;

                default: assert(false);
            }

            // this basically controls how the graph is built further:

            // recycledTermIds.insert({termL3, operationId3});
            // recycledTermIds.insert(recycledTermIdsForNextStep.begin(), recycledTermIdsForNextStep.end());
            // recycledTermIdsForNextStep = {termL4, operationId4};

            recycledTermIds.insert({operationId2, termL3, operationId3});
            recycledTermIds.insert(recycledTermIdsForNextStep.begin(), recycledTermIdsForNextStep.end());
            recycledTermIdsForNextStep = {operationId1, termL4, operationId4};

            recycledOperationSymbols.insert(operationSymbol);

            questsLeafIds.push_back({operationId1, operationId2});
        }

        {
            // SteadyClock clock;

            for (int i = 0; i < 64; ++i)
            {
                for (const auto &rule : rewriteRules)
                {
                    this->eGraph.rewrite(rule);

                    if (this->eGraph.classes.size() > (usedTerms.size() + usedOperations.size()) * 2)
                    {
                        assert(false); // something has gone terribly wrong
                        return false;
                    }
                }
            }

            // std::cout << "Rewriting: " << clock.getMillisecondsElapsed() << "ms" << std::endl;
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
            // SteadyClock clock;
            HintsExtractor hintsExtractor(this->eGraph);
            outHints = hintsExtractor.extract();
            // std::cout << "HintsExtractor: " << clock.getMillisecondsElapsed() << "ms" << std::endl;
        }

        return true;
    }

private:

    struct Rewrites final
    {
        static constexpr auto x = "x";
        static constexpr auto y = "y";
        static constexpr auto z = "z";

        static RewriteRule makeIdentityRule(const Symbol &operation)
        {
            RewriteRule rule;
            rule.leftHand = PatternTerm(operation, {{x}, {y}});
            rule.rightHand = x;
            return rule;
        }

        static RewriteRule makeIdentityRule(const Symbol &operation, const Symbol &term)
        {
            RewriteRule rule;
            rule.leftHand = PatternTerm(operation, {{x}, PatternTerm(term, {})});
            rule.rightHand = x;
            return rule;
        }

        static RewriteRule makeZeroRule(const Symbol &operation)
        {
            RewriteRule rule;
            rule.leftHand = PatternTerm(operation, {{x}, {y}});
            rule.rightHand = y;
            return rule;
        }

        static RewriteRule makeZeroRule(const Symbol &operation, const Symbol &term)
        {
            RewriteRule rule;
            rule.leftHand = PatternTerm(operation, {{x}, PatternTerm(term, {})});
            rule.rightHand = PatternTerm(term, {});
            return rule;
        }

        static RewriteRule makeIdempotenceRule(const Symbol &operation)
        {
            RewriteRule rule;
            rule.leftHand = PatternTerm(operation, {{x}, {x}});
            rule.rightHand = x;
            return rule;
        }

        static RewriteRule makeIdempotenceRule(const Symbol &operation, const Symbol &term)
        {
            RewriteRule rule;
            rule.leftHand = PatternTerm(operation, {PatternTerm(term, {}), PatternTerm(term, {})});
            rule.rightHand = PatternTerm(term, {});
            return rule;
        }

        static RewriteRule makeCommutativityRule(const Symbol &operation)
        {
            RewriteRule rule;
            rule.leftHand = PatternTerm(operation, {{x}, {y}});
            rule.rightHand = PatternTerm(operation, {{y}, {x}});
            return rule;
        }

        static RewriteRule makeCommutativityRule(const Symbol &operation, const Symbol &term)
        {
            RewriteRule rule;
            rule.leftHand = PatternTerm(operation, {{x}, PatternTerm(term, {})});
            rule.rightHand = PatternTerm(operation, {PatternTerm(term, {}), {x}});
            return rule;
        }

        static RewriteRule makeCommutativityRule(const Symbol &operation, const Symbol &term1, const Symbol &term2)
        {
            RewriteRule rule;
            rule.leftHand = PatternTerm(operation, {PatternTerm(term1, {}), PatternTerm(term2, {})});
            rule.rightHand = PatternTerm(operation, {PatternTerm(term2, {}), PatternTerm(term1, {})});
            return rule;
        }

        static RewriteRule makeAssociativityRule(const Symbol &operation)
        {
            RewriteRule rule;
            rule.leftHand = PatternTerm(operation, {PatternTerm(operation, {{x}, {y}}), {z}});
            rule.rightHand = PatternTerm(operation, {{x}, PatternTerm(operation, {{y}, {z}})});
            return rule;
        }

        static RewriteRule makeDistributivityRule(const Symbol &operation1, const Symbol &operation2)
        {
            RewriteRule rule;
            assert(operation1 != operation2);
            rule.leftHand = PatternTerm(operation1, {{x}, PatternTerm(operation2, {{y}, {z}})});
            rule.rightHand = PatternTerm(operation2, {PatternTerm(operation1, {{x}, {y}}), PatternTerm(operation1, {{x}, {z}})});
            return rule;
        }
    };

private:

    const HashSet<Symbol> allTerms = {
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
        "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};

    const HashSet<Symbol> allOperations = {
        ":>", "|>", "-<", ">-", "=<<", ">>=", "-<<", ".-", "|-", "-|", "~>", "<~>",
        "~~>", "?", "!", "~", "::", "@", "#", "$", "&", "."};

private:

    static constexpr auto numLevels = 12;

    e::Graph &eGraph;

    Random &random;
};
