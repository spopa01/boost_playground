//http://stackoverflow.com/questions/10331264/attempting-to-parse-sql-like-statement-with-boost-spirit
//http://stackoverflow.com/questions/12627963/parsing-sql-queries-in-c-using-boost-spirit
//http://stackoverflow.com/questions/23519853/unable-to-parse-sql-type-where-condition-using-boostspiritqi

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

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

//the statement : SELECT columns FROM table WHERE conditions

//columns
using basic_column = std::string;
using basic_columns = std::vector<basic_column>;

//table
using basic_table = std::string;

//condition(s)
using basic_field = std::string;
enum basic_op { op_eq, op_neq };

struct null{};
using basic_value = boost::variant<null, int, std::string>;

struct basic_condition{
  basic_field   field_;
  basic_op      op_;
  basic_value   value_;
};

using basic_conditions = std::vector<basic_condition>;

//

struct basic_select{
  basic_columns columns_;
  basic_table table_;
  boost::optional<basic_conditions> conditions_;
};

BOOST_FUSION_ADAPT_STRUCT(
  basic_condition,
  (basic_field, field_)
  (basic_op, op_)
  (basic_value, value_)
)

BOOST_FUSION_ADAPT_STRUCT(
  basic_select,
  (basic_columns, columns_)
  (basic_table, table_)
  (boost::optional<basic_conditions>, conditions_)
)

std::ostream& operator<<(std::ostream& os, basic_columns const& columns){
  for(auto& col : columns){ os << col << " "; }
  return os;
}

std::ostream& operator<<(std::ostream& os, null){
  return os << "null";
}

std::ostream& operator<<(std::ostream& os, basic_op const& o){
  switch( o ){
    case op_eq: return os << "==";
    case op_neq: return os << "!=";
  }
  return os << "?";
}

std::ostream& operator<<(std::ostream& os, basic_conditions const& conditions){
  for(auto& cond : conditions){
    os  << "[ Fld{" << cond.field_ 
        << "} Op{" << cond.op_ 
        << "} Value{" << cond.value_
        << "} ]";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, basic_select const& select){
  os  << "\nSELECT: " << select.columns_
      << "\nFROM: " << select.table_;
  if( select.conditions_ )
    os  << "\nWHERE: " << *select.conditions_;
  return os << "\n";
}

//parsing using synthesized attributes...
template<typename Iterator>
struct basic_select_grammar : qi::grammar<Iterator, basic_select(), ascii::space_type>{
  basic_select_grammar() : basic_select_grammar::base_type(expression_){
    using namespace qi;
 
    /* Note: you can use lexeme or remove the skipper from the rule in order to inhibit skipping WS */

    ident_ = lexeme [ alpha >> *alnum ];   //columns, table
    strlit_ = lexeme ["'" >> *~char_("'") >> "'"];  //string literal, like: 'string'
    nulllit_ = no_case["null" >> attr(null())]; //

    field_ = ident_;
    op_token.add
      ("==", op_eq)
      ("!=", op_neq);
    op_ = no_case[op_token];
    value_ = int_ | strlit_ | nulllit_;

    condition_ = (field_ >> op_ >> value_);

    columns_ = (no_case["select"] >> (ident_ % ','));
    table_ = (no_case["from"] >> ident_);
    conditions_ = (no_case["where"] >> (condition_ % no_case["and"]));

    expression_  = columns_ >> table_ >> -conditions_ >> ';';
  }
  
  //aux
  qi::rule<Iterator, std::string(),     ascii::space_type> ident_;
  qi::rule<Iterator, std::string(),     ascii::space_type> strlit_;
  qi::rule<Iterator, null(),            ascii::space_type> nulllit_;
 
  //condition
  qi::rule<Iterator, basic_field(),     ascii::space_type> field_;
  qi::symbols<char, basic_op> op_token;
  qi::rule<Iterator, basic_op(),        ascii::space_type> op_;
  qi::rule<Iterator, basic_value(),     ascii::space_type> value_;

  qi::rule<Iterator, basic_condition(), ascii::space_type> condition_;
  
  //parts
  qi::rule<Iterator, basic_columns(),   ascii::space_type> columns_;
  qi::rule<Iterator, basic_table(),     ascii::space_type> table_;
  qi::rule<Iterator, basic_conditions(),ascii::space_type> conditions_;
  
  //basic select
  qi::rule<Iterator, basic_select(),    ascii::space_type> expression_;
};

//g++ file.cpp -std=c++11

int main( /* ... */ ){
  std::cout << "\n";

  std::string line;
  while (std::getline(std::cin, line)){
    if (line.empty()) break;

    auto iter = line.begin();
    auto end = line.end();

    ascii::space_type ws;
    basic_select_grammar<std::string::iterator> gram;

    basic_select se;
    if (phrase_parse(iter, end, gram, ws, se) && iter == end){
      std::cout << "Parsing succeeded - result: " << se << "\n";
    }else{
      std::string rest(iter, end);
      std::cout << "Parsing failed - stopped at: \" " << rest << "\"\n";
    }
  }

  std::cout << "Bye... :-) \n";
  return 0;
}
