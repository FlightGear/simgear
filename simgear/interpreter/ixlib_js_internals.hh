// ----------------------------------------------------------------------------
//  Description      : Javascript interpreter
// ----------------------------------------------------------------------------
//  (c) Copyright 2000 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_JS_INTERNALS
#define IXLIB_JS_INTERNALS




#include <ixlib_javascript.hh>




using namespace std;




namespace ixion {
  namespace javascript {
    struct code_location {
      TIndex	Line;
      
      code_location(scanner::token &tok);
      explicit code_location(TIndex line);
      string stringify() const;
      };

    struct return_exception {
      ref<value>        ReturnValue;
      code_location	Location;
      
      return_exception(ref<value> retval,code_location const &loc)
        : ReturnValue(retval),Location(loc) {
	}
      };

    struct break_exception {
      bool              HasLabel;
      string            Label;
      code_location	Location;
      
      break_exception(bool has_label,string const &label,code_location const &loc)
        : HasLabel(has_label),Label(label),Location(loc) {
	}
      };
    
    struct continue_exception {
      bool              HasLabel;
      string            Label;
      code_location	Location;

      continue_exception(bool has_label,string const &label,code_location const &loc)
        : HasLabel(has_label),Label(label),Location(loc) {
	}
      };

    // values -----------------------------------------------------------------
    class null : public value {
      private:
        typedef value super;
        
      public:
        value_type getType() const;
        bool toBoolean() const;
        
        ref<value> duplicate();
      };
    
    class const_floating_point : public value_with_methods {
      private:
        typedef value_with_methods      super;

      protected:
        double                          Value;
      
      public:
        const_floating_point(double value);
    
        value_type getType() const;
        int toInt() const;
        double toFloat() const;
        bool toBoolean() const;
        string toString() const;
        
        ref<value> duplicate();
        
        ref<value> callMethod(string const &identifier,parameter_list const &parameters);
        
        ref<value> operatorUnary(operator_id op) const;
        ref<value> operatorBinary(operator_id op,ref<value> op2) const;
      };

    class floating_point : public const_floating_point {
      private:
        typedef const_floating_point    super;
        
      public:
        floating_point(double value);

        ref<value> operatorUnaryModifying(operator_id op);
        ref<value> operatorBinaryModifying(operator_id op,ref<value> op2);
      };
      
    class const_integer : public value_with_methods {
      private:
        typedef value_with_methods      super;

      protected:
        long                            Value;
      
      public:
        const_integer(long value);
    
        value_type getType() const;
        int toInt() const;
        double toFloat() const;
        bool toBoolean() const;
        string toString() const;
        
        ref<value> duplicate();
        
        ref<value> callMethod(string const &identifier,parameter_list const &parameters);
        
        ref<value> operatorUnary(operator_id op) const;
        ref<value> operatorBinary(operator_id op,ref<value> op2) const;
      };

    class integer : public const_integer {
      private:
        typedef const_integer   super;
        
      public:
        integer(long value);

        ref<value> operatorUnaryModifying(operator_id op);
        ref<value> operatorBinaryModifying(operator_id op,ref<value> op2);
      };
      
    class js_string : public value_with_methods {
      private:
        typedef value_with_methods      super;
        
      protected:
        string                          Value;
      
      public:
        js_string(string const &value);
    
        value_type getType() const;
        string toString() const;
        bool toBoolean() const;
        string stringify() const;

        ref<value> duplicate();
        
        ref<value> lookup(string const &identifier);
        ref<value> callMethod(string const &identifier,parameter_list const &parameters);

        ref<value> operatorBinary(operator_id op,ref<value> op2) const;
        ref<value> operatorBinaryModifying(operator_id op,ref<value> op2);
      };
      
    class lvalue : public value {
      protected:
        ref<value>      Reference;
      
      public:
        lvalue(ref<value> ref);
        
        value_type getType() const;
        string toString() const;
        int toInt() const;
        double toFloat() const;
        bool toBoolean() const;
        string stringify() const;

        ref<value> eliminateWrappers();
        ref<value> duplicate();
        
        ref<value> lookup(string const &identifier);
        ref<value> subscript(value const &index);
        ref<value> call(parameter_list const &parameters);
        ref<value> callAsMethod(ref<value> instance,parameter_list const &parameters);
        ref<value> construct(parameter_list const &parameters);
        ref<value> assign(ref<value> op2);

        ref<value> operatorUnary(operator_id op) const;
        ref<value> operatorBinary(operator_id op,ref<value> op2) const;
        ref<value> operatorBinaryShortcut(operator_id op,expression const &op2,context const &ctx) const;
        ref<value> operatorUnaryModifying(operator_id op);
        ref<value> operatorBinaryModifying(operator_id op,ref<value> op2);
      };

    class constant_wrapper : public value {
      protected:
        ref<value>      Constant;

      public:
        constant_wrapper(ref<value> val);
        
        value_type getType() const;
        string toString() const;
        int toInt() const;
        double toFloat() const;
        bool toBoolean() const;
        string stringify() const;

        ref<value> eliminateWrappers();
        ref<value> duplicate();
        
        ref<value> lookup(string const &identifier);
        ref<value> subscript(value const &index);
        ref<value> call(parameter_list const &parameters) const;
        ref<value> callAsMethod(ref<value> instance,parameter_list const &parameters);
        ref<value> construct(parameter_list const &parameters);
        ref<value> assign(ref<value> value);

        ref<value> operatorUnary(operator_id op) const;
        ref<value> operatorBinary(operator_id op,ref<value> op2) const;
        ref<value> operatorBinaryShortcut(operator_id op,expression const &op2,context const &ctx) const;
        ref<value> operatorUnaryModifying(operator_id op);
        ref<value> operatorBinaryModifying(operator_id op,ref<value> op2);
      };

    class callable_with_parameters : public value {
      public:
        typedef vector<string>          parameter_name_list;
        
      protected:
        parameter_name_list             ParameterNameList;
      
      public:
        callable_with_parameters(parameter_name_list const &pnames);
        
        void addParametersTo(list_scope &scope,parameter_list const &parameters) const;
        static ref<value> evaluateBody(expression &body,context const &ctx);
      };
    
    class function : public callable_with_parameters {
        typedef callable_with_parameters 	super;
        ref<expression>                 	Body;
        ref<value>                      	LexicalScope;
      
      public:
        function(parameter_name_list const &pnames,ref<expression> body,ref<value> lex_scope);

        value_type getType() const{
          return VT_FUNCTION;
          }
        
        ref<value> duplicate();
        
        ref<value> call(parameter_list const &parameters);
      };

    class method : public callable_with_parameters {
        typedef callable_with_parameters 	super;
        ref<expression>                 	Body;
        ref<value>                      	LexicalScope;
      
      public:
        method(parameter_name_list const &pnames,ref<expression> body,ref<value> lex_scope);

        value_type getType() const{
          return VT_FUNCTION;
          }
        
        ref<value> duplicate();
        
        ref<value> callAsMethod(ref<value> instance,parameter_list const &parameters);
      };

    class constructor : public callable_with_parameters {
        typedef callable_with_parameters        super;
        ref<expression>                         Body;
        ref<value>                      	LexicalScope;
      
      public:
        constructor(parameter_name_list const &pnames,ref<expression> body,ref<value> lex_scope);

        value_type getType() const{
          return VT_FUNCTION;
          }
        
        ref<value> duplicate();
        ref<value> callAsMethod(ref<value> instance,parameter_list const &parameters);
      };

    class js_class : public value {
        class super_instance_during_construction : public value {
          // this object constructs the superclass 
          // a) if it is called, by calling the super constructor
          //    with the aprropriate parameters
          // b) implicitly with no super constructor arguments, 
          //    if the super object is referenced explicitly
          
            ref<value>                  SuperClass;
            ref<value>                  SuperClassInstance;

          public:
            super_instance_during_construction(ref<value> super_class);

            value_type getType() const {
              return VT_OBJECT;
              }
            
            ref<value> call(parameter_list const &parameters);
            ref<value> lookup(string const &identifier);
            
            ref<value> getSuperClassInstance();
          };

        typedef vector<ref<expression> > declaration_list;
    
        ref<value>                      LexicalScope;
        ref<value>                      SuperClass;
        ref<value>                      Constructor;
        ref<value>                      StaticMethodScope;
        ref<value>                      MethodScope;
        ref<value>                      StaticVariableScope;
        declaration_list                VariableList;
        
      public:
        js_class(ref<value> lex_scope,ref<value> super_class,ref<value> constructor,
          ref<value> static_method_scope,ref<value> method_scope,
          ref<value> static_variable_scope,declaration_list const &variable_list);
      
        value_type getType() const {
          return VT_TYPE;
          }
        
        ref<value> duplicate();
        ref<value> lookup(string const &identifier);
        ref<value> lookupLocal(string const &identifier);
        ref<value> construct(parameter_list const &parameters);
      };
    
    class js_class_instance : public value {
        class bound_method : public value {
            ref<value>                  Instance;
            ref<value>                  Method;
            
          public:
            bound_method(ref<value> instance,ref<value> method);

            value_type getType() const {
              return VT_BOUND_METHOD;
              }

            ref<value> call(parameter_list const &parameters);
          };

        ref<value>                      SuperClassInstance;
        ref<js_class,value>             Class;
        ref<value>                      MethodScope;
        ref<value>                      VariableScope;
        
      public:
        js_class_instance(ref<js_class,value> cls,ref<value> method_scope,
          ref<value> variable_scope);
          
        void setSuperClassInstance(ref<value> super_class_instance);
        
        value_type getType() const {
          return VT_OBJECT;
          }
          
        ref<value> duplicate();
        ref<value> lookup(string const &identifier);
      };
    
    class js_array_constructor : public value {
      public:
        value_type getType() const {
          return VT_TYPE;
          }
        
        ref<value> duplicate();
        ref<value> construct(parameter_list const &parameters);
      };

    // expressions ----------------------------------------------------------
    class expression {
        code_location		Location;
	
      public:
        expression(code_location const &loc);
        virtual ~expression();
        virtual ref<value> evaluate(context const &ctx) const = 0;
	
	code_location const &getCodeLocation() const {
	  return Location;
	  }
      };

    class constant : public expression {
        ref<value>              Value;
      public:
        constant(ref<value> val,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };
    
    class unary_operator : public expression {
        value::operator_id      Operator;
        ref<expression>         Operand;
      
      public:
        unary_operator(value::operator_id opt,ref<expression> opn,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class modifying_unary_operator : public expression {
        value::operator_id      Operator;
        ref<expression>         Operand;
      
      public:
        modifying_unary_operator(value::operator_id opt,ref<expression> opn,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };
      
    class binary_operator : public expression {
        value::operator_id      Operator;
        ref<expression>         Operand1;
        ref<expression>         Operand2;
      
      public:
        binary_operator(value::operator_id opt,ref<expression> opn1,ref<expression> opn2,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class binary_shortcut_operator : public expression {
        value::operator_id      Operator;
        ref<expression>         Operand1;
        ref<expression>         Operand2;
      
      public:
        binary_shortcut_operator(value::operator_id opt,ref<expression> opn1,ref<expression> opn2,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class modifying_binary_operator : public expression {
        value::operator_id      Operator;
        ref<expression>         Operand1;
        ref<expression>         Operand2;
      
      public:
        modifying_binary_operator(value::operator_id opt,ref<expression> opn1,ref<expression> opn2,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };
    
    class ternary_operator : public expression {
        ref<expression>         Operand1;
        ref<expression>         Operand2;
        ref<expression>         Operand3;
      
      public:
        ternary_operator(ref<expression> opn1,ref<expression> opn2,ref<expression> opn3,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };
    
    class subscript_operation : public expression {
        ref<expression>         Operand1;
        ref<expression>         Operand2;
      
      public:
        subscript_operation(ref<expression> opn1,ref<expression> opn2,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class lookup_operation : public expression {
        ref<expression>         Operand;
        string                  Identifier;
      
      public:
        lookup_operation(string const &id,code_location const &loc);
        lookup_operation(ref<expression> opn,string const &id,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };
    
    class assignment : public expression {
        ref<expression>         Operand1;
        ref<expression>         Operand2;
      
      public:
        assignment(ref<expression> opn1,ref<expression> opn2,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };
    
    class basic_call : public expression {
      public:
        typedef vector<ref<expression> >        parameter_expression_list;
        typedef vector<ref<value> >             parameter_value_list;
      
      private:
        parameter_expression_list               ParameterExpressionList;
      
      public:
        basic_call(parameter_expression_list const &pexps,code_location const &loc);
        void makeParameterValueList(context const &ctx,parameter_value_list &pvalues) const;
      };
    
    class function_call : public basic_call {
        typedef basic_call              super;
        ref<expression>                 Function;
      
      public:
        function_call(ref<expression> fun,parameter_expression_list const &pexps,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class construction : public basic_call {
        typedef basic_call              super;
        ref<expression>                 Class;
      
      public:
        construction(ref<expression> cls,parameter_expression_list const &pexps,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    // declarations -----------------------------------------------------------
    class variable_declaration : public expression {
      protected:
        string          Identifier;
        ref<expression> DefaultValue;
      
      public:
        variable_declaration(string const &id,ref<expression> def_value,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class constant_declaration : public expression {
      protected:
        string          Identifier;
        ref<expression> DefaultValue;
      
      public:
        constant_declaration(string const &id,ref<expression> def_value,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class function_declaration : public expression {
      public:
        typedef function::parameter_name_list   parameter_name_list;

      private:
        string                  Identifier;
        parameter_name_list     ParameterNameList;
        ref<expression>         Body;
      
      public:
        function_declaration(string const &id,parameter_name_list const &pnames,
          ref<expression> body,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class method_declaration : public expression {
      public:
        typedef method::parameter_name_list     parameter_name_list;

      private:
        string                  Identifier;
        parameter_name_list     ParameterNameList;
        ref<expression>         Body;
      
      public:
        method_declaration(string const &id,parameter_name_list const &pnames,
          ref<expression> body,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class constructor_declaration : public expression {
      public:
        typedef method::parameter_name_list     parameter_name_list;

      private:
        parameter_name_list     ParameterNameList;
        ref<expression>         Body;
      
      public:
        constructor_declaration(parameter_name_list const &pnames,
          ref<expression> body,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class js_class_declaration : public expression {
        typedef vector<ref<expression> >        declaration_list;

        string                                  Identifier;
        ref<expression>                         SuperClass;
        ref<expression>                         ConstructorDeclaration;
        declaration_list                        StaticMethodList;
        declaration_list                        MethodList;
        declaration_list                        StaticVariableList;
        declaration_list                        VariableList;

      public:
        js_class_declaration(string const &id,ref<expression> superclass,
	  code_location const &loc);
        
        ref<value> evaluate(context const &ctx) const;
        
        void setConstructor(ref<expression> decl);
        void addStaticMethod(ref<expression> decl);
        void addMethod(ref<expression> decl);
        void addStaticVariable(ref<expression> decl);
        void addVariable(ref<expression> decl);
      };
    
    // instructions ---------------------------------------------------------
    class instruction_list : public expression {
        typedef vector<ref<expression> >        expression_list;
        expression_list                         ExpressionList;
      
      public:
        instruction_list(code_location const &loc)
	  : expression(loc) {
	  }
        ref<value> evaluate(context const &ctx) const;
        void add(ref<expression> expr);
      };
    
    class scoped_instruction_list : public instruction_list {
      public:
        scoped_instruction_list(code_location const &loc)
	  : instruction_list(loc) {
	  }
        ref<value> evaluate(context const &ctx) const;
      };
    
    class js_if : public expression {
        ref<expression>         Conditional;
        ref<expression>         IfExpression;
        ref<expression>         IfNotExpression;
      
      public:
        js_if(ref<expression> cond,ref<expression> ifex,ref<expression> ifnotex,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class js_while : public expression {
        ref<expression>         Conditional;
        ref<expression>         LoopExpression;
        bool                    HasLabel;
        string                  Label;
      
      public:
        js_while(ref<expression> cond,ref<expression> loopex,code_location const &loc);
        js_while(ref<expression> cond,ref<expression> loopex,string const &Label,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class js_do_while : public expression {
        ref<expression>         Conditional;
        ref<expression>         LoopExpression;
        bool                    HasLabel;
        string                  Label;
      
      public:
        js_do_while(ref<expression> cond,ref<expression> loopex,code_location const &loc);
        js_do_while(ref<expression> cond,ref<expression> loopex,string const &Label,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class js_for : public expression {
        ref<expression>         Initialization;
        ref<expression>         Conditional;
        ref<expression>         Update;
        ref<expression>         LoopExpression;
        bool                    HasLabel;
        string                  Label;
      
      public:
        js_for(ref<expression> init,ref<expression> cond,ref<expression> update,ref<expression> loop,code_location const &loc);
        js_for(ref<expression> init,ref<expression> cond,ref<expression> update,ref<expression> loop,string const &label,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };
    
    class js_for_in : public expression {
        ref<expression>         Iterator;
        ref<expression>         Array;
        ref<expression>         LoopExpression;
        bool                    HasLabel;
        string                  Label;
      
      public:
        js_for_in(ref<expression> iter,ref<expression> array,ref<expression> loop,code_location const &loc);
        js_for_in(ref<expression> iter,ref<expression> array,ref<expression> loop,string const &label,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };
    
    class js_return : public expression {
        ref<expression>         ReturnValue;
      
      public:
        js_return(ref<expression> retval,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class js_break : public expression {
        bool                    HasLabel;
        string                  Label;
      
      public:
        js_break(code_location const &loc);
        js_break(string const &label,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class js_continue : public expression {
        bool                    HasLabel;
        string                  Label;
      
      public:
        js_continue(code_location const &loc);
        js_continue(string const &label,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };
    
    class break_label : public expression {
        string                  Label;
        ref<expression>         Expression;
      
      public:
        break_label(string const &label,ref<expression> expr,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
      };

    class js_switch : public expression {
        bool                    HasLabel;
        string                  Label;
        ref<expression>         Discriminant;
        
        struct case_label {
          ref<expression>               DiscriminantValue;
          ref<expression>               Expression;
          };
        typedef vector<case_label>      case_list;
        case_list                       CaseList;
      
      public:
        js_switch(ref<expression> discriminant,code_location const &loc);
        js_switch(ref<expression> discriminant,string const &label,code_location const &loc);
        ref<value> evaluate(context const &ctx) const;
        void addCase(ref<expression> dvalue,ref<expression> expr);
      };
    }
  }




#endif
