/* -------- definitions ------- */

%option c++ yylineno noyywrap prefix="js" outfile="lex.javascript.cc" batch

%{
#include <ixlib_js_internals.hh>
#include <ixlib_token_javascript.hh>

using namespace ixion;
using namespace javascript;
%}

WHITESPACE	[ \t\n\r]

DIGIT		[0-9]
DIGIT_NZ	[1-9]
DIGIT_OCT	[0-7]
DIGIT_HEX	[0-9a-fA-F]
DIGIT_SEQ	{DIGIT}+

NONDIGIT	[_a-zA-Z]
ID_COMPONENT	[_a-zA-Z0-9]

ESCAPE_SIMPLE	\\['"?\\abfnrtv]
ESCAPE_OCTAL	\\{DIGIT_OCT}{1,3}
ESCAPE_HEX	\\x{DIGIT_HEX}{1,2}
ESCAPE		{ESCAPE_SIMPLE}|{ESCAPE_OCTAL}|{ESCAPE_HEX}
S_CHAR		[^"\\\n]|{ESCAPE}

SIGN		\+|\-
SIGNopt		{SIGN}?




/* higher-level entities ------------------------------------------------------
*/
IDENTIFIER	{NONDIGIT}{ID_COMPONENT}*




/* literals -------------------------------------------------------------------
*/
LIT_DECIMAL	{DIGIT_NZ}{DIGIT}*
LIT_OCTAL	0{DIGIT_OCT}*
LIT_HEX		0[xX]{DIGIT_HEX}+
LIT_INT		({LIT_DECIMAL}|{LIT_OCTAL}|{LIT_HEX})

LIT_STRING	\"{S_CHAR}*\"|\'{S_CHAR}*\'

LIT_FRACTION	{DIGIT_SEQ}?\.{DIGIT_SEQ}|{DIGIT_SEQ}\.
LIT_EXPONENT	[eE]{SIGNopt}{DIGIT_SEQ}
LIT_FLOAT	{LIT_FRACTION}{LIT_EXPONENT}?|{DIGIT_SEQ}{LIT_EXPONENT}






/* Contexts -------------------------------------------------------------------
*/

%x Comment
%x LineComment




/* Rules ----------------------------------------------------------------------
*/
%% 

\/\*			BEGIN(Comment);
<Comment>\*\/		BEGIN(INITIAL);
<Comment><<EOF>>	EXJS_THROW(ECJS_UNTERMINATED_COMMENT)
<Comment>.		/* nothing */
<Comment>\n		/* nothing */
\/\/			BEGIN(LineComment);
<LineComment>[\n\r]+	BEGIN(INITIAL);
<LineComment>.		/* nothing */

<<EOF>>    		return TT_EOF;

\{			return '{';
\}			return '}';
\;			return ';';

\[			return '[';
\]			return ']';
\(			return '(';
\)			return ')';
\?			return '?';
\:			return ':';
\+			return '+';
\-			return '-';
\*			return '*';
\/			return '/';
\%			return '%';
\^			return '^';
\&			return '&';
\|			return '|';
\~			return '~';
\!			return '!';
\=			return '=';
\<			return '<';
\>			return '>';
\,			return ',';
\.			return '.';
\+\=			return TT_JS_PLUS_ASSIGN;
\-\=			return TT_JS_MINUS_ASSIGN;
\*\=			return TT_JS_MULTIPLY_ASSIGN;
\/\=			return TT_JS_DIVIDE_ASSIGN;
\%\=			return TT_JS_MODULO_ASSIGN;
\^\=			return TT_JS_BIT_XOR_ASSIGN;
\&\=			return TT_JS_BIT_AND_ASSIGN;
\|\=			return TT_JS_BIT_OR_ASSIGN;
\<\<			return TT_JS_LEFT_SHIFT;
\>\>			return TT_JS_RIGHT_SHIFT;
\<\<\=			return TT_JS_LEFT_SHIFT_ASSIGN;
\>\>\=			return TT_JS_RIGHT_SHIFT_ASSIGN;
\=\=\=			return TT_JS_IDENTICAL;
\!\=\=			return TT_JS_NOT_IDENTICAL;
\=\=			return TT_JS_EQUAL;
\!\=			return TT_JS_NOT_EQUAL;
\<\=			return TT_JS_LESS_EQUAL;
\>\=			return TT_JS_GREATER_EQUAL;
\&\&			return TT_JS_LOGICAL_AND;
\|\|			return TT_JS_LOGICAL_OR;
\+\+			return TT_JS_INCREMENT;
\-\-			return TT_JS_DECREMENT;

new			return TT_JS_NEW;

this			return TT_JS_THIS;
function		return TT_JS_FUNCTION;
var			return TT_JS_VAR;
null			return TT_JS_NULL;
if			return TT_JS_IF;
while			return TT_JS_WHILE;
do			return TT_JS_DO;
else			return TT_JS_ELSE;
for			return TT_JS_FOR;
return			return TT_JS_RETURN;
switch			return TT_JS_SWITCH;
case			return TT_JS_CASE;
continue		return TT_JS_CONTINUE;
break			return TT_JS_BREAK;
default			return TT_JS_DEFAULT;
true			return TT_JS_LIT_TRUE;
false			return TT_JS_LIT_FALSE;
undefined		return TT_JS_LIT_UNDEFINED;
in			return TT_JS_IN;
const			return TT_JS_CONST;
class			return TT_JS_CLASS;
extends			return TT_JS_EXTENDS;
namespace		return TT_JS_NAMESPACE;
static			return TT_JS_STATIC;
constructor		return TT_JS_CONSTRUCTOR;

{LIT_INT}		return TT_JS_LIT_INT;
{LIT_FLOAT}		return TT_JS_LIT_FLOAT;
{LIT_STRING}		return TT_JS_LIT_STRING;

{IDENTIFIER}		return TT_JS_IDENTIFIER;

{WHITESPACE}+		/* nothing */
.			EXJS_THROWINFOLOCATION(ECJS_INVALID_TOKEN,YYText(),code_location(lineno()))
