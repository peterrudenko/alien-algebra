#pragma once

#include "Common.h"
#include "Random.h"
#include "Parser.h"
#include "Seed.h"
#include "HintsExtractor.h"
#include "Serialization.h"

#include "EGraph.h"
using e::ClassId;
using e::makePatternTerm;
using e::RewriteRule;
using e::Symbol;

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
        int numRootClassesInHints = 0;
        int numHints = 0;

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

        SteadyClock timer;
        bool hasResult = false;
        int numAttempts = 0;

        while (!hasResult)
        {
            this->hints = {};
            this->questClasses = {};
            this->eGraph = {};
            hasResult = this->tryGenerate();
            numAttempts++;
        }

        Stats stats;

        for (const auto &it : this->hints)
        {
            stats.numHints += int(it.second.size());

            // debug/cheat mode:
            /*
            this->onShowHint("----------- (" + std::to_string(it.second.size()) + ") -----------");
            for (const auto &hint : it.second)
            {
                this->onShowHint(hint->formatted);
            }
            */
        }

        stats.seed = seed.encode();
        //const auto testSeedDecoding = Seed(stats.seed);
        //const auto testSeedEncoding = testSeedDecoding.encode();

        stats.numGenerationAttempts = numAttempts;
        stats.generationTimeMs = timer.getMillisecondsElapsed();

        stats.numRootClasses = this->eGraph.classes.size();
        stats.numRootClassesInHints = this->hints.size();

        //stats.internalState = Serialization::encode(Serialization::serialize(this->eGraph));

        this->onShowLevelInfo(stats);

        this->generateCurrentQuestion();
    }

    void generateCurrentQuestion()
    {
        // those hints which are related to current level:
        Vector<Hint::Ptr> hintsForCurrentQuest;

        // "a ~ b = ?"
        Vector<Hint::Ptr> expressionsForQuestion;

        {
            int largestClassSize = 0;
            int smallestClassSize = std::numeric_limits<int>::max();

            for (const auto &it : this->hints)
            {
                assert(this->eGraph.find(it.first) == it.first);
                if (!this->questClasses[this->currentQuest].contains(it.first))
                {
                    continue;
                }

                for (const auto &hint : it.second)
                {
                    // skip hints that have more than 1 "unknown" operation
                    if (hint->getNumNewOperations(this->shownOperations) <= 1)
                    {
                        hintsForCurrentQuest.push_back(hint);
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

        {
            Hint::Ptr largestHint = nullptr;
            Hint::Ptr smallestHint = nullptr;
            int largestHintDepth = 0;
            int smallestHintDepth = std::numeric_limits<int>::max();
            for (const auto &hint : expressionsForQuestion)
            {
                if (hint->getNumNewOperations(this->shownOperations) > 1)
                {
                    // skip hints that have more than 1 "unknown" operation
                    continue;
                }

                if (largestHintDepth < hint->astDepth)
                {
                    largestHintDepth = hint->astDepth;
                    largestHint = hint;
                }

                if (smallestHintDepth > hint->astDepth)
                {
                    smallestHintDepth = hint->astDepth;
                    smallestHint = hint; // todo: award with most points
                }
            }

            this->question = largestHint;
            assert(this->question != nullptr);
            this->shownOperations.insert(this->question->rootNode.name);
            assert(this->question != nullptr);
        }

        this->showHintPairs(hintsForCurrentQuest, this->question);

        this->onShowQuestion(this->question->formatted);
    }

    void showHintPairs(const Vector<Hint::Ptr> &hintClass, const Hint::Ptr question)
    {
        auto sortedHints = hintClass;
        std::sort(sortedHints.begin(), sortedHints.end(),
            [&](const Hint::Ptr &a, const Hint::Ptr &b)
            {
                const auto getScore = [&](const Hint::Ptr &hint)
                {
                    int hintScore = 0;

                    // penalty for all hints from question's class
                    // (but sometimes it's all we got)
                    hintScore -= int(this->question == hint) * 10000;
                    hintScore -= int(this->question->rootId == hint->rootId) * 1000;

                    // should not show hints with operations "unknown" to user
                    hintScore -= hint->getNumNewOperations(this->shownOperations) * 100;

                    // prioritize smaller hints here on the left side
                    hintScore += (this->question->astDepth - hint->astDepth) * 10;

                    // todo better scoring
                    return hintScore;
                };

                return getScore(a) > getScore(b);
            });

        const auto hintLeftHandSide = sortedHints.front();
        const auto hintRightHandSide = this->pickHintRightHandSide(hintLeftHandSide, hintClass);
        this->onShowHint(hintLeftHandSide->formatted + " = " + hintRightHandSide->formatted);
    }

    Hint::Ptr pickHintRightHandSide(const Hint::Ptr targetHint, const Vector<Hint::Ptr> &allHints) const
    {
        auto sortedHints = allHints;

        std::sort(sortedHints.begin(), sortedHints.end(),
            [&](const Hint::Ptr &a, const Hint::Ptr &b)
            {
                const auto getScore = [&](const Hint::Ptr &hint)
                {
                    int hintScore = 0;

                    hintScore -= int(this->question == hint || targetHint == hint) * 10000;
                    hintScore -= int(this->question->rootId == hint->rootId) * 1000;

                    hintScore -= hint->getNumNewOperations(this->shownOperations) * 100;

                    // prioritize bigger hints here on the right side
                    hintScore += (hint->astDepth - question->astDepth) * 10;

                    if (hint->astDepth > 1 && targetHint->astDepth > 1 &&
                        (hint->rootNode.children.front().name == targetHint->rootNode.children.front().name ||
                            hint->rootNode.children.back().name == targetHint->rootNode.children.back().name))
                    {
                        hintScore -= 10;
                    }

                    hintScore -= int(hint->usedSymbols == targetHint->usedSymbols);

                    // todo better scoring
                    return hintScore;
                };

                return getScore(a) > getScore(b);
            });

        return sortedHints.front();
    }

    // here goes the quest generator
    bool tryGenerate()
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
        const Vector<OperationProperty> questSteps = { // todo random
            OperationProperty::Identity,
            OperationProperty::Commutativity,
            OperationProperty::Zero,
            OperationProperty::Distributivity,
            OperationProperty::Idempotence,
            OperationProperty::Associativity,
            OperationProperty::Identity,
            OperationProperty::Commutativity,
            OperationProperty::Distributivity,
            OperationProperty::Identity,
            OperationProperty::Commutativity,
        };

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
            recycledTermIds.insert({operationId2, operationId3});
            recycledTermIds.insert(recycledTermIdsForNextStep.begin(), recycledTermIdsForNextStep.end());
            recycledTermIdsForNextStep = {operationId1, operationId4};
            recycledOperationSymbols.insert(operationSymbol);

            questsLeafIds.push_back({operationId1, operationId2});
        }

        {
            SteadyClock clock;

            for (int i = 0; i < 32; ++i)
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

            std::cout << "Rewriting: " << clock.getMillisecondsElapsed() << "ms" << std::endl;
        }

        for (const auto &leafIds : questsLeafIds)
        {
            HashSet<ClassId> rootClasses;
            for (const auto &leafId : leafIds)
            {
                rootClasses.insert(this->eGraph.find(leafId));
            }
            this->questClasses.push_back(move(rootClasses));
        }

        {
            SteadyClock clock;
            HintsExtractor hintsExtractor(this->eGraph);
            this->hints = hintsExtractor.extract();
            std::cout << "HintsExtractor: " << clock.getMillisecondsElapsed() << "ms" << std::endl;
        }

        return true;
    }

    void validateAnswer(const String &expression)
    {
        // debug mode
        if (expression.empty())
        {
            this->onShowHint("===========");

            for (const auto &it : this->hints)
            {
                if (it.first == this->question->rootId)
                {
                    for (const auto &hint : it.second)
                    {
                        this->onShowHint(hint->formatted);
                    }
                }
            }

            this->onShowHint("===========");

            this->currentQuest++;
            this->generateCurrentQuestion();

            return;
        }

        Parser::Input input(expression, "");

        try
        {
            if (const auto root = Parser::parse(input))
            {
                e::PatternTerm pattern;
                Parser::convertAstToPattern(pattern, *root);
                const auto formattedAnswer = Parser::formatPatternTerm(pattern, false);

                if (this->acceptedAnswers.contains(formattedAnswer))
                {
                    // todo give a hint?
                    return;
                }

                bool isValidAnswer = false;
                for (const auto &[classId, _classPtr] : this->eGraph.classes)
                {
                    isValidAnswer = isValidAnswer ||
                                    (classId == this->question->rootId && this->matchPatternTerm(pattern, classId));
                }

                if (isValidAnswer)
                {
                    this->acceptedAnswers.insert(formattedAnswer);
                    this->score += this->validAnswerScore;
                    this->onValidateAnswer(isValidAnswer, this->score);

                    this->currentQuest++;
                    this->generateCurrentQuestion();

                    return;
                }

                this->score += this->failedAttemptScore;
                this->onValidateAnswer(isValidAnswer, this->score);

                // not a valid answer, try to give a hint
                for (const auto &[classId, _classPtr] : this->eGraph.classes)
                {
                    if (!this->matchPatternTerm(pattern, classId))
                    {
                        continue;
                    }

                    if (!this->hints.contains(classId))
                    {
                        continue;
                    }

                    const auto displayedHints = this->hints[classId];
                    if (auto otherHint = pickHintPair(formattedAnswer, displayedHints))
                    {
                        this->onShowHint(formattedAnswer + " = " + (*otherHint)->formatted);
                    }
                }
            }
            else
            {
                // todo what?
            }
        }
        catch (const std::exception &e)
        {
            this->onShowError(e.what()); // todo something more sensible
        }
    }

private:

    static Optional<Hint::Ptr> pickHintPair(const String &targetFormatted, const Vector<Hint::Ptr> &allHints)
    {
        for (const auto &otherHint : allHints)
        {
            if (otherHint->formatted == targetFormatted)
            {
                continue;
            }

            // todo more checks
            return otherHint;
        }

        return {};
    }

    bool matchPatternTerm(const e::Pattern &pattern, ClassId classId)
    {
        const auto rootId = this->eGraph.find(classId);
        assert(this->eGraph.classes.contains(rootId));

        const auto &patternTerm = std::get<e::PatternTerm>(pattern);
        for (const auto &term : this->eGraph.classes.at(rootId)->terms)
        {
            if (term->name != patternTerm.name || term->childrenIds.size() != patternTerm.arguments.size())
            {
                continue;
            }

            bool childrenMatch = true;
            for (int i = 0; i < patternTerm.arguments.size(); ++i)
            {
                childrenMatch = childrenMatch && this->matchPatternTerm(patternTerm.arguments[i], term->childrenIds[i]);
            }

            if (childrenMatch)
            {
                return true;
            }
        }

        return false;
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
            rule.leftHand = e::makePatternTerm(operation, {x, y});
            rule.rightHand = x;
            return rule;
        }

        static RewriteRule makeIdentityRule(const Symbol &operation, const Symbol &term)
        {
            RewriteRule rule;
            rule.leftHand = e::makePatternTerm(operation, {x, e::makePatternTerm(term)});
            rule.rightHand = x;
            return rule;
        }

        static RewriteRule makeIdentityRule(const Symbol &operation, const Symbol &term1, const Symbol &term2)
        {
            RewriteRule rule;
            const auto patternTerm1 = e::makePatternTerm(term1);
            const auto patternTerm2 = e::makePatternTerm(term2);
            rule.leftHand = e::makePatternTerm(operation, {patternTerm1, patternTerm2});
            rule.rightHand = patternTerm1;
            return rule;
        }

        static RewriteRule makeZeroRule(const Symbol &operation)
        {
            RewriteRule rule;
            rule.leftHand = e::makePatternTerm(operation, {x, y});
            rule.rightHand = y;
            return rule;
        }

        static RewriteRule makeZeroRule(const Symbol &operation, const Symbol &term)
        {
            RewriteRule rule;
            const auto patternTerm = e::makePatternTerm(term);
            rule.leftHand = e::makePatternTerm(operation, {x, patternTerm});
            rule.rightHand = patternTerm;
            return rule;
        }

        static RewriteRule makeZeroRule(const Symbol &operation, const Symbol &term1, const Symbol &term2)
        {
            RewriteRule rule;
            const auto patternTerm1 = e::makePatternTerm(term1);
            const auto patternTerm2 = e::makePatternTerm(term2);
            rule.leftHand = e::makePatternTerm(operation, {patternTerm1, patternTerm2});
            rule.rightHand = patternTerm2;
            return rule;
        }

        static RewriteRule makeIdempotenceRule(const Symbol &operation)
        {
            RewriteRule rule;
            rule.leftHand = e::makePatternTerm(operation, {x, x});
            rule.rightHand = x;
            return rule;
        }

        static RewriteRule makeIdempotenceRule(const Symbol &operation, const Symbol &term)
        {
            RewriteRule rule;
            const auto patternTerm = e::makePatternTerm(term);
            rule.leftHand = e::makePatternTerm(operation, {patternTerm, patternTerm});
            rule.rightHand = patternTerm;
            return rule;
        }

        static RewriteRule makeCommutativityRule(const Symbol &operation)
        {
            RewriteRule rule;
            rule.leftHand = e::makePatternTerm(operation, {x, y});
            rule.rightHand = e::makePatternTerm(operation, {y, x});
            return rule;
        }

        static RewriteRule makeCommutativityRule(const Symbol &operation, const Symbol &term)
        {
            RewriteRule rule;
            const auto patternTerm = e::makePatternTerm(term);
            rule.leftHand = e::makePatternTerm(operation, {x, patternTerm});
            rule.rightHand = e::makePatternTerm(operation, {patternTerm, x});
            return rule;
        }

        static RewriteRule makeCommutativityRule(const Symbol &operation, const Symbol &term1, const Symbol &term2)
        {
            RewriteRule rule;
            const auto patternTerm1 = e::makePatternTerm(term1);
            const auto patternTerm2 = e::makePatternTerm(term2);
            rule.leftHand = e::makePatternTerm(operation, {patternTerm1, patternTerm2});
            rule.rightHand = e::makePatternTerm(operation, {patternTerm2, patternTerm1});
            return rule;
        }

        static RewriteRule makeAssociativityRule(const Symbol &operation)
        {
            RewriteRule rule;
            rule.leftHand = e::makePatternTerm(operation, {e::makePatternTerm(operation, {x, y}), z});
            rule.rightHand = e::makePatternTerm(operation, {x, e::makePatternTerm(operation, {y, z})});
            return rule;
        }

        static RewriteRule makeDistributivityRule(const Symbol &operation1, const Symbol &operation2)
        {
            RewriteRule rule;
            assert(operation1 != operation2);
            rule.leftHand = e::makePatternTerm(operation1, {x, e::makePatternTerm(operation2, {y, z})});
            rule.rightHand = e::makePatternTerm(operation2, {e::makePatternTerm(operation1, {x, y}), e::makePatternTerm(operation1, {x, z})});
            return rule;
        }
    };

private:

    HashMap<ClassId, Vector<Hint::Ptr>> hints;
    Hint::Ptr question;

    // each level will contain a number of expressions to work with
    Vector<HashSet<ClassId>> questClasses;
    int currentQuest = 0;

    HashSet<String> shownOperations;

    const HashSet<Symbol> allTerms = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
        "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};

    const HashSet<Symbol> allOperations = {":>", "|>", "-<", ">-", "=<<", ">>=", "-<<", ".-", "|-", "-|", "~>", "<~>",
        "~~>", "?", "!", "~", "::", "@", "#", "$", "&", "."};

    e::Graph eGraph;

    Random random;

    int score = 0;
    const int failedAttemptScore = -1;
    const int validAnswerScore = 3;
    HashSet<String> acceptedAnswers;
};
