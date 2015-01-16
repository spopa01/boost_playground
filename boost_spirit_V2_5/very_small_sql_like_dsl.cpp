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


/*
//http://stackoverflow.com/questions/14548592/boost-spirit-implement-small-one-line-dsl-on-a-server-application

DSL syntax:

WHERE [NOT] <condition> [ AND | OR <condition> ] <command> [parameters]

where there are 2 possible conditions:
  <condition> = <property> = "value"
  <condition> = <property> like <regexp>

and 2 possible commands:
  print <property> [, <property> [...]]
  set <property> = "value" [, <property> = "value" [...]]

Examples:
where currency like "GBP|USD" set logging = 1, logfile = "myfile"
where not status == "ok" print ident, errorMessage
*/

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <utility>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phx   = boost::phoenix;

//...

using pattern = std::string;
struct regex{
  explicit regex(std::string const& pattern) : pattern_(pattern) {}
  pattern pattern_;
};

using property = std::string;
using value = boost::variant<double, int, std::string, regex>;

struct condition{
  bool negated_;
  property property_;
  value value_;
};

enum logicOp{logicFirst, logicAnd, logicOr};
struct filter{
  logicOp op_;
  condition condition_;
};

using assignment = std::pair<property, value>;
struct set_command{std::vector<assignment> assignments_;};
struct print_command{std::vector<property> properties_;};
using command = boost::variant<set_command, print_command>;

struct statement{
  std::vector<filter> filters_;
  command command_;
};

//...

//I should really start using spirit::karma...

std::ostream& operator<<(std::ostream& os, statement const& s ){ return os; }

//...

BOOST_FUSION_ADAPT_STRUCT( regex, (std::string, pattern_) )

BOOST_FUSION_ADAPT_STRUCT( condition, (bool, negated_) (property, property_) (value, value_) )

BOOST_FUSION_ADAPT_STRUCT( filter, (logicOp, op_) (condition, condition_) )

BOOST_FUSION_ADAPT_STRUCT( set_command, (std::vector<assignment>, assignments_) )
BOOST_FUSION_ADAPT_STRUCT( print_command, (std::vector<property>, properties_) )

BOOST_FUSION_ADAPT_STRUCT( statement, (std::vector<filter>, filters_) (command, command_) )

//...

template<typename Iterator>
struct dsl_grammar : qi::grammar<Iterator, statement(), ascii::space_type>{
  dsl_grammar() : dsl_grammar::base_type(expression_){
    using namespace qi;

    /* Note: strings should be able to contain quotes, so \ is escape char */
    strlit_ = "'" >> *(  (lit('\\') >> char_) | ~char_("'") ) > "'";  //string literal, like: 'string'

    property_ =  alpha >> *alnum;

    /* a bit of synth attrs magic to generate a regex instead of string, in this way we avoid keeping an op per condition */
    regex_ = strlit_ [ _val = phx::construct<regex>(_1) ];

    value_ = double_ | int_ | strlit_;

    condition_ = (no_case["not"] >> attr(true) | attr(false)) >> property_ >> (no_case["like"] >> regex_ | '=' >> value_ );

    filters_ %= +((
                    no_case["where"]  [_pass = (phx::size(_val)) == 0] >> attr(logicFirst)
                  | no_case["and"]    [_pass = (phx::size(_val)) >  0] >> attr(logicAnd)
                  | no_case["or"]     [_pass = (phx::size(_val)) >  0] >> attr(logicOr)
                  ) >> condition_ );
  
    print_ = no_case["print"] >> property_ % ';';
    set_ = no_case["set"] >> (property_ >> '=' >> value_) % ',';
    command_ = print_ | set_;

    expression_ = filters_ >> command_;
  }
  
  //aux
  qi::rule<Iterator, std::string() > strlit_;

  //
  qi::rule<Iterator, property() > property_;

  qi::rule<Iterator, value() , ascii::space_type> regex_;
  qi::rule<Iterator, value() , ascii::space_type> value_;

  qi::rule<Iterator, condition() , ascii::space_type> condition_;
  
  qi::rule<Iterator, set_command() , ascii::space_type> set_;
  qi::rule<Iterator, print_command() , ascii::space_type> print_;
  qi::rule<Iterator, command() , ascii::space_type> command_;

  qi::rule<Iterator, std::vector<filter>() , ascii::space_type> filters_;

  //statement
  qi::rule<Iterator, statement(), ascii::space_type> expression_;
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
    dsl_grammar<std::string::iterator> gram;

    statement s;
    if (phrase_parse(iter, end, gram, ws, s) && iter == end){
      std::cout << "Parsing succeeded - result: " << s << "\n";
    }else{
      std::string rest(iter, end);
      std::cout << "Parsing failed - stopped at: \" " << rest << "\"\n";
    }
  }

  std::cout << "Bye... :-) \n";
  return 0;
}
