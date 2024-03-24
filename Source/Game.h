#pragma once

#include "Common.h"
#include "Random.h"
#include "HintsExtractor.h"

#include "../ThirdParty/e-graph/EGraph.h"

class Game
{
public:

    Game() = default;
    virtual ~Game() = default;

    virtual void onShowHint(const String &hint) = 0;
    virtual void onShowQuesion(const String &question) = 0;
    virtual void onValidateAnswer(bool isValid) = 0;

protected:

    void start()
    {
        // here goes the quest generator

        // fixme pick randomly, depending on difficulty level
        // always pick a few terms, but a lot of operations?
        const auto termSymbols = this->allTerms;
        const auto operationSymbols = this->allOperations;

        Vector<e::ClassId> expressionIds;

        Vector<e::ClassId> termIds;
        for (const auto &term : termSymbols)
        {
            const auto termId = this->eGraph.addTerm(term);
            termIds.push_back(termId);
            expressionIds.push_back(termId);
        }

        // generate some random operations
        const auto numOperationsInGraph = 11; // todo random
        for (int i = 0; i < numOperationsInGraph; ++i)
        {
            const auto operation = this->random.pickRandomElement(operationSymbols);
            const auto expressionId1 = this->random.pickRandomElement(expressionIds);
            const auto expressionId2 = this->random.pickRandomElement(expressionIds);
            expressionIds.push_back(this->eGraph.addOperation(operation, {expressionId1, expressionId2}));
        }

        // make some of them equal randomly
        // (the more unites, the simpler final expressions will be)
        // after some experiments, 2 or 3 unites seem ok to me
        const auto numUnites = 2; // todo random, but not much
        for (int i = 0; i < numUnites; ++i)
        {
            const auto termId1 = this->random.pickRandomElement(expressionIds);
            const auto termId2 = this->random.pickRandomElement(expressionIds);
            this->eGraph.unite(termId1, termId2);
        }

        // assign random properties to random operations

        const auto x = e::Symbol("x");
        const auto y = e::Symbol("y");
        const auto z = e::Symbol("z");

        Vector<e::RewriteRule> rewrites;

        for (const auto &op : operationSymbols)
        {
            const bool isAssociative = this->random.getRandomBool();
            const bool isCommutative = this->random.getRandomBool();
            // fixme should not be the same operation
            const auto isDistributiveWith = this->random.pickRandomElements(operationSymbols, 1);

            // fixme these should not intersect
            // also they simplify final expressions a lot
            const auto identityTerms = this->random.pickRandomElements(termSymbols, 1);
            const auto zeroTerms = this->random.pickRandomElements(termSymbols, 1);
            const auto idempotentTerms = this->random.pickRandomElements(termSymbols, 1);

            // todo unary ops: de morgan's laws, negation/double negation laws?

            if (isAssociative)
            {
                // todo is associative only with those terms ...
                e::RewriteRule associativityRule;
                associativityRule.leftHand = e::makePatternTerm(op, {e::makePatternTerm(op, {x, y}), z});
                associativityRule.rightHand = e::makePatternTerm(op, {x, e::makePatternTerm(op, {y, z})});
                rewrites.push_back(move(associativityRule));
            }

            if (isCommutative)
            {
                // todo is commutative only with those terms ...
                e::RewriteRule commutativityRule;
                commutativityRule.leftHand = e::makePatternTerm(op, {x, y});
                commutativityRule.rightHand = e::makePatternTerm(op, {y, x});
                rewrites.push_back(move(commutativityRule));
            }

            // slows the rewriting way down
            for (const auto &op2 : isDistributiveWith)
            {
                if (op == op2)
                {
                    continue;
                }

                e::RewriteRule distributiveRule;
                distributiveRule.leftHand = e::makePatternTerm(op2, {x, e::makePatternTerm(op, {y, z})});
                distributiveRule.rightHand = e::makePatternTerm(op, {e::makePatternTerm(op2, {x, y}), e::makePatternTerm(op2, {x, z})});
                rewrites.push_back(move(distributiveRule));
            }

            for (const auto &identityTerm : identityTerms)
            {
                // todo in reverse order randomly? or force making it commutative
                e::RewriteRule identityRule;
                identityRule.leftHand = e::makePatternTerm(op, {x, e::makePatternTerm(identityTerm)});
                identityRule.rightHand = e::Symbol(x);
                rewrites.push_back(move(identityRule));
            }

            for (const auto &zeroTerm : zeroTerms)
            {
                // todo also always commutative?
                e::RewriteRule zeroRule;
                zeroRule.leftHand = e::makePatternTerm(op, {x, e::makePatternTerm(zeroTerm)});
                zeroRule.rightHand = e::makePatternTerm(zeroTerm);
                rewrites.push_back(move(zeroRule));
            }

            for (const auto &idempotentTerm : idempotentTerms)
            {
                e::RewriteRule idempotentRule;
                idempotentRule.leftHand = e::makePatternTerm(op, {e::makePatternTerm(idempotentTerm), e::makePatternTerm(idempotentTerm)});
                idempotentRule.rightHand = e::makePatternTerm(idempotentTerm);
                rewrites.push_back(move(idempotentRule));
            }
        }

        // rewrite a lot, rebuild the graph, aand we're good to go
        for (int i = 0; i < 10; ++i)
        {
            for (const auto &rule : rewrites)
            {
                this->eGraph.rewrite(rule);
            }
        }

        this->eGraph.restoreInvariants();

        // generating hints:
        HintsExtractor expressionExtractor(this->eGraph, this->random);
        const auto expressions = expressionExtractor.extract();

        HashMap<e::ClassId, Vector<HintsExtractor::Expression::Ptr>> groupedByEquality;
        for (const auto &expression : expressions)
        {
            if (!groupedByEquality.contains(expression->rootId))
            {
                groupedByEquality[expression->rootId] = {};
            }

            groupedByEquality[expression->rootId].push_back(expression);
        }

        for (const auto &it : groupedByEquality)
        {
            for (const auto &expr : it.second)
            {
                // try to filter out the most obvious rules: assoc/comm/etc
                if (it.second.front()->usedSymbols == expr->usedSymbols)
                {
                    continue;
                }

                const String hint = it.second.front()->formatted + " = " + expr->formatted;
                this->onShowHint(hint);
            }
        }
    }

    void showHints(const String &expression)
    {
        // todo
    }

    void validateAnswer(const String &expression)
    {
        // todo
    }

private:

    const Vector<e::Symbol> allTerms = {"a", "b", "c"}; //, "d", "e", "f"};
    const Vector<e::Symbol> allOperations = {":>", "|>", "-<", ">-",
        "=<<", ">>=", "-<<", ".=", ".-", "|-", "-|", "~>", "<~>", "~~>",
        "?", "!", "~", "::", "@", "#", "$", "&", "."};

    e::Graph eGraph;

    Random random;
};
