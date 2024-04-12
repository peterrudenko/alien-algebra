#pragma once

#include "Common.h"

#include "EGraph.h"

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
    struct left_associative : seq<S, spacing, star_must<O, spacing, S, spacing>> {};

    struct variable : plus<ascii::ranges<'a', 'z', 'A', 'Z'>> {};
    struct operation : sor<
        string<'=', '<', '<'>, string<'>', '>', '='>,
        string<'-', '<', '<'>, string<'<', '~', '>'>, string<'~', '~', '>'>,
        string<'~', '>'>, string<':', '>'>, string<'|', '>'>, string<'-', '<'>, string<'>', '-'>,
        string<'.', '='>, string<'.', '-'>, string<'|', '-'>, string<'-', '|'>,
        string<'?'>, string<'!'>, string<'~'>, string<':', ':'>, string<'@'>,
        string<'#'>, string<'$'>, string<'&'>, string<'.'>> {};

    struct expression;
    struct bracket_expression : if_must<one<'('>, spacing, expression, spacing, one<')'>> {};
    struct value : sor<variable, bracket_expression> {};

    struct expression : left_associative<value, operation> {};

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

            const auto *firstTerm = astNode.children.front().get();
            if (firstTerm->children.empty())
            {
                outPattern.name = firstTerm->string(); // a single variable expression
            }
            else
            {
                for (auto &childNode : firstTerm->children)
                {
                    convertAstToPattern(outPattern, *childNode);
                }
            }
        }
        else if (astNode.is_type<dsl::expression>())
        {
            outPattern.arguments.push_back({e::PatternTerm()});

            for (auto &childNode : astNode.children)
            {
                convertAstToPattern(*outPattern.arguments.back().term, *childNode);
            }
        }
        else if (astNode.is_type<dsl::operation>())
        {
            if (!outPattern.name.empty())
            {
                // here we found something like a + b + c, so we'll proceed
                // assuming left-associativity, and transform it into (a + b) + c,
                // for that, create a child node out of the existing name
                // and arguments, update own name and use the newly created child
                // as the first and the only argument, then proceed parsing others:
                assert(outPattern.arguments.size() == 2);

                auto childTerm = e::PatternTerm(outPattern.name, move(outPattern.arguments));
                // outPattern.name = astNode.string(); happens anyway
                outPattern.arguments = {move(childTerm)};
            }

            outPattern.name = astNode.string();
        }
        else if (astNode.is_type<dsl::variable>())
        {
            assert(outPattern.arguments.size() < 2);
            e::PatternTerm term(astNode.string(), {});
            outPattern.arguments.push_back(move(term));
        }
        else
        {
            assert(false);
        }
    }

    static String formatPatternTerm(const e::PatternTerm &patternTerm, bool wrapWithBrackets = true)
    {
        // we only generate binary operators
        if (patternTerm.arguments.size() == 2)
        {
            const auto result = formatPatternTerm(*patternTerm.arguments.front().term) +
                                " " + patternTerm.name + " " +
                                formatPatternTerm(*patternTerm.arguments.back().term);
            return wrapWithBrackets ? ("(" + result + ")") : result;
        }

        return patternTerm.name;
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
