// ----------------------------------------------------------------------------
//  Description      : Regular expressions string object
// ----------------------------------------------------------------------------
//  (c) Copyright 1998 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_RE
#define IXLIB_RE




#include <vector>
#include <memory>
#include <ixlib_exgen.hh>
#include <ixlib_string.hh>




// Regex exceptions -----------------------------------------------------------
#define ECRE_INVQUANTIFIER              0
#define ECRE_UNBALBACKREF               1
#define ECRE_INVESCAPE                  2
#define ECRE_INVBACKREF                 3
#define ECRE_UNTERMCLASS                4
#define ECRE_NOPATTERN			5




namespace ixion {
  class regex_exception : public base_exception {
    public:
    regex_exception(TErrorCode error,char const *info = NULL,char *module = NULL,TIndex line = 0);
    virtual char *getText() const;
    };
  }




// Regex token macros ---------------------------------------------------------
#define XSTRRE_LITERAL          '\\'
#define XSTRRE_BACKREF          '\\'
#define XSTRRE_ESCAPESEQ        '\\'
#define XSTRRE_ANYCHAR          '.'
#define XSTRRE_START            '^'
#define XSTRRE_END              '$'
#define XSTRRE_ALTERNATIVE      '|'
#define XSTRRE_CLASSSTART       '['
#define XSTRRE_CLASSNEG         '^'
#define XSTRRE_CLASSRANGE       '-'
#define XSTRRE_CLASSSTOP        ']'

#define XSTRRE_BACKREFSTART     '('
#define XSTRRE_BACKREFSTOP      ')'

#define XSTRREQ_0PLUS           '*'
#define XSTRREQ_1PLUS           '+'
#define XSTRREQ_01              '?'
#define XSTRREQ_START           '{'
#define XSTRREQ_RANGE           ','
#define XSTRREQ_STOP            '}'
#define XSTRREQ_NONGREEDY       '?'




namespace ixion {
  /**
  A class implementing a generic regular expression matcher not only for strings.
  If you are looking for a usual regular expresion parser, look at 
  ixion::regex_string.
  
  If you query anything about the last match, and that last match did
  never happen, behavior is undefined.
  */
  
  template<class T>
  class regex {
    protected:
      // various helper classes -----------------------------------------------
      class backref_stack {
        private:
          struct backref_entry {
            enum { OPEN,CLOSE }           	Type;
            TIndex                        	Index;
            };
          
          typedef std::vector<backref_entry>	internal_stack;
          
          internal_stack                  	Stack;
        
	public:
          typedef TSize                   	rewind_info;
          
          void open(TIndex index);
          void close(TIndex index);
          
          rewind_info getRewindInfo() const;
          void rewind(rewind_info ri);
          void clear();
          
          TSize size();
          T get(TIndex number,T const &candidate) const;
        };




      // matchers -------------------------------------------------------------
      class matcher {
        protected:
          matcher                   *Next;
          bool                      OwnNext;
          TSize                     MatchLength;
      
        public:
          matcher();
          virtual ~matcher();
	  
	  virtual matcher *duplicate() const = 0;
          
          TSize getMatchLength() const { 
            return MatchLength; 
            }
          TSize subsequentMatchLength() const;
          virtual TSize minimumMatchLength() const = 0;
          TSize minimumSubsequentMatchLength() const;
  
          matcher *getNext() const { 
            return Next; 
            }
          virtual void setNext(matcher *next,bool ownnext = true) {
            Next = next;
            OwnNext = ownnext;
            }
          
	  // this routine must set the MatchLength member correctly.
	  virtual bool match(backref_stack &brstack,T const &candidate,TIndex at)
            = 0;
      
        protected:
          bool matchNext(backref_stack &brstack,T const &candidate,TIndex at) const { 
            return Next ? Next->match(brstack,candidate,at) : true; 
            }
	  void copy(matcher const *src);
        };
    
  
  
  
      class quantifier : public matcher {
        private:
	  typedef matcher	    super;
          bool                      Greedy,MaxValid;
          TSize                     MinCount,MaxCount;
          matcher                   *Quantified;
    
        struct backtrack_stack_entry {
          TIndex                          Index;
          backref_stack::rewind_info      RewindInfo;
          };
  
        public:
	  quantifier() 
	    : Quantified(NULL) {
	    }
          quantifier(bool greedy,TSize mincount);
          quantifier(bool greedy,TSize mincount,TSize maxcount);
          ~quantifier();
      
	  matcher *duplicate() const;
          
          TSize minimumMatchLength() const;
          
          void setQuantified(matcher *quantified) { 
            Quantified = quantified;
            }
          bool match(backref_stack &brstack,T const &candidate,TIndex at);

	protected:
	  void copy(quantifier const *src);
        };
    
  
  
  
      class sequence_matcher : public matcher {
          T               MatchStr;
      
        public:
          sequence_matcher(T const &matchstr);

	  matcher *duplicate() const;

          TSize minimumMatchLength() const {
            return MatchStr.size();
            }
          bool match(backref_stack &brstack,T const &candidate,TIndex at);
        };
    
  
  
  
      class any_matcher : public matcher {
        public:
          any_matcher() { 
            MatchLength = 1; 
            }

	  matcher *duplicate() const;

          TSize minimumMatchLength() const {
            return 1;
            }
          bool match(backref_stack &brstack,T const &candidate,TIndex at) { 
            return at < candidate.size() && matchNext(brstack,candidate,at+1); 
            }
        };
    
  
  
  
      class start_matcher : public matcher {
        public:
          start_matcher() { 
            MatchLength = 0; 
            }

	  matcher *duplicate() const;

          TSize minimumMatchLength() const {
            return 0;
            }
          bool match(backref_stack &brstack,T const &candidate,TIndex at);
        };
    
  
  
  
      class end_matcher : public matcher {
        public:
          end_matcher() { 
            MatchLength = 0;
            }

	  matcher *duplicate() const;

          TSize minimumMatchLength() const {
            return 0;
            }
          bool match(backref_stack &brstack,T const &candidate,TIndex at);
        };
    
  
  
  
      class backref_open_matcher : public matcher {
        public:
          backref_open_matcher() { 
            MatchLength = 0; 
            }

	  matcher *duplicate() const;

          TSize minimumMatchLength() const {
            return 0;
            }
          bool match(backref_stack &brstack,T const &candidate,TIndex at);
        };
    
  


      class backref_close_matcher : public matcher {
        public:
          backref_close_matcher() { 
            MatchLength = 0; 
            }

	  matcher *duplicate() const;

          TSize minimumMatchLength() const {
            return 0;
            }
          bool match(backref_stack &brstack,T const &candidate,TIndex at);
        };




      class alternative_matcher : public matcher {
          // The connector serves two purposes:
          // a) be a null-matcher that re-unites the different alternate token
          //    sequences
          // b) make the end of each sequence identifiable to be able to compute
          //    the match length
        
          class connector : public matcher {
            public:
	      matcher *duplicate() const {
	        return NULL;
		}

              TSize minimumMatchLength() const {
                return 0;
                }
              bool match(backref_stack &brstack,T const &candidate,TIndex at);
            };

          typedef matcher		  super;
	  typedef std::vector<matcher *>  alt_list;
          alt_list                        AltList;
          connector                       Connector;
      
        public:
          ~alternative_matcher();
  
	  matcher *duplicate() const;

          TSize minimumMatchLength() const;
          void setNext(matcher *next,bool ownnext = true);
          void addAlternative(matcher *alternative);
          bool match(backref_stack &brstack,T const &candidate,TIndex at);

        protected:
	  void copy(alternative_matcher const *src);
        };
    



      class backref_matcher : public matcher {
          TIndex Backref;
    
        public:
          backref_matcher(TIndex backref);

	  matcher *duplicate() const;

          TSize minimumMatchLength() const {
            return 0;
            }
          bool match(backref_stack &brstack,T const &candidate,TIndex at);
        };
    
      // instance data --------------------------------------------------------
      std::auto_ptr<matcher>      ParsedRegex;
      backref_stack               BackrefStack;
      T                           LastCandidate;
      TIndex                      MatchIndex;
      TSize                       MatchLength;
  
    public:
      // interface ------------------------------------------------------------
      regex();
      regex(regex const &src);

      regex &operator=(regex const &src);
      
      bool match(T const &candidate,TIndex from = 0);
      bool matchAt(T const &candidate,TIndex at = 0);
    
      // Queries pertaining to the last match
      TIndex getMatchIndex() { 
        return MatchIndex; 
	}
      TSize getMatchLength() { 
        return MatchLength; 
	}
      std::string getMatch() { 
        return T(LastCandidate.begin()+MatchIndex,
	   LastCandidate.begin()+MatchIndex+MatchLength); 
	}
      TSize countBackrefs() { 
        return BackrefStack.size(); 
	}
      T getBackref(TIndex index) { 
        return BackrefStack.get(index,LastCandidate); 
	}
    };
  


  /**
  A regular expression parser and matcher.
  
  Backref numbering starts at \0.

  ReplaceAll does not set the MatchIndex/MatchGlobal members.

  What is there is compatible with perl5. (See man perlre or
  http://www.cpan.org/doc/manual/html/pod/perlre.html)
  However, not everything is there. Here's what's missing:
  
  <ul>
    <li> \Q-\E,\b,\B,\A,\Z,\z
    <li> discerning between line and string
    <li> (?#comments)
    <li> (?:clustering)
    <li> (?=positive lookahead assumptions)
    <li> (?!negative lookahead assumptions
    <li> (?<=positive lookbehind assumptions)
    <li> (?<!negative lookbehind assumptions
    <li> (?>independent substrings)
    <li> modifiers such as "case independent"
    </ul>
    
  as well as all the stuff involving perl code, naturally.
  None of these is actually hard to hack in. If you want them,
  pester me or try for yourself (and submit patches!)
  */
  class regex_string : public regex<std::string> {
    private:
      class class_matcher : public regex<std::string>::matcher {
        private:
	  typedef regex<std::string>::matcher	super;
          static TSize const      		CharValues = 256;
          bool                    		Set[CharValues];
          bool                    		Negated;

        public:
	  class_matcher();
          class_matcher(std::string const &cls);

	  matcher *duplicate() const;

          TSize minimumMatchLength() const {
            return 1;
            }
          bool match(backref_stack &brstack,std::string const &candidate,TIndex at);
      
        private:
          void expandClass(std::string const &cls);
	
	protected:
	  void copy(class_matcher const *src);
        };




      class special_class_matcher : public regex<std::string>::matcher {
        public:
          enum type { DIGIT,NONDIGIT,ALNUM,NONALNUM,SPACE,NONSPACE };
  
        private:
          type		Type;
  
        public:
          special_class_matcher(type tp);

	  matcher *duplicate() const;

          TSize minimumMatchLength() const {
            return 1;
            }
          bool match(backref_stack &brstack,std::string const &candidate,TIndex at);
        };




    public:
      regex_string() {
        }
      regex_string(std::string const &str) {
	parse(str);
	}
      regex_string(char const *s) {
	parse(s);
        }

      void parse(std::string const &expr);

      std::string replaceAll(std::string const &candidate,std::string const &replacement,
        TIndex from = 0);

    private:
      regex<std::string>::matcher *parseRegex(std::string const &expr);
      quantifier *parseQuantifier(std::string const &expr,TIndex &at);
      bool isGreedy(std::string const &expr,TIndex &at);
    };
  }



#endif
