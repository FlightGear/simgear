// ----------------------------------------------------------------------------
//  Description      : Scanner for xTextFile
// ----------------------------------------------------------------------------
//  (c) Copyright 1999 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#include <FlexLexer.h>
#include "ixlib_i18n.hh"
#include <ixlib_numconv.hh>
#include <ixlib_token_lex.hh>
#include <ixlib_scanner.hh>




using namespace std;
using namespace ixion;




// Plain text rendering table -------------------------------------------------
static char *(PlainText[]) = { 
  N_("Unknown token"),
  N_("End of input") 
  };




// scanner_exception ----------------------------------------------------------
scanner_exception::scanner_exception(TErrorCode error, TIndex line, 
string const &info)
: base_exception(error, NULL, NULL, 0, "SCAN") {
  HasInfo = true;
  try {
    string temp = "line ";
    temp += unsigned2dec(line);
    if (info != "") {
      temp += " : ";
      temp += info;
      }
    strcpy(Info, temp.c_str());
    }
  catch (...) { }
  }




char *scanner_exception::getText() const {
  return PlainText[Error];
  }




// scanner --------------------------------------------------------------------
scanner::scanner(FlexLexer &lexer)
: Lexer(lexer) {
  }




scanner::token_list scanner::scan() {
  CurrentToken.Type = Lexer.yylex();
  CurrentToken.Line = Lexer.lineno();
  CurrentToken.Text = Lexer.YYText();

  token_list tokenlist;
  while (!reachedEOF()) {
    tokenlist.push_back(getNextToken());
    }
  return tokenlist;
  }




scanner::token scanner::getNextToken() {
  if (!reachedEOF()) {
    token lasttoken = CurrentToken;

    CurrentToken.Type = Lexer.yylex();
    CurrentToken.Line = Lexer.lineno();
    CurrentToken.Text = Lexer.YYText();
    
    if (CurrentToken.Type == TT_UNKNOWN)
      throw scanner_exception(ECSCAN_UNKNOWN_TOKEN,CurrentToken.Line,CurrentToken.Text);
    else return lasttoken;
    }
  throw scanner_exception(ECSCAN_UNKNOWN_TOKEN, CurrentToken.Line, "");
  }




bool scanner::reachedEOF() const {
  return (CurrentToken.Type == TT_EOF);
  }




