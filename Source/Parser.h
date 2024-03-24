#pragma once

#include "Common.h"

#include "../ThirdParty/e-graph/EGraph.h"

#include "tao/pegtl.hpp"
#include "tao/pegtl/contrib/parse_tree.hpp"
namespace peg = tao::pegtl;
namespace tree = peg::parse_tree;

// defines a simple language:
// latin letters are variables,
// various special symbols and arrows are binary operators,
// all operators are left-associative and have no precedence,
// bracket expressions are supported;
// valid expressions look like:
// b :: (a |> (a @ c))
// (b -< c) <~> (a . c)
// (a $ c) ~> b

namespace dsl
{
    using namespace peg;

    struct spacing : star<space> {};

    template <typename S, typename O>
    struct leftAssociative : seq<S, spacing, star_must<O, spacing, S, spacing>> {};

    struct variable : ascii::ranges<'a', 'z', 'A', 'Z'> {};
    struct operation : sor<
        string<':', '>'>, string<'|', '>'>, string<'-', '<'>, string<'>', '-'>,
        string<'=', '<', '<'>, string<'>', '>', '='>, string<'-', '<', '<'>,
        string<'.', '='>, string<'.', '-'>, string<'|', '-'>, string<'-', '|'>,
        string<'~', '>'>, string<'<', '~', '>'>, string<'~', '~', '>'>,
        string<'?'>, string<'!'>, string<'~'>, string<':', ':'>, string<'@'>,
        string<'#'>, string<'$'>, string<'&'>, string<'.'>> {};

    struct expression;
    struct bracketExpression : if_must<one<'('>, spacing, expression, spacing, one<')'>> {};
    struct value : sor<variable, bracketExpression> {};

    struct expression : leftAssociative<value, operation> {};

    struct grammar : must<spacing, expression, eof> {};

    template <typename Rule>
    struct selector : tree::selector<Rule,
        tree::store_content::on<variable, operation>,
        tree::fold_one::on<value, expression>> {};

    struct node : tree::basic_node<node> {};
}

namespace Parser
{
    using Input = peg::string_input<peg::tracking_mode::eager, peg::eol::lf_crlf, std::string>;

    template <typename Input>
    int validate(Input &input)
    {
        return peg::parse<dsl::grammar>(input);
    }

    template <typename Input>
    auto parse(Input &input)
    {
        return tree::parse<dsl::grammar, dsl::node, dsl::selector>(input);
    }

    static void convertAstToPattern(e::PatternTerm &outPattern, const dsl::node &astNode)
    {
        if (astNode.is_root())
        {
            if (astNode.children.empty())
            {
                return;
            }

            for (auto &childNode : astNode.children.front()->children)
            {
                convertAstToPattern(outPattern, *childNode);
            }
        }
        else if (astNode.is_type<dsl::expression>())
        {
            outPattern.arguments.push_back(e::PatternTerm());

            for (auto &childNode : astNode.children)
            {
                convertAstToPattern(std::get<e::PatternTerm>(outPattern.arguments.back()), *childNode);
            }
        }
        else if (astNode.is_type<dsl::operation>())
        {
            outPattern.name = astNode.string();
        }
        else if (astNode.is_type<dsl::variable>())
        {
            outPattern.arguments.push_back(astNode.string());
        }
        else
        {
            assert(false);
        }
    }

    static String formatPatternTerm(const e::Pattern &pattern, bool wrapWithBrackets = true)
    {
        if (const auto *patternVariable = std::get_if<e::Symbol>(&pattern))
        {
            return *patternVariable;
        }
        else if (const auto *patternTerm = std::get_if<e::PatternTerm>(&pattern))
        {
            // we only generate binary operators
            assert(patternTerm->arguments.size() == 2);
            const auto result = formatPatternTerm(patternTerm->arguments.front()) +
                                " " + patternTerm->name + " " +
                                formatPatternTerm(patternTerm->arguments.back());
            return wrapWithBrackets ? ("(" + result + ")") : result;
        }

        assert(false);
        return {};
    }
}

/*
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>

int main(int argc, char **argv)
{
    if (argc < 1)
    {
        // example: test "b :: (a |> (a @ c))" | dot -Tsvg -o parse_tree.svg
        return 1;
    }

    argv_input in(argv, 1);

    try
    {
        if (const auto root = Parser::parse(in))
        {
            //e::PatternTerm pattern;
            //Parser::convertAstToPattern(pattern, *root.get());
            //std::cout << Parser::formatPatternTerm(pattern, false) << std::endl;

            parse_tree::print_dot(std::cout, *root);
            return 0;
        }
    }
    catch( const parse_error &e )
    {
        const auto p = e.positions().front();
        std::cerr << e.what() << '\n'
                  << in.line_at( p ) << '\n'
                  << std::setw( p.column ) << '^' << std::endl;
        return 1;
    }

    std::cerr << "parse error" << std::endl;
    return 1;
}
*/
