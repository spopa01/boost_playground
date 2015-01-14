#define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <iostream>
#include <string>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

/*
Parsing Expression Grammar
- Formal grammar for describing a formal language in terms of a set of rules used to recognize strings of this language
- Does not require a tokenization stage
- Similar to Extended Backus-Naur Form (EBNF)
- Unlike (E)BNF, PEG's are not ambiguous, there is exactly one valid parse tree for each PEG
- Any PEG can be directly represented as a recursive-descent parser

PEG (example):

exprression <- term (('+' term) / ('-' term))*
term        <- factor (('*' factor) / ('/' factor))*
factor      <- number / '(' expr ')' / ('-' factor) / ('+' factor)
number      <- [0-9]+

A recursive descent parser is a top-down parser built from a set of mutually-recursive functions, each representing one of the grammar elements.
Thus the structure of the resulting program closely mirrors that of the grammar it recognizes.
*/

/*
Operators.

Description             PEG     Spirit Qi
Sequence                a b     a >> b
Alternative             a | b   a | b
Zero of more (Kleene)   a*      *a
One or more (Plus)      a+      +a
Optional                a?      -a

// and&not predicate can provide basic look-ahead
And-predicate           &a      &a          it matchs a without consuming it
Not-predicate           !a      !a          if a does match the parser is succesful without consuming a

Difference                      a - b       e.g. '"' >> *(char_ - '"') >> '"' matches literal string "string"
Expectation                     a > b       a MUST be followed by b; no backtracking allowed. a nonmatch throws exception_failure<iter>
List                            a % b       shortcut for a >> *( b >> a); int_ % ',' matches any sequence of integers sparated by ',' eg: "9,2,4,...,-2"
Permutation                     a ^ b       parse a,b in any order, each element can occur 0:1 times, e.g. *(char_('A') ^ 'C' ^ 'T' ^ 'G') matches "ACTGGCTAGACT"
Sequential Or                   a || b      shortcut for: a >> -b | b  , e.g. int_ || ('.' >> int_)  matches any of "123.12", ".456", "123"
*/

//parsing using synthesized attributes...
template<typename Iterator>
struct calc_grammar : qi::grammar<Iterator, int(), ascii::space_type>{
  calc_grammar() : calc_grammar::base_type(expression){
    qi::uint_type uint_; //parser

    qi::_val_type _val; //the enclosing rule's synthesized attribute
    qi::_1_type _1;     //first attribute of the parser

    expression = term [_val = _1] >> *( ('+' >> term [_val += _1]) | ('-' >> term [_val -= _1]) );

    term = factor [_val = _1] >> *( ('*' >> factor [_val *= _1]) | ('/' >> factor [_val /= _1]) );

    factor = uint_ [_val = _1] | '(' >> expression [_val = _1] >> ')' | ('-' >> factor [_val = -_1]) | ('+' >> factor [_val = +_1]);
  }

  qi::rule<Iterator, int(), ascii::space_type> expression, term, factor;
};

//g++ file.cpp -std=c++11

int main( /* ... */ ){

  std::string line;
  while (std::getline(std::cin, line)){
    if (line.empty()) break;

    auto iter = line.begin();
    auto end = line.end();

    ascii::space_type ws;
    calc_grammar<std::string::iterator> gram;

    int res;
    if (phrase_parse(iter, end, gram, ws, res) && iter == end){
      std::cout << "Parsing succeeded - result: " << res << "\n\n";
    }else{
      std::string rest(iter, end);
      std::cout << "Parsing failed - stopped at: \" " << rest << "\"\n\n";
    }
  }

  std::cout << "Bye... :-) \n\n";
  return 0;
}
