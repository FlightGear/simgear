// ----------------------------------------------------------------------------
//  Description      : Token definitions for Javascript scanner
// ----------------------------------------------------------------------------
//  (c) Copyright 2000 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_TOKEN_JAVASCRIPT
#define IXLIB_TOKEN_JAVASCRIPT




#include <ixlib_token_lex.hh>




// keywords
#define TT_JS_THIS			(TT_USER + 0)
#define TT_JS_FUNCTION			(TT_USER + 1)
#define TT_JS_VAR			(TT_USER + 2)
#define TT_JS_NULL			(TT_USER + 3)
#define TT_JS_IF			(TT_USER + 4)
#define TT_JS_WHILE			(TT_USER + 5)
#define TT_JS_DO			(TT_USER + 6)
#define TT_JS_ELSE			(TT_USER + 7)
#define TT_JS_FOR			(TT_USER + 8)
#define TT_JS_RETURN			(TT_USER + 9)
#define TT_JS_SWITCH			(TT_USER + 10)
#define TT_JS_CASE			(TT_USER + 11)
#define TT_JS_CONTINUE			(TT_USER + 12)
#define TT_JS_BREAK			(TT_USER + 13)
#define TT_JS_DEFAULT			(TT_USER + 14)
#define TT_JS_IN			(TT_USER + 15)
#define TT_JS_CONST			(TT_USER + 16)
#define TT_JS_CLASS			(TT_USER + 17)
#define TT_JS_EXTENDS			(TT_USER + 18)
#define TT_JS_NAMESPACE			(TT_USER + 19)
#define TT_JS_STATIC			(TT_USER + 20)
#define TT_JS_CONSTRUCTOR		(TT_USER + 21)

// operators
#define TT_JS_NEW			(TT_USER + 1024)

#define TT_JS_PLUS_ASSIGN		(TT_USER + 1025)
#define TT_JS_MINUS_ASSIGN		(TT_USER + 1026)
#define TT_JS_MULTIPLY_ASSIGN		(TT_USER + 1027)
#define TT_JS_DIVIDE_ASSIGN		(TT_USER + 1028)
#define TT_JS_MODULO_ASSIGN		(TT_USER + 1029)
#define TT_JS_BIT_XOR_ASSIGN		(TT_USER + 1030)
#define TT_JS_BIT_AND_ASSIGN		(TT_USER + 1031)
#define TT_JS_BIT_OR_ASSIGN		(TT_USER + 1032)
#define TT_JS_LEFT_SHIFT		(TT_USER + 1033)
#define TT_JS_RIGHT_SHIFT		(TT_USER + 1034)
#define TT_JS_LEFT_SHIFT_ASSIGN		(TT_USER + 1035)
#define TT_JS_RIGHT_SHIFT_ASSIGN	(TT_USER + 1036)
#define TT_JS_EQUAL			(TT_USER + 1037)
#define TT_JS_NOT_EQUAL			(TT_USER + 1038)
#define TT_JS_LESS_EQUAL		(TT_USER + 1039)
#define TT_JS_GREATER_EQUAL		(TT_USER + 1040)
#define TT_JS_LOGICAL_AND		(TT_USER + 1041)
#define TT_JS_LOGICAL_OR		(TT_USER + 1042)
#define TT_JS_INCREMENT			(TT_USER + 1043)
#define TT_JS_DECREMENT			(TT_USER + 1044)
#define TT_JS_IDENTICAL			(TT_USER + 1045)
#define TT_JS_NOT_IDENTICAL		(TT_USER + 1046)

// literals
#define TT_JS_LIT_INT			(TT_USER + 2048)
#define TT_JS_LIT_FLOAT			(TT_USER + 2049)
#define TT_JS_LIT_STRING		(TT_USER + 2050)
#define TT_JS_LIT_TRUE			(TT_USER + 2051)
#define TT_JS_LIT_FALSE			(TT_USER + 2052)
#define TT_JS_LIT_UNDEFINED		(TT_USER + 2053)

// identifier
#define TT_JS_IDENTIFIER		(TT_USER + 3072)




#endif
