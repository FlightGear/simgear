// ----------------------------------------------------------------------------
//  Description      : Regular expressions string object.
// ----------------------------------------------------------------------------
//  (c) Copyright 1998 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#include <stack>
#include <cctype>
#include "ixlib_i18n.hh"
#include <ixlib_exgen.hh>
#include <ixlib_numeric.hh>
#include <ixlib_numconv.hh>
#include <ixlib_re.hh>
#include <ixlib_re_impl.hh>




using namespace std;
using namespace ixion;




// Template instantiations ----------------------------------------------------
template regex<string>;




// Error texts ----------------------------------------------------------------
static char *RegexPlainText[] = {
  N_("Invalid quantifier"),
  N_("Unbalanced backreference"),
  N_("Invalid escape sequence"),
  N_("Invalid backreference"),
  N_("Unterminated character class"),
  N_("Unable to match without expression"),
  };




// regex_exception ------------------------------------------------------------
regex_exception::regex_exception(TErrorCode error,
  char const *info,char *module,TIndex line)
: base_exception(error,info,module,line,"RE") {
  }




char *regex_exception::getText() const {
  return _(RegexPlainText[Error]);
  }




// regex_string::class_matcher ------------------------------------------------
regex_string::class_matcher::class_matcher()
  : Negated(false) {
  MatchLength = 1;
  }




regex_string::class_matcher::class_matcher(string const &cls)
  : Negated(false) {
  MatchLength = 1;
  
  if (cls.size() && cls[0] == XSTRRE_CLASSNEG) {
    expandClass(cls.substr(1));
    Negated = true;
    }
  else
    expandClass(cls);
  }




ixion::regex<string>::matcher *ixion::regex_string::class_matcher::duplicate() const {
  class_matcher *dupe = new class_matcher();
  dupe->copy(this);
  return dupe;
  }




bool regex_string::class_matcher::match(backref_stack &brstack,string const &candidate,TIndex at) {
  if (at >= candidate.size()) return false;
  
  bool result = Set[candidate[at]];
  if (Negated) result = !result;
  
  return result && matchNext(brstack,candidate,at+1);
  }




void regex_string::class_matcher::expandClass(string const &cls) {
  memset(&Set,0,sizeof(Set));
  
  if (cls.size() == 0) return;
  Set[cls[0]] = true;
  char lastchar = cls[0];

  for (TIndex index = 1;index < cls.size();index++) {
    if ((cls[index] == XSTRRE_CLASSRANGE) && (index < cls.size()-1)) {
      for (char ch = lastchar+1;ch < cls[index+1];ch++)
        Set[ch] = true;
      }
    else Set[cls[index]] = true;
    lastchar = cls[index];
    }
  }




void ixion::regex_string::class_matcher::copy(class_matcher const *src) {
  super::copy(src);
  for (TIndex i = 0;i < CharValues;i++) 
    Set[i] = src->Set[i];
  Negated = src->Negated;
  }




// regex_string::special_class_matcher ----------------------------------------
regex_string::special_class_matcher::special_class_matcher(type tp)
  : Type(tp) {
  MatchLength = 1;
  }




ixion::regex<string>::matcher *ixion::regex_string::special_class_matcher::duplicate() const {
  special_class_matcher *dupe = new special_class_matcher(Type);
  dupe->copy(this);
  return dupe;
  }




bool regex_string::special_class_matcher::match(backref_stack &brstack,string const &candidate,TIndex at) {
  if (at >= candidate.size()) return false;
        enum type { DIGIT,NONDIGIT,ALNUM,NONALNUM,SPACE,NONSPACE };
  
  bool result;
  switch (Type) {
    case DIGIT: result = isdigit(candidate[at]);
      break;
    case NONDIGIT: result = !isdigit(candidate[at]);
      break;
    case ALNUM: result = isalnum(candidate[at]);
      break;
    case NONALNUM: result = !isalnum(candidate[at]);
      break;
    case SPACE: result = isspace(candidate[at]);
      break;
    case NONSPACE: result = !isspace(candidate[at]);
      break;
    default:
      EX_THROW(regex,ECRE_INVESCAPE)
    }
  
  return result && matchNext(brstack,candidate,at+1);
  }




// regex_string ---------------------------------------------------------------
void regex_string::parse(string const &expr) {
  auto_ptr<matcher> new_re(parseRegex(expr));
  ParsedRegex = new_re;
  }




string regex_string::replaceAll(string const &candidate,string const &replacement,TIndex from) {
  string result;
  string tempreplacement;

  LastCandidate = candidate;
  if (ParsedRegex.get() == NULL)
    EX_THROW(regex,ECRE_NOPATTERN)

  for (TIndex index = from;index < candidate.size();) {
    BackrefStack.clear();
    if (ParsedRegex->match(BackrefStack,candidate,index)) {
      TIndex matchlength = ParsedRegex->subsequentMatchLength();
      tempreplacement = replacement;

      TSize backrefs = BackrefStack.size();
      for (TIndex i = 0;i < backrefs;i++)
        tempreplacement = findReplace(tempreplacement,XSTRRE_BACKREF+unsigned2dec(i),
          BackrefStack.get(i,LastCandidate));

      result += tempreplacement;
      index += matchlength;
      }
    else result += candidate[index++];
    }
  return result;
  }




regex_string::matcher *regex_string::parseRegex(string const &expr) {
  if (!expr.size()) return NULL;
  TIndex index = 0;
  matcher *firstobject,*lastobject = NULL;
  alternative_matcher *alternative = NULL;

  while (index < expr.size()) {
    matcher *object = NULL;
    quantifier *quantifier = NULL;
    bool quantified = true;
    char ch;

    // several objects may be inserted in one loop run
    switch (expr[index++]) {
      // case XSTRRE_BACKREF: (dupe)
      // case XSTRRE_ESCAPESEQ: (dupe)
      case XSTRRE_LITERAL: {
          if (index >= expr.size()) EX_THROW(regex,ECRE_INVESCAPE)

          ch = expr[index++];
          if (isdigit(ch))
            object = new backref_matcher(ch-'0');
          else {
	    switch (ch) {
	      case 'd': object = new special_class_matcher(special_class_matcher::DIGIT);
	        break;
	      case 'D': object = new special_class_matcher(special_class_matcher::NONDIGIT);
	        break;
	      case 'w': object = new special_class_matcher(special_class_matcher::ALNUM);
	        break;
	      case 'W': object = new special_class_matcher(special_class_matcher::NONALNUM);
	        break;
	      case 's': object = new special_class_matcher(special_class_matcher::SPACE);
	        break;
	      case 'S': object = new special_class_matcher(special_class_matcher::NONSPACE);
	        break;
	      default: object = new sequence_matcher(string(1,ch));
	      }
	    }
          break;
        }
      case XSTRRE_ANYCHAR:
        object = new any_matcher;
        break;
      case XSTRRE_START:
        quantified = false;
        object = new start_matcher;
        break;
      case XSTRRE_END:
        quantified = false;
        object = new end_matcher;
        break;
      case XSTRRE_ALTERNATIVE: {
          if (!alternative) 
            alternative = new alternative_matcher;
          alternative->addAlternative(firstobject);
          firstobject = NULL;
          lastobject = NULL;
          break;
          }
      case XSTRRE_CLASSSTART: {
          TIndex classend = expr.find(XSTRRE_CLASSSTOP,index);
          if (classend == string::npos)
            EX_THROW(regex,ECRE_UNTERMCLASS)
          object = new class_matcher(expr.substr(index,classend-index));
  
          index = classend+1;
          break;
          }
      case XSTRRE_BACKREFSTART: {
          matcher *parsed;

          TSize brlevel = 1;
          for (TIndex searchstop = index;searchstop < expr.size();searchstop++) {
            if ((expr[searchstop] == XSTRRE_BACKREFSTART) &&
            (expr[searchstop-1] != XSTRRE_LITERAL))
              brlevel++;
            if ((expr[searchstop] == XSTRRE_BACKREFSTOP) &&
            (expr[searchstop-1] != XSTRRE_LITERAL)) {
              brlevel--;
              if (brlevel == 0) {
                parsed = parseRegex(expr.substr(index,searchstop-index));
                if (!parsed) EX_THROW(regex,ECRE_INVBACKREF)

                index = searchstop+1;
                break;
                }
              }
            }

          if (!parsed) EX_THROW(regex,ECRE_UNBALBACKREF)

          object = new backref_open_matcher;
          object->setNext(parsed);

          matcher *closer = new backref_close_matcher;

          matcher *searchlast = parsed,*foundlast;
          while (searchlast) {
            foundlast = searchlast;
            searchlast = searchlast->getNext();
            }
          foundlast->setNext(closer);

          break;
          }
      case XSTRRE_BACKREFSTOP:
        EX_THROW(regex,ECRE_UNBALBACKREF)
      default:
        object = new sequence_matcher(expr.substr(index-1,1));
        break;
      }

    if (object) {
      if (quantified) quantifier = parseQuantifier(expr,index);
      if (quantifier) {
        quantifier->setQuantified(object);
        if (lastobject) lastobject->setNext(quantifier);
        else firstobject = quantifier;
        }
      else {
        if (lastobject) lastobject->setNext(object);
        else firstobject = object;
        }
      }

    // we need this for the alternative matcher, which also inserts
    // its connector
    matcher *searchlast = quantifier ? quantifier : object;
    while (searchlast) {
      lastobject = searchlast;
      searchlast = searchlast->getNext();
      }
    }
  if (alternative) {
    alternative->addAlternative(firstobject);
    return alternative;
    }
  else return firstobject;
  }




regex_string::quantifier *regex_string::parseQuantifier(string const &expr,TIndex &at) {
  quantifier *quant = NULL;
  if (at == expr.size()) return NULL;
  if (expr[at] == XSTRREQ_0PLUS) {
    quant = new quantifier(isGreedy(expr,++at),0);
    return quant;
    }
  if (expr[at] == XSTRREQ_1PLUS) {
    quant = new quantifier(isGreedy(expr,++at),1);
    return quant;
    }
  if (expr[at] == XSTRREQ_01) {
    quant = new quantifier(isGreedy(expr,++at),0,1);
    return quant;
    }
  if (expr[at] == XSTRREQ_START) {
    TSize min,max;

    at++;
    TIndex endindex;
    endindex = expr.find(XSTRREQ_STOP,at);
    if (endindex == string::npos) 
      EXGEN_THROW(ECRE_INVQUANTIFIER)

    string quantspec = expr.substr(at,endindex-at);
    at = endindex+1;

    try {
      string::size_type rangeindex = quantspec.find(XSTRREQ_RANGE);
      if (rangeindex == string::npos) {
        min = evalUnsigned(quantspec);
        quant = new quantifier(isGreedy(expr,at),min,min);
        }
      else if (rangeindex == quantspec.size()-1) {
        min = evalUnsigned(quantspec.substr(0,rangeindex));
        quant = new quantifier(isGreedy(expr,at),min);
        }
      else {
        min = evalUnsigned(quantspec.substr(0,rangeindex));
        max = evalUnsigned(quantspec.substr(rangeindex+1));
        quant = new quantifier(isGreedy(expr,at),min,max);
        }
      return quant;
      }
    EX_CONVERT(generic,EC_CANNOTEVALUATE,regex,ECRE_INVQUANTIFIER)
    }
  return NULL;
  }




bool regex_string::isGreedy(string const &expr,TIndex &at)
{
  if (at == expr.size()) return true;
  if (expr[at] == XSTRREQ_NONGREEDY) {
    at++;
    return false;
  }
  else return true;
}
