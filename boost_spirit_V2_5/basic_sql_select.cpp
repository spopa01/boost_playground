//http://stackoverflow.com/questions/10331264/attempting-to-parse-sql-like-statement-with-boost-spirit
//http://stackoverflow.com/questions/12627963/parsing-sql-queries-in-c-using-boost-spirit
//http://stackoverflow.com/questions/23519853/unable-to-parse-sql-type-where-condition-using-boostspiritqi

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/optional.hpp>

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

//the statement : SELECT select FROM from WHERE where
struct basic_select{
  std::vector<std::string> columns_;
  std::string table_;
  boost::optional<std::string> where_;
};

BOOST_FUSION_ADAPT_STRUCT(
  basic_select,
  (std::vector<std::string>, columns_)
  (std::string, table_)
  (boost::optional<std::string>, where_)
)

std::ostream& operator<<(std::ostream& os, std::vector<std::string> const& columns){
  for(auto& col : columns){ os << col << " "; }
  return os;
}

std::ostream& operator<<(std::ostream& os, basic_select const& se){
  os << "\nSELECT: " << se.columns_ << "\nFROM: " << se.table_ << (se.where_ ? ("\nWHERE: " + *se.where_) : "") << "\n";
  return os;
}

//parsing using synthesized attributes...
template<typename Iterator>
struct basic_select_grammar : qi::grammar<Iterator, basic_select(), ascii::space_type>{
  basic_select_grammar() : basic_select_grammar::base_type(expression){
    using namespace qi;

    /*

    */
  
    ident = lexeme [ alpha >> *alnum ]; //columns, table

    columns = no_case["select"] >> (ident % ',');

    table = no_case["from"] >> ident;

    where = no_case["where"] >> +(~char_(';')) >> ';';
  
    expression  = columns >> table >> (where | ';');
  }

  qi::rule<Iterator, basic_select(), ascii::space_type> expression;
  qi::rule<Iterator, std::vector<std::string>(), ascii::space_type> columns;
  qi::rule<Iterator, std::string(), ascii::space_type> table;
  qi::rule<Iterator, std::string(), ascii::space_type> where;
  qi::rule<Iterator, std::string(), ascii::space_type> ident;
};

/*
The WHERE part of the query is optional.
*/

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
