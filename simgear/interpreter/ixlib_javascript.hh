// ----------------------------------------------------------------------------
//  Description      : Javascript interpreter
// ----------------------------------------------------------------------------
//  (c) Copyright 2000 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_JAVASCRIPT
#define IXLIB_JAVASCRIPT




#include <vector>
#if __GNUC__ < 3
  #include <hash_map>
#else
  #include <ext/hash_map>
#endif
#include <ixlib_string.hh>
#include <ixlib_exbase.hh>
#include <ixlib_garbage.hh>
#include <ixlib_scanner.hh>




// Error codes ----------------------------------------------------------------
#define ECJS_UNTERMINATED_COMMENT		0
#define ECJS_CANNOT_CONVERT			1
#define ECJS_INVALID_OPERATION			2
#define ECJS_UNEXPECTED				3
#define ECJS_UNEXPECTED_EOF			4
#define ECJS_CANNOT_MODIFY_RVALUE		5
#define ECJS_UNKNOWN_IDENTIFIER			6
#define ECJS_UNKNOWN_OPERATOR			7
#define ECJS_INVALID_NON_LOCAL_EXIT		8
#define ECJS_INVALID_NUMBER_OF_ARGUMENTS	9
#define ECJS_INVALID_TOKEN			10
#define ECJS_CANNOT_REDECLARE			11
#define ECJS_DOUBLE_CONSTRUCTION		12
#define ECJS_NO_SUPERCLASS			13
#define ECJS_DIVISION_BY_ZERO                   14




// helpful macros -------------------------------------------------------------
#define IXLIB_JS_ASSERT_PARAMETERS(NAME,ARGMIN,ARGMAX) \
  if (parameters.size() < ARGMIN || parameters.size() > ARGMAX) \
    EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,NAME)

#define IXLIB_JS_IF_METHOD(NAME,ARGMIN,ARGMAX) \
  if (identifier == NAME) \
    if (parameters.size() < ARGMIN || parameters.size() > ARGMAX) \
      EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,NAME) \
    else

#define IXLIB_JS_DECLARE_FUNCTION(NAME) \
  namespace { \
    class NAME : public value { \
      public: \
        value_type getType() const { \
          return VT_FUNCTION; \
          } \
        ixion::ref<ixion::javascript::value> call(parameter_list const &parameters); \
      }; \
    } \
  ixion::ref<ixion::javascript::value> NAME::call(parameter_list const &parameters)

#define IXLIB_JS_CONVERT_PARAMETERS_0 \
  
  


// Exception throw macros -----------------------------------------------------
#define EXJS_THROW(CODE)\
  EX_THROW(javascript,CODE)
#define EXJS_THROWINFO(CODE,INFO)\
  EX_THROWINFO(javascript,CODE,INFO)
#define EXJS_THROW_NO_LOCATION(CODE)\
  EX_THROW(no_location_javascript,CODE)
#define EXJS_THROWINFO_NO_LOCATION(CODE,INFO)\
  EX_THROWINFO(no_location_javascript,CODE,INFO)
#define EXJS_THROWINFOLOCATION(CODE,INFO,LOCATION)\
  throw ixion::javascript_exception(CODE,LOCATION,INFO,__FILE__,__LINE__);
#define EXJS_THROWINFOTOKEN(CODE,INFO,TOKEN)\
  EXJS_THROWINFOLOCATION(CODE,INFO,code_location(TOKEN))
#define EXJS_THROWINFOEXPRESSION(CODE,INFO,EXPR)\
  EXJS_THROWINFOLOCATION(CODE,INFO,(EXPR).getCodeLocation())





namespace ixion {
  namespace javascript {
    struct code_location;
    }
  
  // exceptions ---------------------------------------------------------------
  struct no_location_javascript_exception : public base_exception {
    no_location_javascript_exception(TErrorCode error,char const *info = NULL,char *module = NULL,
      TIndex line = 0)
      : base_exception(error,info,module,line,"JS") {
      }
    virtual char *getText() const;
    };




  struct javascript_exception : public base_exception {
    javascript_exception(TErrorCode error,char const *info = NULL,char *module = NULL,
      TIndex line = 0)
      : base_exception(error,info,module,line,"JS") {
      }
    javascript_exception(TErrorCode error,javascript::code_location const &loc,char const *info = 0,char *module = NULL,
      TIndex line = 0);
    javascript_exception(no_location_javascript_exception const &half_ex,javascript::code_location const &loc);
    virtual char *getText() const;
    };




  // javascript ---------------------------------------------------------------
  /**
  This code tries to be an implementation of ECMAScript 4, as available at 
  http://www.mozilla.org/js/language/
  Note that ES4 is still in the process of standardization. 

  It is meant to behave like an ES4 interpreter in strict mode, none
  of the backward-compatible braindead-isms like newline semicolon
  insertion and other stuff will ever be implemented.

  This is the list of its shortcomings:
  <ul>
    <li> exceptions
    <li> namespaces,packages
    <li> constness
    <li> Number/String constructor and class methods
    <li> real regexp's
    <li> the methods listed in FIXME's (js_library.cc js_value.cc)
    <li> cannot cross-assign predefined methods [won't be]
    <li> Grammatical semicolon insertion [won't be]
    <li> type declaration [won't be]
    </ul>

  Be advised that a javascript value that is passed to you through the
  interpreter, e.g. as a call parameter, may not be of the type that
  you expect. For example, in "var x = 4; f(x);", what comes in as
  the parameter x into f is a wrapper value that adds assign()ability
  to a value that is wrapped inside. The advised solution to get the
  object that you expect is to call eliminateWrappers() on the potentially
  wrapped value.
  */
  namespace javascript {
    class value;
    class list_scope;
    struct context {
      ref<list_scope,value>     DeclarationScope;
      ref<value>                LookupScope;
      
      context(ref<list_scope,value> scope);
      context(ref<value> scope);
      context(ref<list_scope,value> decl_scope,ref<value> lookup_scope);
      };

    class expression;
    
    class value {
      public:
        enum operator_id { 
          // unary, modifying
          OP_PRE_INCREMENT,OP_POST_INCREMENT,
          OP_PRE_DECREMENT,OP_POST_DECREMENT,
          // unary, non-modifying
          OP_UNARY_PLUS,OP_UNARY_MINUS,
          OP_LOG_NOT,OP_BIN_NOT,
          // binary, modifying
          OP_PLUS_ASSIGN,OP_MINUS_ASSIGN,
          OP_MUTLIPLY_ASSIGN,OP_DIVIDE_ASSIGN,OP_MODULO_ASSIGN,
          OP_BIT_AND_ASSIGN,OP_BIT_OR_ASSIGN,OP_BIT_XOR_ASSIGN,
          OP_LEFT_SHIFT_ASSIGN,OP_RIGHT_SHIFT_ASSIGN,
          // binary, non-modifying
          OP_PLUS,OP_MINUS,
          OP_MULTIPLY,OP_DIVIDE,OP_MODULO,
          OP_BIT_AND,OP_BIT_OR,OP_BIT_XOR,
          OP_LEFT_SHIFT,OP_RIGHT_SHIFT,
          OP_LOGICAL_OR,OP_LOGICAL_AND,
          OP_EQUAL,OP_NOT_EQUAL,OP_IDENTICAL,OP_NOT_IDENTICAL,
          OP_LESS_EQUAL,OP_GREATER_EQUAL,OP_LESS,OP_GREATER,
          // special
          OP_ASSIGN,
          };
        
        enum value_type {
          VT_UNDEFINED,VT_NULL,
          VT_INTEGER,VT_FLOATING_POINT,VT_STRING,
          VT_FUNCTION,VT_OBJECT,VT_BUILTIN,VT_HOST,
	  VT_SCOPE,VT_BOUND_METHOD,VT_TYPE
          };
        typedef std::vector<ref<value> >	parameter_list;

        virtual ~value() {
          }
      
        virtual value_type getType() const = 0;
        virtual std::string toString() const;
        virtual int toInt() const;
        virtual double toFloat() const;
        virtual bool toBoolean() const;
	// toString is meant as a type conversion, whereas stringify
	// is for debuggers and the like
	virtual std::string stringify() const;
        
        virtual ref<value> eliminateWrappers();
        virtual ref<value> duplicate();

        virtual ref<value> lookup(std::string const &identifier);
        virtual ref<value> subscript(value const &index);
        virtual ref<value> call(parameter_list const &parameters);
        virtual ref<value> callAsMethod(ref<value> instance,parameter_list const &parameters);
        virtual ref<value> construct(parameter_list const &parameters);
        virtual ref<value> assign(ref<value> op2);
        
        virtual ref<value> operatorUnary(operator_id op) const;
        virtual ref<value> operatorBinary(operator_id op,ref<value> op2) const;
        virtual ref<value> operatorBinaryShortcut(operator_id op,expression const &op2,context const &ctx) const;
        virtual ref<value> operatorUnaryModifying(operator_id op);
        virtual ref<value> operatorBinaryModifying(operator_id op,ref<value> op2);
        
        static operator_id token2operator(scanner::token const &token,bool unary = false,bool prefix = false);
        static std::string operator2string(operator_id op);
	static std::string valueType2string(value_type vt);
      };
    
    // obviously, any value can have methods, but with this neat little
    // interface implementing methods has just become easier.
    class value_with_methods : public value {
        class bound_method : public value {
            std::string				Identifier;
            ref<value_with_methods,value>	Parent;
            
          public:
            bound_method(std::string const &identifier,ref<value_with_methods,value> parent);

            value_type getType() const {
              return VT_BOUND_METHOD;
              }

            ref<value> duplicate();
            ref<value> call(parameter_list const &parameters);
          };
            
      public:
        ref<value> lookup(std::string const &identifier);
        virtual ref<value> callMethod(std::string const &identifier,parameter_list const &parameters) = 0;
      };

    // obviously, any value can already represent a scope ("lookup" member!).
    // the list_scope class is an explicit scope that can "swallow" 
    // (=unite with) other scopes and keeps a list of registered members
    class list_scope : public value {
      protected:
        typedef std::hash_map<std::string,ref<value>,string_hash> member_map;
        typedef std::vector<ref<value> >			swallowed_list;
        
        member_map	MemberMap;
        swallowed_list	SwallowedList;
        
      public:
        value_type getType() const {
	  return VT_SCOPE;
	  }

        ref<value> lookup(std::string const &identifier);

        void unite(ref<value> scope);
        void separate(ref<value> scope);
	void clearScopes();
        
        bool hasMember(std::string const &name) const;
	void addMember(std::string const &name,ref<value> member);
        void removeMember(std::string const &name);
	void clearMembers();
	
	void clear();
      };
    
    class js_array : public value_with_methods {
      private:
        typedef value_with_methods		super;
        
      protected:
        typedef std::vector<ref<value> >	value_array;
        value_array				Array;
  
      public:
        js_array() {
          }
        js_array(TSize size);
        js_array(value_array::const_iterator first,value_array::const_iterator last)
          : Array(first,last) {
          }
        js_array(js_array const &src)
          : Array(src.Array) {
          }
        
        value_type getType() const {
          return VT_BUILTIN;
          }

	std::string stringify() const;
        
        ref<value> duplicate();
  
        ref<value> lookup(std::string const &identifier);
        ref<value> subscript(value const &index);
        ref<value> callMethod(std::string const &identifier,parameter_list const &parameters);
  
        TSize size() const {
	  return Array.size();
	  }
	void resize(TSize size);
	ref<value> &operator[](TIndex idx);
        void push_back(ref<value> val);
      };

    class expression;
    
    ref<value> makeUndefined();
    ref<value> makeNull();
    ref<value> makeValue(signed long val);
    ref<value> makeConstant(signed long val);
    ref<value> makeValue(signed int val);
    ref<value> makeConstant(signed int val);
    ref<value> makeValue(unsigned long val);
    ref<value> makeConstant(unsigned long val);
    ref<value> makeValue(unsigned int val);
    ref<value> makeConstant(unsigned int val);
    ref<value> makeValue(double val);
    ref<value> makeConstant(double val);
    ref<value> makeValue(std::string const &val);
    ref<value> makeConstant(std::string const &val);
    ref<value> makeArray(TSize size = 0);
    ref<value> makeLValue(ref<value> target);
    ref<value> wrapConstant(ref<value> val);

    class interpreter {
      public:
        ref<list_scope,value>			RootScope;
      
      public:
        interpreter();
	~interpreter();
        
        ref<expression> parse(std::string const &str);
        ref<expression> parse(std::istream &istr);
        ref<value> execute(std::string const &str);
        ref<value> execute(std::istream &istr);
        ref<value> execute(ref<expression> expr);
    
      private:
        ref<value> evaluateCatchExits(ref<expression> expr);
      };

    void addGlobal(interpreter &ip);
    void addMath(interpreter &ip);
    void addStandardLibrary(interpreter &ip);
    }
  }




#endif
