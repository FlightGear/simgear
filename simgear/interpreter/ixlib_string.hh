// ----------------------------------------------------------------------------
//  Description      : String crunching tools
// ----------------------------------------------------------------------------
//  (c) Copyright 1999 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_STRING
#define IXLIB_STRING




#include <string>
#include <ixlib_base.hh>
#include <ixlib_exgen.hh>




namespace ixion {
  template<class InputIterator>
  inline std::string concat(InputIterator first,InputIterator last,std::string const &sep = " ") {
    std::string str;
    while (first != last) {
      if (str.size()) str += sep;
      str += *first++;
      }
    return str;
    }
  
  
  
  
  std::string findReplace(std::string const &target,std::string const &src,std::string const &dest);
  std::string findReplace(std::string const &target,char* src,char *dest);
  std::string findReplace(std::string const &target,char src,char dest);
  std::string upper(std::string const &original);
  std::string lower(std::string const &original);
  std::string removeLeading(std::string const &original,char ch = ' ');
  std::string removeTrailing(std::string const &original,char ch = ' ');
  std::string removeLeadingTrailing(std::string const &original,char ch = ' ');
  std::string parseCEscapes(std::string const &original);
  
  TSize getMaxBase64DecodedSize(TSize encoded);
  // data must provide enough space for the maximal size determined by the
  // above function
  TSize base64decode(TByte *data,std::string const &base64);
  void base64encode(std::string &base64,TByte const *data,TSize size);




  class string_hash {
    public:
      unsigned long operator()(std::string const &str) const;
    };
  }



#endif
