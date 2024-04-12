#pragma once

#include "Common.h"
#include "EGraph.h"

#include "tao/pegtl.hpp"
#include "tao/pegtl/contrib/parse_tree.hpp"

namespace Parser
{
namespace Ast
{
    using namespace tao::pegtl;
    using namespace tao::pegtl::parse_tree;

    using Symbol = plus<ascii::ranges<'a', 'z', 'A', 'Z', '0', '9'>>;
    struct Term : Symbol {};
    struct PatternVariable : seq<one<'$'>, Symbol> {};
    struct Operation : sor<
        // :> |> -< >- =<< -<< |- -| ~> <~>  ~~> ? ! ~ :: @ # $ & .
        string<':', '>'>, string<'|', '>'>, string<'-', '<'>, string<'>', '-'>,
        string<'=', '<', '<'>, string<'-', '<', '<'>, string<'|', '-'>, string<'-', '|'>,
        string<'~', '>'>, string<'<', '~', '>'>, string<'~', '~', '>'>,
        string<'?'>, string<'!'>, string<'~'>, string<':', ':'>,
        string<'@'>, string<'#'>, string<'$'>, string<'&'>, string<'.'>,
        // ⇌ ⥢ ⥤ ⥃ ⥄ ⤝ ⤞ ⬷ ⤐ ➻ ⇜ ⇝ ↜ ↝ ↫ ↬ ⬸ ⤑ ⤙ ⤚ ⥈ ⬿ ⤳ ⥊ ⥐ ↽ ⇀ ⤾ ⤿ ⤸ ⤹ ⤻ ∴ ∵ ∷ ∺ ∻ ≀ ⤜
        utf8::one<0x21cc>, utf8::one<0x2962>, utf8::one<0x2964>, utf8::one<0x2943>,
        utf8::one<0x2944>, utf8::one<0x291d>, utf8::one<0x291e>, utf8::one<0x2b37>,
        utf8::one<0x2910>, utf8::one<0x27bb>, utf8::one<0x21dc>, utf8::one<0x21dd>,
        utf8::one<0x219c>, utf8::one<0x219d>, utf8::one<0x21ab>, utf8::one<0x21ac>,
        utf8::one<0x2b38>, utf8::one<0x2911>, utf8::one<0x2919>, utf8::one<0x291a>,
        utf8::one<0x2948>, utf8::one<0x2b3f>, utf8::one<0x2933>, utf8::one<0x294a>,
        utf8::one<0x2950>, utf8::one<0x21bd>, utf8::one<0x21c0>, utf8::one<0x293e>,
        utf8::one<0x293f>, utf8::one<0x2938>, utf8::one<0x2939>, utf8::one<0x293b>,
        utf8::one<0x2234>, utf8::one<0x2235>, utf8::one<0x2237>, utf8::one<0x223a>,
        utf8::one<0x223b>, utf8::one<0x2240>, utf8::one<0x291c>> {};

    struct Arrow : string<'=', '>'> {};
    struct Spacing : star<space> {};

    struct Expression;
    struct BracketExpression : if_must<one<'('>, Spacing, Expression, Spacing, one<')'>> {};
    struct Value : sor<PatternVariable, Term, BracketExpression> {};

    template <typename S, typename O>
    struct LeftAssociative : seq<S, Spacing, star_must<O, Spacing, S, Spacing>> {};

    struct Expression : LeftAssociative<Value, Operation> {};
    struct RewriteRuleOrExpression : LeftAssociative<Expression, Arrow> {};

    struct Grammar : must<Spacing, RewriteRuleOrExpression, eof> {};

    template <typename Rule>
    struct Selector : selector<Rule,
        store_content::on<Term, PatternVariable, Operation>,
        fold_one::on<Value, Expression>,
        discard_empty::on<Arrow>> {};

    struct Node : basic_node<Node> {};
} // namespace Ast

// if customOperationSymbol is not empty, then all operation names
// in the expression will be replaced with it, which is useful for describing
// rewrite rules for specific operations in a template-like string format
e::Pattern makePattern(const Ast::Node &astNode, const e::Symbol &customOperationSymbol)
{
    if (astNode.is_root())
    {
        assert(!astNode.children.empty());
        return makePattern(*astNode.children.front(), customOperationSymbol);
    }
    else if (astNode.is_type<Ast::Expression>())
    {
        e::PatternTerm term;

        assert(astNode.children.size() >= 3);
        for (const auto &childNode : astNode.children)
        {
            if (childNode->is_type<Ast::Operation>())
            {
                if (term.arguments.size() == 2)
                {
                    // here we found something like a + b + c, so we'll proceed
                    // assuming left-associativity, and transform it into (a + b) + c,
                    // for that, create a child node out of the existing name
                    // and arguments, update own name and use the newly created child
                    // as the first and the only argument, then proceed parsing others:

                    auto childTerm = e::PatternTerm(term.name, move(term.arguments));
                    // term.name = ...; happens anyway
                    term.arguments = {move(childTerm)};
                }

                term.name = customOperationSymbol.empty() ?
                    childNode->string() : customOperationSymbol;
            }
            else
            {
                term.arguments.push_back(makePattern(*childNode, customOperationSymbol));
            }
        }

        return term;
    }
    else if (astNode.is_type<Ast::Term>())
    {
        e::PatternTerm term;
        term.name = astNode.string();
        return term;
    }
    else if (astNode.is_type<Ast::PatternVariable>())
    {
        e::PatternVariable variable = astNode.string();
        return variable;
    }

    assert(false);
}

e::RewriteRule makeRewriteRule(const Ast::Node &astNode, const e::Symbol &customOperationSymbol)
{
    assert(astNode.is_root());
    assert(astNode.children.size() == 2);
    e::RewriteRule rule;
    rule.leftHand = makePattern(*astNode.children.front(), customOperationSymbol);
    rule.rightHand = makePattern(*astNode.children.back(), customOperationSymbol);
    return rule;
}

e::RewriteRule makeRewriteRule(const std::string &expression, const std::string &customOperationSymbol)
{
    using namespace tao::pegtl;
    string_input input(expression, "");
    const auto node = parse_tree::parse<Ast::Grammar, Ast::Node, Ast::Selector>(input);
    return makeRewriteRule(*node, customOperationSymbol);
}

e::Pattern makePattern(const std::string &expression)
{
    using namespace tao::pegtl;
    string_input input(expression, "");
    const auto node = parse_tree::parse<Ast::Grammar, Ast::Node, Ast::Selector>(input);
    return makePattern(*node, {});
}

String formatPatternTerm(const e::PatternTerm &patternTerm, bool wrapWithBrackets = true)
{
    // we only generate binary operators
    if (patternTerm.arguments.size() == 2)
    {
        const auto result = formatPatternTerm(*patternTerm.arguments.front().term) +
            " " + patternTerm.name + " " + formatPatternTerm(*patternTerm.arguments.back().term);
        return wrapWithBrackets ? ("(" + result + ")") : result;
    }

    return patternTerm.name;
}
} // namespace Language
