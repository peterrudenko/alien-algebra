#pragma once

#include "Common.h"
#include "Random.h"
#include "EGraph.h"

// Helper classes used to extract random expressions
// from the e-graph along with their class ids and some meta info,
// so it's more convenient to manage them: group/filter/etc.

struct Hint final
{
    using Ptr = SharedPointer<Hint>;

    struct AstNode final
    {
        AstNode() = default;
        explicit AstNode(const e::Symbol &name) :
            name(name) {}

        e::Symbol name;
        Vector<AstNode> children;
    };

    Hint() = delete;
    Hint(const Hint &other) = default;
    explicit Hint(const e::Symbol &rootSymbol)
    {
        this->rootNode = AstNode(rootSymbol);
    }

    e::ClassId rootId = 0;

    int astDepth = 0;
    String formatted;

    HashSet<e::ClassId> usedLeafIds;
    HashSet<e::Symbol> usedSymbols;
    HashSet<e::Symbol> usedTermSymbols;
    HashSet<e::Symbol> usedOperationSymbols;

    int getNumNewOperations(const HashSet<e::Symbol> &knownOperations)
    {
        int result = 0;
        for (const auto &usedSymbol : this->usedOperationSymbols)
        {
            if (!contains(knownOperations, usedSymbol))
            {
                result++;
            }
        }
        return result;
    }

    Vector<e::Symbol> getNewOperations(const HashSet<e::Symbol> &knownOperations)
    {
        Vector<e::Symbol> result;
        for (const auto &usedSymbol : this->usedOperationSymbols)
        {
            if (!contains(knownOperations, usedSymbol))
            {
                result.push_back(usedSymbol);
            }
        }
        return result;
    }

    AstNode rootNode;

    void collectInfo()
    {
        this->astDepth = Hint::getNodeDepth(this->rootNode);
        this->formatted = Hint::formatNode(this->rootNode, false);
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

class HintsExtractor final
{
public:

    explicit HintsExtractor(const e::Graph &eGraph) :
        eGraph(eGraph) {}

    auto extract()
    {
        // collect some full trees of expressions for each term
        HashMap<String, Hint::Ptr> expressions;
        for (const auto &[termPtr, leafId] : this->eGraph.termsLookup)
        {
            // I don't have good ideas on how to do exhaustive search here,
            // so instead will just pick random routes many times and deduplicate;
            // this class has its own pseudo-random generator with the default seed,
            // so hints collection will always be the same on the same graph.
            for (int i = 0; i < 256; ++i)
            {
                Hint::Ptr expression = make<Hint>(termPtr->name);
                expression->rootId = this->eGraph.find(leafId);

                if (this->collectExpressions(expression, expression->rootNode, termPtr))
                {
                    expression->collectInfo();
                    expressions[expression->formatted] = expression;
                }
            }
        }

        HashMap<e::ClassId, Vector<Hint::Ptr>> result;
        for (const auto &[formatted, expression] : expressions)
        {
            if (!contains(result, expression->rootId))
            {
                result[expression->rootId] = {};
            }

            result[expression->rootId].push_back(expression);
        }

        return result;
    }

    bool collectExpressions(Hint::Ptr expression, Hint::AstNode &parentAstNode, e::Term::Ptr term)
    {
        bool hasResult = true;

        expression->usedLeafIds.insert(this->eGraph.termsLookup.at(term));
        expression->usedSymbols.insert(term->name);

        if (term->childrenIds.empty())
        {
            expression->usedTermSymbols.insert(term->name);
        }
        else
        {
            expression->usedOperationSymbols.insert(term->name);
        }

        for (const auto childClassId : term->childrenIds)
        {
            for (const auto &[classId, classPtr] : this->eGraph.classes)
            {
                if (classId != childClassId)
                {
                    continue;
                }

                if (classPtr->terms.empty())
                {
                    assert(false); // the e-graph has probably not been rebuilt
                    return false;
                }

                const auto randomSubTerm = this->random.pickOne(classPtr->terms);
                assert(randomSubTerm->childrenIds.size() == 2 || randomSubTerm->childrenIds.empty());

                // I'm not 100% sure if this is a correct condition,
                // but hopefully it should work: if we've already added that term,
                // and it is an operation (i.e. has more sub-terms),
                // we're likely to end up in a loop. I guess.
                const bool isLoop = !randomSubTerm->childrenIds.empty() &&
                    contains(expression->usedLeafIds, this->eGraph.termsLookup.at(randomSubTerm));

                if (isLoop)
                {
                    return false;
                }

                parentAstNode.children.push_back(Hint::AstNode(randomSubTerm->name));
                hasResult = hasResult && this->collectExpressions(expression,
                    parentAstNode.children.back(), randomSubTerm);
            }
        }

        return hasResult;
    }

private:

    const e::Graph &eGraph;

    Random random;
};
