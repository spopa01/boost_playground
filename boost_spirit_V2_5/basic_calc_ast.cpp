#define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <iostream>
#include <string>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

/*
PEG (example):
exprression <- term (('+' term) / ('-' term))*
term        <- factor (('*' factor) / ('/' factor))*
factor      <- number / '(' expr ')' / ('-' factor) / ('+' factor)
number      <- [0-9]+
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

struct nil{}; //just a tag
using number = unsigned int;
struct signed_number;
struct program;

typedef boost::variant<
        nil
      , number
      , boost::recursive_wrapper<signed_number>
      , boosr::recursive_wrapper<program>
    >
  operand;

struct signed_number{
  char sign_;
  operand operand_;
};

...

//parsing using semantic actions...
template<typename Iterator>
struct calc_grammar : qi::grammar<Iterator, program(), ascii::space_type>{
  calc_grammar() : calc_grammar::base_type(expression){
    qi::uint_type uint_; //parser

    qi::_val_type _val; //the enclosing rule's synthesized attribute
    qi::_1_type _1;     //first attribute of the parser

    expression = term >> *( ('+' >> term) | ('-' >> term) );

    term = factor >> *( ('*' >> factor) | ('/' >> factor) );

    factor = uint_ | '(' >> expression >> ')' | ('-' >> factor) | ('+' >> factor);
  }

  qi::rule<Iterator, program(), ascii::space_type> expression;
  qi::rule<Iterator, program(), ascii::space_type> term;
  qi::rule<Iterator, operand(), ascii::space_type> factor;
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
