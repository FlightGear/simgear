// ----------------------------------------------------------------------------
//  Description      : scanner wrapper for FlexLexer
// ----------------------------------------------------------------------------
//  (c) Copyright 1999 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_SCANNER
#define IXLIB_SCANNER




#include <ixlib_base.hh>
#include <ixlib_exbase.hh>
#include <vector>
#include <ixlib_string.hh>




class FlexLexer;




// possible errors during execution -------------------------------------------
#define ECSCAN_UNKNOWN_TOKEN      	0
#define ECSCAN_EOF		      	1




// scanner_exception ----------------------------------------------------------
namespace ixion {
  struct scanner_exception : public base_exception {
    scanner_exception(TErrorCode const error,TIndex const line,std::string const &info);
    virtual char *getText() const;
    };




// scanner --------------------------------------------------------------------
  class scanner {
    public:
      typedef unsigned 	token_type;
      
      struct token {
        token_type    		Type;
        TIndex        		Line;
        std::string       	Text;
        };
  
      typedef std::vector<token>		token_list;
      typedef std::vector<token>::iterator	token_iterator;
  
      scanner(FlexLexer &lexer);
      token_list scan();
    
    protected:
      FlexLexer	&Lexer;
      token	CurrentToken;	

      token getNextToken();
      bool reachedEOF() const;
    };
  }




#endif
