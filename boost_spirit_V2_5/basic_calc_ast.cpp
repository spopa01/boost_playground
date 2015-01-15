#define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <boost/variant/recursive_variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <string>
#include <list>

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

// the AST

using calc_number = unsigned int;
struct calc_signed_number;
struct calc_program;

using calc_operand =
        boost::variant<
            calc_number
          , boost::recursive_wrapper<calc_signed_number>
          , boost::recursive_wrapper<calc_program>
        >;

struct calc_signed_number{
  char sign_;
  calc_operand operand_;
};

struct calc_operation{
  char operator_;
  calc_operand operand_;
};

struct calc_program{
  calc_operand first_;
  std::list<calc_operation> rest_;
};

BOOST_FUSION_ADAPT_STRUCT(
  calc_signed_number,
  (char, sign_)
  (calc_operand, operand_)
)

BOOST_FUSION_ADAPT_STRUCT(
  calc_operation,
  (char, operator_)
  (calc_operand, operand_)
)

BOOST_FUSION_ADAPT_STRUCT(
  calc_program,
  (calc_operand, first_)
  (std::list<calc_operation>, rest_)
)

//the evaluator

struct calc_program_eval{
  typedef int result_type;

  int operator()(calc_number n) const { return n; }

  int operator()(calc_signed_number const& x) const {
    int rhs = boost::apply_visitor(*this, x.operand_);
    switch(x.sign_){
      case '-' : return -rhs;
      case '+' : return rhs;
    }
    BOOST_ASSERT(0);//it should not get here
    return 0;
  }

  int operator()(calc_operation const& x, int lhs) const {
    int rhs = boost::apply_visitor(*this, x.operand_);
    switch(x.operator_){
      case '-' : return lhs - rhs;
      case '+' : return lhs + rhs;
      case '*' : return lhs * rhs;
      case '/' : return lhs / rhs;
    }
    BOOST_ASSERT(0);//it should not get here
    return 0;
  }

  int operator()(calc_program const& x) const {
    int state = boost::apply_visitor(*this, x.first_);
    BOOST_FOREACH(calc_operation const& oper, x.rest_){
      state = (*this)(oper, state);
    }
    return state;
  }
};

//the grammar - parsing using semantic actions...

template<typename Iterator>
struct calc_grammar : qi::grammar<Iterator, calc_program(), ascii::space_type>{
  calc_grammar() : calc_grammar::base_type(expression){
    qi::uint_type uint_; //parser
    qi::char_type char_;

    qi::_val_type _val; //the enclosing rule's synthesized attribute
    qi::_1_type _1;     //first attribute of the parser

    expression = term >> *( (char_('+') >> term) | (char_('-') >> term) );

    term = factor >> *( (char_('*') >> factor) | (char_('/') >> factor) );

    factor = uint_ | '(' >> expression >> ')' | (char_('-') >> factor) | (char_('+') >> factor);
  }

  qi::rule<Iterator, calc_program(), ascii::space_type> expression;
  qi::rule<Iterator, calc_program(), ascii::space_type> term;
  qi::rule<Iterator, calc_operand(), ascii::space_type> factor;
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
    calc_program prog;
    calc_program_eval eval;

    if (phrase_parse(iter, end, gram, ws, prog) && iter == end){
      std::cout << "Parsing succeeded - result: " << eval(prog) << "\n\n";
    }else{
      std::string rest(iter, end);
      std::cout << "Parsing failed - stopped at: \" " << rest << "\"\n\n";
    }
  }

  std::cout << "Bye... :-) \n\n";
  return 0;
}
