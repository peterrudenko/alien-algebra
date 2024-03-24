#pragma once

#include "Common.h"
#include "Random.h"

#include "../ThirdParty/e-graph/EGraph.h"

// A helper class used to extract random expressions
// from the e-graph along with their class ids and some meta info,
// so it's more convenient to manage them: group/filter/etc.

class HintsExtractor final
{
public:

    HintsExtractor(const e::Graph &eGraph, Random &random) :
        eGraph(eGraph), random(random) {}

    struct AstNode final
    {
        AstNode() = default;
        explicit AstNode(const e::Symbol &name) :
            name(name) {}

        e::Symbol name;
        Vector<AstNode> children;
    };

    struct Expression final
    {
        using Ptr = SharedPointer<Expression>;

        Expression() = delete;
        Expression(const Expression &other) = default;
        explicit Expression(const e::Symbol &rootSymbol)
        {
            this->rootNode = AstNode(rootSymbol);
        }

        e::ClassId leafId = 0;
        e::ClassId rootId = 0;

        int astDepth = 0;
        String formatted;
        HashSet<e::Symbol> usedSymbols;

        AstNode rootNode;

        void collectInfo()
        {
            this->astDepth = Expression::getNodeDepth(this->rootNode);
            this->formatted = Expression::formatNode(this->rootNode, false);
        }

    private:

        static String formatNode(const AstNode &node, bool wrapWithBrackets = true)
        {
            // we only generate binary operators and terms
            assert(node.children.size() == 2 || node.children.empty());
            if (node.children.size() == 2)
            {
                const auto result = formatNode(node.children.front()) +
                                    " " + node.name + " " + formatNode(node.children.back());
                return wrapWithBrackets ? ("(" + result + ")") : result;
            }

            return node.name;
        }

        static int getNodeDepth(const AstNode &node)
        {
            int depth = 0;
            for (auto &child : node.children)
            {
                depth = std::max(depth, getNodeDepth(child));
            }
            return depth + 1;
        }
    };

    Vector<Expression::Ptr> extract()
    {
        // all expressions' roots are basically leaf ids, aka non-canonical id,
        // a straightforward way to find them along with their terms
        // is to iterate e-class parents

        using LeafTerms = HashMap<e::ClassId, e::Term::Ptr>;

        LeafTerms leafTerms;
        for (const auto &[classId, classPtr] : this->eGraph.getClasses())
        {
            for (const auto &parent : classPtr->getParents())
            {
                leafTerms[parent.termId] = parent.term;
            }
        }

        // now collect some full trees of expressions for each term
        HashMap<String, Expression::Ptr> expressions;
        for (const auto &[leafId, termPtr] : leafTerms)
        {
            // I don't have good ideas on how to do exhaustive search here,
            // so instead will just pick random routes many times and deduplicate
            for (int i = 0; i < 100; ++i)
            {
                Expression::Ptr expression = make<Expression>(termPtr->getName());
                expression->leafId = leafId;
                expression->rootId = this->eGraph.find(leafId);

                const auto hasResult = this->collectExpressions(expression, expression->rootNode, termPtr);
                if (hasResult)
                {
                    expression->collectInfo();
                    expressions[expression->formatted] = expression;
                }
            }
        }

        Vector<Expression::Ptr> result;
        for (const auto &[formatted, expression] : expressions)
        {
            result.push_back(expression);
        }

        return result;
    }

    bool collectExpressions(Expression::Ptr expression, AstNode &parentAstNode, e::Term::Ptr term)
    {
        bool hasResult = true;

        expression->usedSymbols.insert(term->getName());

        for (const auto childClassId : term->getChildrenIds())
        {
            for (const auto &[classId, classPtr] : this->eGraph.getClasses())
            {
                if (classId != childClassId)
                {
                    continue;
                }

                const auto &subTerms = classPtr->getTerms();
                if (subTerms.empty())
                {
                    assert(false); // the e-graph has probably not been rebuilt
                    return false;
                }

                const auto randomSubTerm = this->random.pickRandomElement(subTerms);
                assert(randomSubTerm->getChildrenIds().size() == 2 || randomSubTerm->getChildrenIds().empty());

                // I'm not 100% sure if this is a correct condition,
                // but hopefully it should work: if we've already added that symbol,
                // and it has more sub-terms, we're likely to end up in a loop. I guess.
                const bool isLoop = !randomSubTerm->getChildrenIds().empty() &&
                                    expression->usedSymbols.contains(randomSubTerm->getName());

                if (isLoop)
                {
                    return false;
                }

                parentAstNode.children.push_back(AstNode(randomSubTerm->getName()));
                hasResult = hasResult && this->collectExpressions(expression,
                    parentAstNode.children.back(), randomSubTerm);
            }
        }

        return hasResult;
    }

private:

    const e::Graph &eGraph;

    Random &random;
};
