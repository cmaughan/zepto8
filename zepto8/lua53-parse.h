// Copyright (c) 2015-2016 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/ColinH/PEGTL/

#include <pegtl.hh>
#include <pegtl/analyze.hh>
#include <pegtl/contrib/raw_string.hh>

namespace lua53
{
   // PEGTL grammar for the Lua 5.3.0 lexer and parser.
   //
   // The grammar here is not very similar to the grammar
   // in the Lua reference documentation on which it is based
   // which is due to multiple causes.
   //
   // The main difference is that this grammar includes really
   // "everything", not just the structural parts from the
   // reference documentation:
   // - The PEG-approach combines lexer and parser; this grammar
   //   handles comments and tokenisation.
   // - The operator precedence and associativity are reflected
   //   in the structure of this grammar.
   // - All details for all types of literals are included, with
   //   escape-sequences for literal strings, and long literals.
   //
   // The second necessary difference is that all left-recursion
   // had to be eliminated.
   //
   // In some places the grammar was optimised to require as little
   // back-tracking as possible, most prominently for expressions.
   // The original grammar contains the following production rules:
   //
   //   prefixexp ::= var | functioncall | ‘(’ exp ‘)’
   //   functioncall ::=  prefixexp args | prefixexp ‘:’ Name args
   //   var ::=  Name | prefixexp ‘[’ exp ‘]’ | prefixexp ‘.’ Name
   //
   // We need to eliminate the left-recursion, and we also want to
   // remove the ambiguity between function calls and variables,
   // i.e. the fact that we can have expressions like
   //
   //   ( a * b ).c()[ d ].e:f()
   //
   // where only the last element decides between function call and
   // variable, making it necessary to parse the whole thing again
   // if we chose wrong at the beginning.
   // First we eliminate prefixexp and obtain:
   //
   //   functioncall ::=  ( var | functioncall | ‘(’ exp ‘)’ ) ( args | ‘:’ Name args )
   //   var ::=  Name | ( var | functioncall | ‘(’ exp ‘)’ ) ( ‘[’ exp ‘]’ | ‘.’ Name )
   //
   // Next we split function_call and variable into a first part,
   // a "head", or how they can start, and a second part, the "tail",
   // which, in a sequence like above, is the final deciding part:
   //
   //   vartail ::= '[' exp ']' | '.' Name
   //   varhead ::= Name | '(' exp ')' vartail
   //   functail ::= args | ':' Name args
   //   funchead ::= Name | '(' exp ')'
   //
   // This allows us to rewrite var and function_call as follows.
   //
   //   var ::= varhead { { functail } vartail }
   //   function_call ::= funchead [ { vartail } functail ]
   //
   // Finally we can define a single expression that takes care
   // of var, function_call, and expressions in a bracket:
   //
   //   chead ::= '(' exp ')' | Name
   //   combined ::= chead { functail | vartail }
   //
   // Such a combined expression starts with a bracketed
   // expression or a name, and continues with an arbitrary
   // number of functail and/or vartail parts, all in a one
   // grammar rule without back-tracking.
   //
   // The rule expr_thirteen below implements "combined".
   //
   // Another issue of interest when writing a PEG is how to
   // manage the separators, the white-space and comments that
   // can occur in many places; in the classical two-stage
   // lexer-parser approach the lexer would have taken care of
   // this, but here we use the PEG approach that combines both.
   //
   // In the following grammar most rules adopt the convention
   // that they take care of "internal padding", i.e. spaces
   // and comments that can occur within the rule, but not
   // "external padding", i.e. they don't start or end with
   // a rule that "eats up" all extra padding (spaces and
   // comments). In some places, where it is more efficient,
   // right padding is used.

   struct short_comment : pegtl::until< pegtl::eolf > {};
   struct long_string : pegtl::raw_string< '[', '=', ']' > {};
   struct comment : pegtl::disable< pegtl::two< '-' >, pegtl::sor< long_string, short_comment > > {};

   struct sep : pegtl::sor< pegtl::ascii::space, comment > {};
   struct seps : pegtl::star< sep > {};

   struct str_and : pegtl_string_t( "and" ) {};
   struct str_break : pegtl_string_t( "break" ) {};
   struct str_do : pegtl_string_t( "do" ) {};
   struct str_else : pegtl_string_t( "else" ) {};
   struct str_elseif : pegtl_string_t( "elseif" ) {};
   struct str_end : pegtl_string_t( "end" ) {};
   struct str_false : pegtl_string_t( "false" ) {};
   struct str_for : pegtl_string_t( "for" ) {};
   struct str_function : pegtl_string_t( "function" ) {};
   struct str_goto : pegtl_string_t( "goto" ) {};
   struct str_if : pegtl_string_t( "if" ) {};
   struct str_in : pegtl_string_t( "in" ) {};
   struct str_local : pegtl_string_t( "local" ) {};
   struct str_nil : pegtl_string_t( "nil" ) {};
   struct str_not : pegtl_string_t( "not" ) {};
   struct str_or : pegtl_string_t( "or" ) {};
   struct str_repeat : pegtl_string_t( "repeat" ) {};
   struct str_return : pegtl_string_t( "return" ) {};
   struct str_then : pegtl_string_t( "then" ) {};
   struct str_true : pegtl_string_t( "true" ) {};
   struct str_until : pegtl_string_t( "until" ) {};
   struct str_while : pegtl_string_t( "while" ) {};

   // Note that 'elseif' precedes 'else' in order to prevent only matching
   // the "else" part of an "elseif" and running into an error in the
   // 'keyword' rule.

   struct str_keyword : pegtl::sor< str_and, str_break, str_do, str_elseif, str_else, str_end, str_false, str_for, str_function, str_goto, str_if, str_in, str_local, str_nil, str_not, str_repeat, str_return, str_then, str_true, str_until, str_while > {};

   template< typename Key >
   struct key : pegtl::seq< Key, pegtl::not_at< pegtl::identifier_other > > {};

   struct key_and : key< str_and > {};
   struct key_break : key< str_break > {};
   struct key_do : key< str_do > {};
   struct key_else : key< str_else > {};
   struct key_elseif : key< str_elseif > {};
   struct key_end : key< str_end > {};
   struct key_false : key< str_false > {};
   struct key_for : key< str_for > {};
   struct key_function : key< str_function > {};
   struct key_goto : key< str_goto > {};
   struct key_if : key< str_if > {};
   struct key_in : key< str_in > {};
   struct key_local : key< str_local > {};
   struct key_nil : key< str_nil > {};
   struct key_not : key< str_not > {};
   struct key_or : key< str_or > {};
   struct key_repeat : key< str_repeat > {};
   struct key_return : key< str_return > {};
   struct key_then : key< str_then > {};
   struct key_true : key< str_true > {};
   struct key_until : key< str_until > {};
   struct key_while : key< str_while > {};

   struct keyword : key< str_keyword > {};

   template< typename R >
   struct pad : pegtl::pad< R, sep > {};

   struct three_dots : pegtl::string< '.', '.', '.' > {};

   struct name : pegtl::seq< pegtl::not_at< keyword >, pegtl::identifier > {};

   struct single : pegtl::one< 'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '0', '\n' > {};
   struct spaces : pegtl::seq< pegtl::one< 'z' >, pegtl::star< pegtl::space > > {};
   struct hexbyte : pegtl::if_must< pegtl::one< 'x' >, pegtl::xdigit, pegtl::xdigit > {};
   struct decbyte : pegtl::if_must< pegtl::digit, pegtl::rep_opt< 2, pegtl::digit > > {};
   struct unichar : pegtl::if_must< pegtl::one< 'u' >, pegtl::one< '{' >, pegtl::plus< pegtl::xdigit >, pegtl::one< '}' > > {};
   struct escaped : pegtl::if_must< pegtl::one< '\\' >, pegtl::sor< hexbyte, decbyte, unichar, single, spaces > > {};
   struct regular : pegtl::not_one< '\r', '\n' > {};
   struct character : pegtl::sor< escaped, regular > {};

   template< char Q >
   struct short_string : pegtl::if_must< pegtl::one< Q >, pegtl::until< pegtl::one< Q >, character > > {};
   struct literal_string : pegtl::sor< short_string< '"' >, short_string< '\'' >, long_string > {};

   template< typename E >
   struct exponent : pegtl::opt< pegtl::if_must< E, pegtl::opt< pegtl::one< '+', '-' > >, pegtl::plus< pegtl::digit > > > {};

   template< typename D, typename E >
   struct numeral_three : pegtl::seq< pegtl::if_must< pegtl::one< '.' >, pegtl::plus< D > >, exponent< E > > {};
   template< typename D, typename E >
   struct numeral_two : pegtl::seq< pegtl::plus< D >, pegtl::opt< pegtl::one< '.' >, pegtl::star< D > >, exponent< E > > {};
   template< typename D, typename E >
   struct numeral_one : pegtl::sor< numeral_two< D, E >, numeral_three< D, E > > {};

   struct decimal : numeral_one< pegtl::digit, pegtl::one< 'e', 'E' > > {};
   struct hexadecimal : pegtl::if_must< pegtl::istring< '0', 'x' >, numeral_one< pegtl::xdigit, pegtl::one< 'p', 'P' > > > {};
   struct numeral : pegtl::sor< hexadecimal, decimal > {};

   struct label_statement : pegtl::if_must< pegtl::two< ':' >, seps, name, seps, pegtl::two< ':' > > {};
   struct goto_statement : pegtl::if_must< key_goto, seps, name > {};

   struct statement;
   struct expression;

   struct name_list : pegtl::list< name, pegtl::one< ',' >, sep > {};
   struct name_list_must : pegtl::list_must< name, pegtl::one< ',' >, sep > {};
   struct expr_list_must : pegtl::list_must< expression, pegtl::one< ',' >, sep > {};

   struct statement_return : pegtl::seq< pegtl::pad_opt< expr_list_must, sep >, pegtl::opt< pegtl::one< ';' >, seps > > {};

   template< typename E >
   struct statement_list : pegtl::seq< seps, pegtl::until< pegtl::sor< E, pegtl::if_must< key_return, statement_return, E > >, statement, seps > > {};

   struct table_field_one : pegtl::if_must< pegtl::one< '[' >, seps, expression, seps, pegtl::one< ']' >, seps, pegtl::one< '=' >, seps, expression > {};
   struct table_field_two : pegtl::if_must< pegtl::seq< name, seps, pegtl::one< '=' > >, seps, expression > {};
   struct table_field : pegtl::sor< table_field_one, table_field_two, expression > {};
   struct table_field_list : pegtl::list_tail< table_field, pegtl::one< ',', ';' >, sep > {};
   struct table_constructor : pegtl::if_must< pegtl::one< '{' >, pegtl::pad_opt< table_field_list, sep >, pegtl::one< '}' > > {};

   struct parameter_list_one : pegtl::seq< name_list, pegtl::opt< pegtl::if_must< pad< pegtl::one< ',' > >, three_dots > > > {};
   struct parameter_list : pegtl::sor< three_dots, parameter_list_one > {};

   struct function_body : pegtl::seq< pegtl::one< '(' >, pegtl::pad_opt< parameter_list, sep >, pegtl::one< ')' >, seps, statement_list< key_end > > {};
   struct function_literal : pegtl::if_must< key_function, seps, function_body > {};

   struct bracket_expr : pegtl::if_must< pegtl::one< '(' >, seps, expression, seps, pegtl::one< ')' > > {};

   struct function_args_one : pegtl::if_must< pegtl::one< '(' >, pegtl::pad_opt< expr_list_must, sep >, pegtl::one< ')' > > {};
   struct function_args : pegtl::sor< function_args_one, table_constructor, literal_string > {};

   struct variable_tail_one : pegtl::if_must< pegtl::one< '[' >, seps, expression, seps, pegtl::one< ']' > > {};
   struct variable_tail_two : pegtl::if_must< pegtl::seq< pegtl::not_at< pegtl::two< '.' > >, pegtl::one< '.' > >, seps, name > {};
   struct variable_tail : pegtl::sor< variable_tail_one, variable_tail_two > {};

   struct function_call_tail_one : pegtl::if_must< pegtl::seq< pegtl::not_at< pegtl::two< ':' > >, pegtl::one< ':' > >, seps, name, seps, function_args > {};
   struct function_call_tail : pegtl::sor< function_args, function_call_tail_one > {};

   struct variable_head_one : pegtl::seq< bracket_expr, seps, variable_tail > {};
   struct variable_head : pegtl::sor< name, variable_head_one > {};

   struct function_call_head : pegtl::sor< name, bracket_expr > {};

   struct variable : pegtl::seq< variable_head, pegtl::star< pegtl::star< seps, function_call_tail >, seps, variable_tail > > {};
   struct function_call : pegtl::seq< function_call_head, pegtl::plus< pegtl::until< pegtl::seq< seps, function_call_tail >, seps, variable_tail > > > {};

   template< char O, char ... N >
   struct op_one : pegtl::seq< pegtl::one< O >, pegtl::at< pegtl::not_one< N ... > > > {};
   template< char O, char P, char ... N >
   struct op_two : pegtl::seq< pegtl::string< O, P >, pegtl::at< pegtl::not_one< N ... > > > {};

   template< typename S, typename O >
   struct left_assoc : pegtl::seq< S, seps, pegtl::star< pegtl::if_must< O, seps, S, seps > > > {};
   template< typename S, typename O >
   struct right_assoc : pegtl::seq< S, seps, pegtl::opt< pegtl::if_must< O, seps, right_assoc< S, O > > > > {};

   struct unary_operators : pegtl::sor< pegtl::one< '-' >,
                                        pegtl::one< '#' >,
                                        op_one< '~', '=' >,
                                        key_not > {};

   struct expr_ten;
   struct expr_thirteen : pegtl::seq< pegtl::sor< bracket_expr, name >, pegtl::star< seps, pegtl::sor< function_call_tail, variable_tail > > > {};
   struct expr_twelve : pegtl::sor< key_nil,
                                    key_true,
                                    key_false,
                                    three_dots,
                                    numeral,
                                    literal_string,
                                    function_literal,
                                    expr_thirteen,
                                    table_constructor > {};
   struct expr_eleven : pegtl::seq< expr_twelve, seps, pegtl::opt< pegtl::one< '^' >, seps, expr_ten, seps > > {};
   struct unary_apply : pegtl::if_must< unary_operators, seps, expr_ten, seps > {};
   struct expr_ten : pegtl::sor< unary_apply, expr_eleven > {};
   struct operators_nine : pegtl::sor< pegtl::two< '/' >,
                                       pegtl::one< '/' >,
                                       pegtl::one< '*' >,
                                       pegtl::one< '%' > > {};
   struct expr_nine : left_assoc< expr_ten, operators_nine > {};
   struct operators_eight : pegtl::sor< pegtl::one< '+' >,
                                        pegtl::one< '-' > > {};
   struct expr_eight : left_assoc< expr_nine, operators_eight > {};
   struct expr_seven : right_assoc< expr_eight, op_two< '.', '.', '.' > > {};
   struct operators_six : pegtl::sor< pegtl::two< '<' >,
                                      pegtl::two< '>' > > {};
   struct expr_six : left_assoc< expr_seven, operators_six > {};
   struct expr_five : left_assoc< expr_six, pegtl::one< '&' > > {};
   struct expr_four : left_assoc< expr_five, op_one< '~', '=' > > {};
   struct expr_three : left_assoc< expr_four, pegtl::one< '|' > > {};
#if defined WITH_PICO8
   // From the PICO-8 documentation:
   //
   //  3. != operator
   //
   // Not shorthand, but pico-8 also accepts != instead of ~= for "not equal to"
   struct operator_notequal : pegtl::string< '!', '=' > {};
#endif
   struct operators_two : pegtl::sor< pegtl::two< '=' >,
                                      pegtl::string< '<', '=' >,
                                      pegtl::string< '>', '=' >,
                                      op_one< '<', '<' >,
                                      op_one< '>', '>' >,
#if defined WITH_PICO8
                                      operator_notequal,
#endif
                                      pegtl::string< '~', '=' > > {};
   struct expr_two : left_assoc< expr_three, operators_two > {};
   struct expr_one : left_assoc< expr_two, key_and > {};
   struct expression : left_assoc< expr_one, key_or > {};

   struct do_statement : pegtl::if_must< key_do, statement_list< key_end > > {};
   struct while_statement : pegtl::if_must< key_while, seps, expression, seps, key_do, statement_list< key_end > > {};
   struct repeat_statement : pegtl::if_must< key_repeat, statement_list< key_until >, seps, expression > {};

   struct at_elseif_else_end : pegtl::sor< pegtl::at< key_elseif >, pegtl::at< key_else >, pegtl::at< key_end > > {};
   struct elseif_statement : pegtl::if_must< key_elseif, seps, expression, seps, key_then, statement_list< at_elseif_else_end > > {};
   struct else_statement : pegtl::if_must< key_else, statement_list< key_end > > {};
   struct if_statement : pegtl::if_must< key_if, seps, expression, seps, key_then, statement_list< at_elseif_else_end >, seps, pegtl::until< pegtl::sor< else_statement, key_end >, elseif_statement, seps > > {};
#if defined WITH_PICO8
   // From the PICO-8 documentation:
   //
   // 1. IF THEN END statements on a single line can be expressed without the THEN & END
   //
   // IF (NOT B) I=1 J=2
   //
   // -- is equivalent to: IF (NOT B) THEN I=1 J=2 END
   // -- note that the condition must be surrounded by brackets.
   //
   // From Sam’s observations:
   //
   //  FOR I=0,9 DO IF(I>5) PRINT(I) END    -- this is valid
   //  FOR I=0,9 DO IF(I>5) DO FOR J=0,9 IF(J>5) PRINT(J) END END  -- so is this
   // FIXME: doesn’t work
   struct short_if_statement : pegtl::seq< key_if, seps, bracket_expr, seps, pegtl::not_at< key_then >, pegtl::until< pegtl::at< pegtl::sor< pegtl::eolf, key_end > > > > {};
   //struct short_if_statement : pegtl::seq< key_if, seps, bracket_expr, seps, statement_list< pegtl::eolf >, pegtl::eolf > {};
   //struct short_if_statement : pegtl::seq< key_if, seps, bracket_expr, seps, statement_list< pegtl::sor< pegtl::at< key_end >, pegtl::eolf > > > {};
#endif

   struct for_statement_one : pegtl::seq< name, seps, pegtl::one< '=' >, seps, expression, seps, pegtl::one< ',' >, seps, expression, pegtl::pad_opt< pegtl::if_must< pegtl::one< ',' >, seps, expression >, sep >, key_do, statement_list< key_end > > {};
   struct for_statement_two : pegtl::seq< name_list_must, seps, key_in, seps, expr_list_must, seps, key_do, statement_list< key_end > > {};
   struct for_statement : pegtl::if_must< key_for, seps, pegtl::sor< for_statement_one, for_statement_two > > {};

#if defined WITH_PICO8
   // From the PICO-8 documentation:
   //
   //  2. unary math operators
   //
   // a += 2  -- equivalent to: a = a + 2
   // a -= 2  -- equivalent to: a = a - 2
   // a *= 2  -- equivalent to: a = a * 2
   // a /= 2  -- equivalent to: a = a / 2
   // a %= 2  -- equivalent to: a = a % 2
   struct operators_reassign : pegtl::sor< pegtl::string< '+', '=' >,
                                           pegtl::string< '-', '=' >,
                                           pegtl::string< '*', '=' >,
                                           pegtl::string< '/', '=' >,
                                           pegtl::string< '%', '=' > > {};
   struct reassignment : pegtl::seq< variable, seps, operators_reassign, seps, expr_list_must > {};
#endif
   struct assignment_variable_list : pegtl::list_must< variable, pegtl::one< ',' >, sep > {};
   struct assignments_one : pegtl::if_must< pegtl::one< '=' >, seps, expr_list_must > {};
   struct assignments : pegtl::seq< assignment_variable_list, seps, assignments_one > {};
   struct function_name : pegtl::seq< pegtl::list< name, pegtl::one< '.' >, sep >, seps, pegtl::opt< pegtl::if_must< pegtl::one< ':' >, seps, name, seps > > > {};
   struct function_definition : pegtl::if_must< key_function, seps, function_name, function_body > {};

   struct local_function : pegtl::if_must< key_function, seps, name, seps, function_body > {};
   struct local_variables : pegtl::if_must< name_list_must, seps, pegtl::opt< assignments_one > > {};
   struct local_statement : pegtl::if_must< key_local, seps, pegtl::sor< local_function, local_variables > > {};

   struct semicolon : pegtl::one< ';' > {};
   struct statement : pegtl::sor< semicolon,
                                  assignments,
#if defined WITH_PICO8
                                  reassignment,
#endif
                                  function_call,
                                  label_statement,
                                  key_break,
                                  goto_statement,
                                  do_statement,
                                  while_statement,
                                  repeat_statement,
#if defined WITH_PICO8
                                  // FIXME: doesn’t work properly yet
                                  //short_if_statement,
#endif
                                  if_statement,
                                  for_statement,
                                  function_definition,
                                  local_statement > {};

   struct interpreter : pegtl::seq< pegtl::one< '#' >, pegtl::until< pegtl::eolf > > {};
   struct grammar : pegtl::must< pegtl::opt< interpreter >, statement_list< pegtl::eof > > {};

} // lua53

#if !defined WITH_PICO8
int main( int argc, char ** argv )
{
   pegtl::analyze< lua53::grammar >();

   for ( int i = 1; i < argc; ++i ) {
      pegtl::file_parser( argv[ i ] ).parse< lua53::grammar >();
   }
   return 0;
}
#endif
