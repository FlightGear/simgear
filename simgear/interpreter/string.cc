// ----------------------------------------------------------------------------
//  Description      : String object
// ----------------------------------------------------------------------------
//  (c) Copyright 1999 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#include <cstring>
#include <cctype>
#include <ixlib_numconv.hh>
#include <ixlib_string.hh>




using namespace std;
using namespace ixion;




// String utility functions ---------------------------------------------------
string ixion::findReplace(string const &target,string const &src,string const &dest) {
  string result = target;
  TIndex foundpos = string::npos;
  TIndex n = src.size();
  while ((foundpos = result.find(src)) != string::npos)
    result.replace(foundpos,n,dest);
  return result;
  }




string ixion::findReplace(string const &target,char* src,char *dest) {
  string result = target;
  TSize foundpos = string::npos;
  TSize n = strlen(src);
  while ((foundpos = result.find(src)) != string::npos)
    result.replace(foundpos,n,dest);
  return result;
  }




string ixion::findReplace(string const &target,char src,char dest) {
  string result = target;
  string::iterator first = result.begin(),last = result.end();

  while (first != last) {
    if (*first == src) *first = dest;
    first++;
    }
  return result;
  }




string ixion::upper(string const &original) {
  string temp(original);
  string::iterator first = temp.begin(),last = temp.end();
  
  while (first != last) {
    *first = toupper(*first);
    first++;
    }
  return temp;
  }




string ixion::lower(string const &original) {
  string temp(original);
  string::iterator first = temp.begin(),last = temp.end();
  
  while (first != last) {
    *first = tolower(*first);
    first++;
    }
  return temp;
  }




string ixion::removeLeading(string const &original,char ch) {
  string copy(original);
  string::iterator first = copy.begin(), last = copy.end();
  
  while (first != last && *first == ch) first++;
  if (first != copy.begin()) copy.erase(copy.begin(),first);
  return copy;
  }




string ixion::removeTrailing(string const &original,char ch) {
  string copy(original);
  string::iterator first = copy.begin(), last = copy.end();
  
  if (first != last) {
    last--;
    while (first != last && *last == ch) last--;
    if (*last != ch) last++;
    }
  
  if (last != copy.end()) copy.erase(last,copy.end());
  return copy;
  }




string ixion::removeLeadingTrailing(string const &original,char ch) {
  string copy(original);
  string::iterator first = copy.begin(), last = copy.end();
  
  while (first != last && *first == ch) first++;
  if (first != copy.begin()) copy.erase(copy.begin(),first);
  
  first = copy.begin();
  last = copy.end();

  if (first != last) {
    last--;
    while (first != last && *last == ch) last--;
    if (*last != ch) last++;
    }

  if (last != copy.end()) copy.erase(last,copy.end());
  return copy;
  }




string ixion::parseCEscapes(string const &original) {
  string result = "";
  string::const_iterator first = original.begin(),last = original.end();
  while (first != last) {
    if (*first == '\\') {
      first++;
      if (first == last) { 
        result += '\\';
	break;
	}
      
      #define GET_TEMP_STRING(LENGTH) \
        if (original.end()-first < LENGTH) \
	  EXGEN_THROWINFO(EC_INVALIDPAR,"invalid escape sequence") \
	tempstring = string(first,first+LENGTH); \
	first += LENGTH;
      
      char value;
      string tempstring;
      switch (*first) {
        case 'b': result += '\b'; first++; break;
        case 'f': result += '\f'; first++; break;
        case 'n': result += '\n'; first++; break;
        case 't': result += '\t'; first++; break;
        case 'v': result += '\v'; first++; break;
	case 'X':
	case 'x': first++;
	  GET_TEMP_STRING(2)
	  value = evalNumeral(tempstring,16);
	  result += value;
	  break;
	case 'u': first++;
	  GET_TEMP_STRING(4)
	  value = evalNumeral(tempstring,16);
	  result += value;
	  break;
	case '0':
	  GET_TEMP_STRING(3)
	  value = evalNumeral(tempstring,8);
	  result += value;
	  break;
	default: result += *first++;
	}
      }
    else result += *first++;
    }
  return result;
  }




namespace {
  TByte const B64_INVALID = 0xff;
  TByte const B64_PAD = 0xfe;
  char const B64_PAD_CHAR = '=';
  char Base64EncodeTable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  TByte Base64DecodeTable[] = { // based at 0
    // see test/invertmap.js on how to generate this table
    B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,B64_INVALID,B64_INVALID,62,B64_INVALID,B64_INVALID,B64_INVALID,63,52,53,54,
    55,56,57,58,59,60,61,B64_INVALID,B64_INVALID,B64_INVALID,B64_PAD,B64_INVALID,
    B64_INVALID,B64_INVALID,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,
    19,20,21,22,23,24,25,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,
    44,45,46,47,48,49,50,51,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    };
    
  }




TSize ixion::getMaxBase64DecodedSize(TSize encoded) {
  return ((encoded+3)/4)*3;
  }




TSize ixion::base64decode(TByte *data,string const &base64) {
  string::const_iterator first = base64.begin(),last = base64.end();
  
  TByte *data_start = data;
  
  TUnsigned32 block;
  TByte a,b,c,d;
  
  while (first != last) {
    a = Base64DecodeTable[*(first++)];
    b = Base64DecodeTable[*(first++)];
    c = Base64DecodeTable[*(first++)];
    d = Base64DecodeTable[*(first++)];
    if (c == B64_PAD) {
      block = a << 3*6 | b << 2*6;
      *data++ = (block >> 16) & 0xff;
      }
    else if (d == B64_PAD) {
      block = a << 3*6 | b << 2*6 | c << 1*6;
      *data++ = (block >> 16) & 0xff;
      *data++ = (block >> 8) & 0xff;
      }
    else {
      block = a << 3*6 | b << 2*6 | c << 1*6 | d << 0*6;
      *data++ = (block >> 16) & 0xff;
      *data++ = (block >> 8) & 0xff;
      *data++ = (block >> 0) & 0xff;
      }
    }
  return data-data_start;
  }




void ixion::base64encode(string &base64,TByte const *data,TSize size) {
  base64.resize((size+2)/3*4);

  TUnsigned32 block;
  TByte a,b,c,d;

  TByte const *end = data+size;
  string::iterator first = base64.begin();
  while (data < end)
    if (data+1 == end) {
      block = data[0] << 16;
      a = (block >> 3*6) & 0x3f;
      b = (block >> 2*6) & 0x3f;
      *first++ = Base64EncodeTable[a];
      *first++ = Base64EncodeTable[b];
      *first++ = B64_PAD_CHAR;
      *first++ = B64_PAD_CHAR;
      data++;
      }
    else if (data+2 == end) {
      block = data[0] << 16 | data[1] << 8;
      a = (block >> 3*6) & 0x3f;
      b = (block >> 2*6) & 0x3f;
      c = (block >> 1*6) & 0x3f;
      *first++ = Base64EncodeTable[a];
      *first++ = Base64EncodeTable[b];
      *first++ = Base64EncodeTable[c];
      *first++ = B64_PAD_CHAR;
      data += 2;
      }
    else {
      block = data[0] << 16 | data[1] << 8 | data[2];
      a = (block >> 3*6) & 0x3f;
      b = (block >> 2*6) & 0x3f;
      c = (block >> 1*6) & 0x3f;
      d = (block >> 0*6) & 0x3f;
      *first++ = Base64EncodeTable[a];
      *first++ = Base64EncodeTable[b];
      *first++ = Base64EncodeTable[c];
      *first++ = Base64EncodeTable[d];
      data += 3;
      }
  }




// string_hash ----------------------------------------------------------------
unsigned long ixion::string_hash::operator()(string const &str) const {
  // the sgi stl uses the same hash algorithm
  unsigned long h = 0; 
  FOREACH_CONST(first,str,string)
    h = 5*h + *first;
  return h;
  }
