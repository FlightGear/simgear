// ----------------------------------------------------------------------------
//  Description      : Javascript interpreter
// ----------------------------------------------------------------------------
//  (c) Copyright 2000 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#include <strstream>
#include "ixlib_i18n.hh"
#include <ixlib_base.hh>
#include <ixlib_scanjs.hh>
#include <ixlib_numconv.hh>
#include <ixlib_js_internals.hh>
#include <ixlib_token_javascript.hh>




// tools ----------------------------------------------------------------------
#define ADVANCE \
  first++; \
  if (first == last) EXJS_THROW(ECJS_UNEXPECTED_EOF)
#define EXPECT(WHAT,STRINGIFIED) \
  if (first == last) EXJS_THROW(ECJS_UNEXPECTED_EOF) \
  if (first->Type != WHAT) \
    EXJS_THROWINFOTOKEN(ECJS_UNEXPECTED,("'"+first->Text+"' "+_("instead of ")+STRINGIFIED).c_str(),*first)




static char *dummy_i18n_referencer = N_("instead of ");



using namespace ixion;
using namespace javascript;




namespace {
  ref<expression> 
  makeConstantExpression(ref<value> val,code_location const &loc) {
    ref<expression> result(new constant(val,loc));
    return result;
    }
  }




// garbage collectors ---------------------------------------------------------
IXLIB_GARBAGE_DECLARE_MANAGER(value)
IXLIB_GARBAGE_DECLARE_MANAGER(expression)




// exception texts ------------------------------------------------------------
static char *(PlainText[]) ={
  N_("Unterminated comment"),
  N_("Cannot convert"),
  N_("Invalid operation"),
  N_("Unexpected token encountered"),
  N_("Unexpected end of file"),
  N_("Cannot modify rvalue"),
  N_("Unknown identifier"),
  N_("Unknown operator"),
  N_("Invalid non-local exit"),
  N_("Invalid number of arguments"),
  N_("Invalid token encountered"),
  N_("Cannot redeclare identifier"),
  N_("Constructor called on constructed object"),
  N_("No superclass available"),
  N_("Division by zero"),
  };




// no_location_javascript_exception -------------------------------------------
char *no_location_javascript_exception::getText() const {
  return _(PlainText[Error]);
  }




// javascript_exception -------------------------------------------------------
javascript_exception::javascript_exception(TErrorCode error,code_location const &loc,char const *info,char *module = NULL,
  TIndex line = 0)
: base_exception(error, NULL, module, line, "JS") {
  HasInfo = true;
  try {
    string temp = loc.stringify();
    if (info) {
      temp += ": ";
      temp += info;
      }
    strcpy(Info,temp.c_str());
    }
  catch (...) { }
  }



javascript_exception::javascript_exception(no_location_javascript_exception const &half_ex,javascript::code_location const &loc)
: base_exception(half_ex.Error,NULL,half_ex.Module,half_ex.Line,half_ex.Category) {
  HasInfo = true;
  try {
    string temp = loc.stringify() + ": " + half_ex.Info;
    strcpy(Info,temp.c_str());
    }
  catch (...) { }
  }




char *javascript_exception::getText() const {
  return _(PlainText[Error]);
  }




// code_location --------------------------------------------------------------
code_location::code_location(scanner::token &tok) 
  : Line(tok.Line) {
  }




code_location::code_location(TIndex line)
  : Line(line) {
  }




string code_location::stringify() const {
  return "l"+unsigned2dec(Line);
  }




// context --------------------------------------------------------------------
context::context(ref<list_scope,value> scope)
  : DeclarationScope(scope),LookupScope(scope) {
  }




context::context(ref<value> scope)
  : LookupScope(scope) {
  }




context::context(ref<list_scope,value> decl_scope,ref<value> lookup_scope)
  : DeclarationScope(decl_scope),LookupScope(lookup_scope) {
  }




// parser ---------------------------------------------------------------------
namespace {
  ref<expression> 
  parseInstructionList(scanner::token_iterator &first,scanner::token_iterator const &last,bool scoped);
  ref<expression> 
  parseSwitch(scanner::token_iterator &first,scanner::token_iterator const &last,string const &label);
  ref<expression> 
  parseVariableDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last);
  ref<expression> 
  parseConstantDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last);
  ref<expression> 
  parseFunctionDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last);
  ref<expression> 
  parseMethodDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last);
  ref<expression> 
  parseConstructorDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last,string const &class_id);
  ref<expression> 
  parseClassDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last);
  ref<expression> 
  parseInstruction(scanner::token_iterator &first,scanner::token_iterator const &last);
  ref<expression> 
  parseExpression(scanner::token_iterator &first,scanner::token_iterator const &last,int precedence = 0);




  ref<expression> 
  parseInstructionList(scanner::token_iterator &first,scanner::token_iterator const &last,bool scoped) {
    auto_ptr<instruction_list> ilist(
      scoped 
      ? new scoped_instruction_list(*first)
      : new instruction_list(*first));
    
    while (first != last && first->Type != '}') {
      ref<expression> expr = parseInstruction(first,last);
      if (expr.get() != NULL) 
        ilist->add(expr);
      }
    return ilist.release();
    }
  
  
  
  
  ref<expression> 
  parseSwitch(scanner::token_iterator &first,scanner::token_iterator const &last,string const &label) {
    code_location loc(*first);
    
    ADVANCE
    EXPECT('(',_("'(' in switch statement"))
    ADVANCE
    
    ref<expression> discr = parseExpression(first,last);
    EXPECT(')',_("')' in switch statement"))
    ADVANCE
    
    EXPECT('{',_("'{' in switch statement"))
    ADVANCE
    
    auto_ptr<js_switch> sw;
    if (label.size()) {
      auto_ptr<js_switch> tsw(new js_switch(discr,label,loc));
      sw = tsw;
      }
    else {
      auto_ptr<js_switch> tsw(new js_switch(discr,loc));
      sw = tsw;
      }
  
    ref<expression> dvalue;
    auto_ptr<instruction_list> ilist;
    
    while (first != last && first->Type != '}') {
      if (first->Type == TT_JS_CASE) {
        if (ilist.get())
          sw->addCase(dvalue,ilist.release());
     
        auto_ptr<instruction_list> tilist(new instruction_list(*first));
        ilist = tilist;
        
        ADVANCE
  
        dvalue = parseExpression(first,last);
        EXPECT(':',_("':' in case label"))
        ADVANCE
        }
      else if (first->Type == TT_JS_DEFAULT) {
        if (ilist.get())
          sw->addCase(dvalue,ilist.release());
     
        auto_ptr<instruction_list> tilist(new instruction_list(*first));
        ilist = tilist;
        
        ADVANCE
        dvalue = NULL;
        EXPECT(':',_("':' in default label"))
        ADVANCE
        }
      else {
        ref<expression> expr = parseInstruction(first,last);
        if (ilist.get() && expr.get() != NULL) 
          ilist->add(expr);
        }
      }
  
    if (ilist.get())
      sw->addCase(dvalue,ilist.release());
      
    EXPECT('}',_("'}' in switch statement"))
    ADVANCE
    
    return sw.release();
    }
  
  
  
  
  ref<expression> 
  parseVariableDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last) {
    code_location loc(*first);
    
    EXPECT(TT_JS_IDENTIFIER,_("variable identifier"))
    string id = first->Text;
    ADVANCE
    
    ref<expression> def;
    if (first->Type == '=') {
      ADVANCE
      def = parseExpression(first,last);
      }
    ref<expression> result = new variable_declaration(id,def,loc);
  
    if (first->Type == ',') {
      auto_ptr<instruction_list> ilist(new instruction_list(*first));
      ilist->add(result);
      
      while (first->Type == ',') {
        ADVANCE
        
        code_location loc(*first);
        
        EXPECT(TT_JS_IDENTIFIER,_("variable identifier"))
        id = first->Text;
        ADVANCE
        
        if (first->Type == '=') {
          ADVANCE
          def = parseExpression(first,last);
          }
        ref<expression> decl = new variable_declaration(id,def,loc);
        ilist->add(decl);
        }
      result = ilist.release();
      }
  
    return result;
    }
  
  
  
  
  ref<expression> 
  parseConstantDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last) {
    code_location loc(*first);
  
    EXPECT(TT_JS_IDENTIFIER,_("constant identifier"))
    string id = first->Text;
    ADVANCE
    
    ref<expression> def;
    EXPECT('=',_("initializer for constant"))
    ADVANCE
    def = parseExpression(first,last);
  
    ref<expression> result = new constant_declaration(id,def,loc);
  
    if (first->Type == ',') {
      auto_ptr<instruction_list> ilist(new instruction_list(*first));
      ilist->add(result);
      
      while (first->Type == ',') {
        ADVANCE
        
        code_location loc(*first);
        
        EXPECT(TT_JS_IDENTIFIER,_("constant identifier"))
        id = first->Text;
        ADVANCE
        
        EXPECT('=',_("initializer for constant"))
        ADVANCE
        def = parseExpression(first,last);
  
        ref<expression> decl = new constant_declaration(id,def,loc);
        ilist->add(decl);
        }
      result = ilist.release();
      }
  
    return result;
    }
  
  
  
  
  ref<expression> 
  parseFunctionDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last) {
    code_location loc(*first);
    
    EXPECT(TT_JS_IDENTIFIER,_("function identifier"))
    string id = first->Text;
    ADVANCE
    
    EXPECT('(',_("'(' in function declaration"))
    ADVANCE
    
    function::parameter_name_list pnames;
    
    while (first->Type != ')') {
      EXPECT(TT_JS_IDENTIFIER,_("parameter identifier"))
      pnames.push_back(first->Text);
      ADVANCE
      
      if (first->Type == ',') {
        ADVANCE 
        }
      }
    EXPECT(')',_("')' in function declaration"))
    ADVANCE
  
    EXPECT('{',_("'{' in function definition"))
    ADVANCE
    
    ref<expression> body = parseInstructionList(first,last,false);
  
    EXPECT('}',"'}' in method definition")
    first++;
    
    ref<expression> result = new function_declaration(id,pnames,body,loc);
    return result;
    }
  
  
  
  
  ref<expression> 
  parseMethodDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last) {
    code_location loc(*first);
    
    EXPECT(TT_JS_IDENTIFIER,_("method identifier"))
    string id = first->Text;
    ADVANCE
    
    EXPECT('(',_("'(' in method declaration"))
    ADVANCE
    
    method::parameter_name_list pnames;
    
    while (first->Type != ')') {
      EXPECT(TT_JS_IDENTIFIER,_("parameter identifier"))
      pnames.push_back(first->Text);
      ADVANCE
      
      if (first->Type == ',') {
        ADVANCE 
        }
      }
    EXPECT(')',_("')' in method declaration"))
    ADVANCE
  
    EXPECT('{',_("'{' in method definition"))
    ADVANCE
    
    ref<expression> body = parseInstructionList(first,last,false);
  
    EXPECT('}',"'}' in method definition")
    first++;
    
    ref<expression> result = new method_declaration(id,pnames,body,loc);
    return result;
    }
  
  
  
  
  ref<expression> 
  parseConstructorDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last,string const &class_id) {
    code_location loc(*first);
    
    EXPECT(TT_JS_IDENTIFIER,_("constructor identifier"))
    if (first->Text != class_id) {
      EXJS_THROWINFOTOKEN(ECJS_UNEXPECTED,("'"+first->Text+"' "+_("instead of ")+_("class identifier")).c_str(),*first)
      }
    ADVANCE
    
    EXPECT('(',_("'(' in constructor declaration"))
    ADVANCE
    
    method::parameter_name_list pnames;
    
    while (first->Type != ')') {
      EXPECT(TT_JS_IDENTIFIER,_("parameter identifier"))
      pnames.push_back(first->Text);
      ADVANCE
      
      if (first->Type == ',') {
        ADVANCE 
        }
      }
    EXPECT(')',_("')' in constructor declaration"))
    ADVANCE
  
    EXPECT('{',_("'{' in constructor definition"))
    ADVANCE
    
    ref<expression> body = parseInstructionList(first,last,false);
  
    EXPECT('}',"'}' in constructor definition")
    first++;
    
    ref<expression> result = new constructor_declaration(pnames,body,loc);
    return result;
    }
  
  
  
  
  ref<expression> 
  parseClassDeclaration(scanner::token_iterator &first,scanner::token_iterator const &last) {
    code_location loc(*first);
  
    EXPECT(TT_JS_IDENTIFIER,_("class identifier"))
    string id = first->Text;
    ADVANCE
    
    ref<expression> superclass;
    if (first->Type == TT_JS_EXTENDS) {
      ADVANCE
      superclass = parseExpression(first,last);
      }
    
    EXPECT('{',_("'{' in class declaration"))
    ADVANCE
    
    auto_ptr<js_class_declaration> decl(new js_class_declaration(id,superclass,loc));
    
    while (first != last && first->Type != '}') {
      if (first->Type == TT_JS_STATIC) {
        ADVANCE
        
        if (first->Type == TT_JS_FUNCTION) {
          ADVANCE
          decl->addStaticMethod(parseFunctionDeclaration(first,last));
          }
        else {
          if (first->Type == TT_JS_CONST) {
            ADVANCE
            decl->addStaticVariable(parseConstantDeclaration(first,last));
            }
          else {
            ADVANCE
            decl->addStaticVariable(parseVariableDeclaration(first,last));
            }
          EXPECT(';',"';'")
          first++;
          }
        }
      else {
        if (first->Type == TT_JS_FUNCTION) {
          ADVANCE
          decl->addMethod(parseMethodDeclaration(first,last));
          }
        else if (first->Type == TT_JS_CONSTRUCTOR) {
          ADVANCE
          
          EXPECT(TT_JS_FUNCTION,_("'function' keyword"))
          ADVANCE
          
          decl->setConstructor(parseConstructorDeclaration(first,last,id));
          }
        else {
          if (first->Type == TT_JS_CONST) {
            ADVANCE
            decl->addVariable(parseConstantDeclaration(first,last));
            }
          else {
            ADVANCE
            decl->addVariable(parseVariableDeclaration(first,last));
            }
          EXPECT(';',"';'")
          first++;
          }
        }
      }
  
    EXPECT('}',"'}' in class declaration")
    first++;
  
    return decl.release();
    }
  
  
  
  
  ref<expression> 
  parseInstruction(scanner::token_iterator &first,scanner::token_iterator const &last) {
    if (first == last) EXJS_THROW(ECJS_UNEXPECTED_EOF)
    
    string label;
    if (first+1 != last && first[1].Type == ':') {
      label = first->Text;
      ADVANCE
      ADVANCE
      }
    
    ref<expression> result;
    
    if (first->Type == '{') {
      ADVANCE
      result = parseInstructionList(first,last,true);
      EXPECT('}',"'}'")
      first++;
      }
    else if (first->Type == TT_JS_VAR) {
      ADVANCE
      
      result = parseVariableDeclaration(first,last);
  
      EXPECT(';',"';'")
      first++;
      }
    else if (first->Type == TT_JS_CONST) {
      ADVANCE
      
      result = parseConstantDeclaration(first,last);
  
      EXPECT(';',"';'")
      first++;
      }
    else if (first->Type == TT_JS_FUNCTION) {
      ADVANCE
      result = parseFunctionDeclaration(first,last);
      }
    else if (first->Type == TT_JS_CLASS) {
      ADVANCE
      result = parseClassDeclaration(first,last);
      }
    else if (first->Type == TT_JS_IF) {
      code_location loc(*first);
      ADVANCE
      
      EXPECT('(',_("'(' in if statement"))
      ADVANCE
      
      ref<expression> cond = parseExpression(first,last);
      EXPECT(')',_("')' in if statement"))
      first++;
      ref<expression> if_expr = parseInstruction(first,last);
      ref<expression> else_expr;
      
      if (first != last && first->Type == TT_JS_ELSE) {
        ADVANCE
        else_expr = parseInstruction(first,last);
        }
      result = new js_if(cond,if_expr,else_expr,loc);
      }
    else if (first->Type == TT_JS_SWITCH) {
      result = parseSwitch(first,last,label);
      }
    else if (first->Type == TT_JS_WHILE) {
      code_location loc(*first);
      
      ADVANCE
      EXPECT('(',_("'(' in while statement"))
      ADVANCE
      
      ref<expression> cond = parseExpression(first,last);
      EXPECT(')',_("')' in while statement"))
      first++;
  
      ref<expression> loop_expr = parseInstruction(first,last);
  
      if (label.size()) {
        result = new js_while(cond,loop_expr,label,loc);
        label = "";
        }
      else
        result = new js_while(cond,loop_expr,loc);
      }
    else if (first->Type == TT_JS_DO) {
      code_location loc(*first);
      
      ADVANCE
      ref<expression> loop_expr = parseInstruction(first,last);
  
      EXPECT(TT_JS_WHILE,_("'while' in do-while"))
      ADVANCE
      
      EXPECT('(',_("'(' in do-while statement"))
      ADVANCE
      
      ref<expression> cond = parseExpression(first,last);
      EXPECT(')',_("')' in do-while statement"))
      first++;
      
      if (label.size()) {
        result = new js_do_while(cond,loop_expr,label,loc);
        label = "";
        }
      else
        result = new js_do_while(cond,loop_expr,loc);
      }
    else if (first->Type == TT_JS_FOR) {
      code_location loc(*first);
      
      ADVANCE
      
      EXPECT('(',_("'(' in for statement"))
      ADVANCE
      
      ref<expression> init_expr;
      if (first->Type == TT_JS_VAR) {
        ADVANCE
        
        init_expr = parseVariableDeclaration(first,last);
        }
      else
        init_expr = parseExpression(first,last);
  
      if (first->Type == TT_JS_IN) {
        // for (iterator in list)
        ADVANCE
        ref<expression> array_expr = parseExpression(first,last);
  
        EXPECT(')',_("')' in for statement"))
        first++;
        
        ref<expression> loop_expr = parseInstruction(first,last);
        
        if (label.size()) {
          result = new js_for_in(init_expr,array_expr,loop_expr,label,loc);
          label = "";
          }
        else
          result = new js_for_in(init_expr,array_expr,loop_expr,label,loc);
        }
      else {
        // for (;;) ...
        EXPECT(';',_("';' in for statement"))
        ADVANCE
        
        ref<expression> cond_expr = parseExpression(first,last);
    
        EXPECT(';',_("';' in for statement"))
        ADVANCE
        
        ref<expression> update_expr = parseExpression(first,last);
        
        EXPECT(')',_("')' in for statement"))
        first++;
    
        ref<expression> loop_expr = parseInstruction(first,last);
        
        if (label.size()) {
          result = new js_for(init_expr,cond_expr,update_expr,loop_expr,label,loc);
          label = "";
          }
        else
          result = new js_for(init_expr,cond_expr,update_expr,loop_expr,loc);
        }
      }
    else if (first->Type == TT_JS_RETURN) {
      code_location loc(*first);
      ADVANCE
      
      if (first->Type != ';')
        result = new js_return(parseExpression(first,last),loc);
      else
        result = new js_return(makeConstantExpression(makeNull(),loc),loc);
      
      EXPECT(';',"';'")
      first++;
      }
    else if (first->Type == TT_JS_BREAK) {
      code_location loc(*first);
      ADVANCE
      
      if (first->Type != ';') {
        EXPECT(TT_JS_IDENTIFIER,_("break label"))
        result = new js_break(first->Text,loc);
        ADVANCE
        }
      else
        result = new js_break(loc);
      
      EXPECT(';',"';'")
      first++;
      }
    else if (first->Type == TT_JS_CONTINUE) {
      code_location loc(*first);
      ADVANCE
      
      if (first->Type != ';') {
        EXPECT(TT_JS_IDENTIFIER,_("continue label"))
        result = new js_continue(first->Text,loc);
        ADVANCE
        }
      else
        result = new js_continue(loc);
      
      EXPECT(';',"';'")
      first++;
      }
    else if (first->Type == ';') {
      first++;
      result = makeConstantExpression(ref<value>(NULL),*first);
      }
    else {
      // was nothing else, must be expression
      result = parseExpression(first,last);
      EXPECT(';',"';'")
      first++;
      }
    
    if (label.size()) {
      result = new break_label(label,result,*first);
      }
    
    return result;
    }
  
  
  
  
  namespace {
    int const
    PREC_COMMA = 10,              // , (if implemented)
    PREC_THROW = 20,              // throw (if implemented)
    PREC_ASSIGN = 30,             // += and friends [ok]
    PREC_CONDITIONAL = 40,        // ?: [ok]
    PREC_LOG_OR = 50,             // || [ok]
    PREC_LOG_AND = 60,            // && [ok]
    PREC_BIT_OR = 70,             // | [ok]
    PREC_BIT_XOR = 80,            // ^ [ok]
    PREC_BIT_AND = 90,            // & [ok]
    PREC_EQUAL = 100,             // == != === !=== [ok]
    PREC_COMP = 110,              // < <= > >= [ok]
    PREC_SHIFT = 120,             // << >> [ok]
    PREC_ADD = 130,               // + - [ok]
    PREC_MULT = 140,              // * / % [ok]
    PREC_UNARY = 160,             // new + - ++ -- ! ~
    PREC_FUNCALL = 170,           // ()
    PREC_SUBSCRIPT = 180,         // [] .
    PREC_TERM = 200;              // literal identifier 
    }
  
  
  
  
  ref<expression> 
  parseExpression(scanner::token_iterator &first,scanner::token_iterator const &last,int precedence) {
    /*
    precedence:
    the called routine will continue parsing as long as the encountered 
    operators all have precedence greater than or equal to the given parameter.
    */
    
    // an EOF condition cannot legally occur inside 
    // or at the end of an expression.
    
    if (first == last) EXJS_THROW(ECJS_UNEXPECTED_EOF)
    
    ref<expression> expr;
  
    // parse prefix unaries -----------------------------------------------------
    if (precedence <= PREC_UNARY)  {
      if (first->Type == TT_JS_NEW) {
        code_location loc(*first);
        ADVANCE
        
        ref<expression> cls = parseExpression(first,last,PREC_SUBSCRIPT);
        
        EXPECT('(',_("'(' in 'new' expression"))
        ADVANCE
  
        function_call::parameter_expression_list pexps;
        while (first->Type != ')') {
          pexps.push_back(parseExpression(first,last));
          
          if (first->Type == ',') {
            ADVANCE 
            }
          }
          
        EXPECT(')',_("')' in 'new' expression"))
        ADVANCE
        
        expr = new construction(cls,pexps,loc);
        }
      if (first->Type == TT_JS_INCREMENT || first->Type == TT_JS_DECREMENT) {
        code_location loc(*first);
        value::operator_id op = value::token2operator(*first,true,true);
        ADVANCE
        expr = new modifying_unary_operator(op,parseExpression(first,last,PREC_UNARY),loc);
        }
      if (first->Type == '+' ||first->Type == '-'
      || first->Type == '!' || first->Type == '~') {
        code_location loc(*first);
        value::operator_id op = value::token2operator(*first,true,true);
        ADVANCE
        expr = new unary_operator(op,parseExpression(first,last,PREC_UNARY),loc);
        }
      }
  
    // parse parentheses --------------------------------------------------------
    if (first->Type == '(') {
      ADVANCE
      expr = parseExpression(first,last);
      EXPECT(')',"')'")
      ADVANCE
      }
    
    // parse term ---------------------------------------------------------------
    if (expr.get() == NULL && precedence <= PREC_TERM) {
      if (first->Type == TT_JS_LIT_INT) {
        expr = makeConstantExpression(makeConstant(evalUnsigned(first->Text)),*first);
        ADVANCE
        }
      else if (first->Type == TT_JS_LIT_FLOAT) {
        expr = makeConstantExpression(makeConstant(evalFloat(first->Text)),*first);
        ADVANCE
        }
      else if (first->Type == TT_JS_LIT_STRING) {
        expr = makeConstantExpression(makeConstant(parseCEscapes(
          first->Text.substr(1,first->Text.size()-2)
          )),*first);
        ADVANCE
        }
      else if (first->Type == TT_JS_LIT_TRUE) {
        expr = makeConstantExpression(makeConstant(true),*first);
        ADVANCE
        }
      else if (first->Type == TT_JS_LIT_FALSE) {
        expr = makeConstantExpression(makeConstant(false),*first);
        ADVANCE
        }
      else if (first->Type == TT_JS_LIT_UNDEFINED) {
        expr = makeConstantExpression(makeUndefined(),*first);
        ADVANCE
        }
      else if (first->Type == TT_JS_IDENTIFIER) {
        expr = new lookup_operation(first->Text,*first);
        ADVANCE
        }
      else if (first->Type == TT_JS_NULL) {
        expr = makeConstantExpression(makeNull(),*first);
        ADVANCE
        }
      }
    
    if (expr.get() == NULL)
      EXJS_THROWINFOTOKEN(ECJS_UNEXPECTED,("'"+first->Text+"' instead of expression").c_str(),*first)
  
    bool parsed_something;
    do {
      parsed_something = false;
      
      // parse postfix "subscripts" ---------------------------------------------
      if (first->Type == '(' && precedence <= PREC_FUNCALL) {
        code_location loc(*first);
        ADVANCE
        
        function_call::parameter_expression_list pexps;
        while (first->Type != ')') {
          pexps.push_back(parseExpression(first,last));
          
          if (first->Type == ',') {
            ADVANCE 
            }
          }
          
        EXPECT(')',_("')' in function call"))
        ADVANCE
        
        expr = new function_call(expr,pexps,loc);
        parsed_something = true;
        }
      // parse postfix unary ----------------------------------------------------
      else if ((first->Type == TT_JS_INCREMENT || first->Type == TT_JS_DECREMENT) && precedence <= PREC_UNARY) {
        value::operator_id op = value::token2operator(*first,true);
        expr = new modifying_unary_operator(op,expr,*first);
        parsed_something = true;
        ADVANCE
        }
      // parse special binary operators -----------------------------------------
      else if (first->Type == '.' && precedence <= PREC_SUBSCRIPT) {
        ADVANCE
        expr = new lookup_operation(expr,first->Text,*first);
        ADVANCE
        parsed_something = true;
        }
      else if (first->Type == '[' && precedence <= PREC_SUBSCRIPT) {
        code_location loc(*first);
        ADVANCE
        
        ref<expression> index = parseExpression(first,last);
        
        EXPECT(']',_("']' in subscript"))
        ADVANCE
        
        expr = new subscript_operation(expr,index,loc);
  
        parsed_something = true;
        }
      
      // parse regular binary operators -----------------------------------------
      #define BINARY_OP(PRECEDENCE,EXPRESSION,TYPE) \
        else if ((EXPRESSION) && precedence <= PRECEDENCE) { \
          code_location loc(*first); \
          value::operator_id op = value::token2operator(*first); \
          ADVANCE \
          ref<expression> right = parseExpression(first,last,PRECEDENCE); \
          expr = new TYPE##_operator(op,expr,right,loc); \
          parsed_something = true; \
          }
  
      BINARY_OP(PREC_MULT,first->Type == '*' || first->Type == '/' || first->Type == '%',binary)
      BINARY_OP(PREC_ADD,first->Type == '+' || first->Type == '-',binary)
      BINARY_OP(PREC_SHIFT,first->Type == TT_JS_LEFT_SHIFT || first->Type == TT_JS_RIGHT_SHIFT,binary)
      BINARY_OP(PREC_COMP,first->Type == '>' || first->Type == '<'
        || first->Type == TT_JS_LESS_EQUAL || first->Type == TT_JS_GREATER_EQUAL
        || first->Type == TT_JS_IDENTICAL || first->Type == TT_JS_NOT_IDENTICAL,binary)
      BINARY_OP(PREC_EQUAL,first->Type == TT_JS_EQUAL || first->Type == TT_JS_NOT_EQUAL,binary)
      BINARY_OP(PREC_BIT_AND,first->Type == '&',binary)
      BINARY_OP(PREC_BIT_XOR,first->Type == '^',binary)
      BINARY_OP(PREC_BIT_OR,first->Type == '|',binary)
      BINARY_OP(PREC_LOG_AND,first->Type == TT_JS_LOGICAL_AND,binary_shortcut)
      BINARY_OP(PREC_LOG_OR,first->Type == TT_JS_LOGICAL_OR,binary_shortcut)
      else if (first->Type == '?') {
        code_location loc(*first);
        ADVANCE
        ref<expression> op2 = parseExpression(first,last);
        if (first->Type != ':')
          EXJS_THROWINFO(ECJS_UNEXPECTED,(first->Text+" instead of ':'").c_str())
        ADVANCE
        ref<expression> op3 = parseExpression(first,last,PREC_CONDITIONAL);
        expr = new ternary_operator(expr,op2,op3,loc);
        parsed_something = true;
        }
      else if (first->Type == '=' && precedence <= PREC_ASSIGN) {
        code_location loc(*first);
        ADVANCE
        ref<expression> op2 = parseExpression(first,last);
        expr = new assignment(expr,op2,loc);
        parsed_something = true;
        }
      BINARY_OP(PREC_ASSIGN,first->Type == TT_JS_PLUS_ASSIGN
        || first->Type == TT_JS_MINUS_ASSIGN
        || first->Type == TT_JS_MULTIPLY_ASSIGN
        || first->Type == TT_JS_DIVIDE_ASSIGN
        || first->Type == TT_JS_MODULO_ASSIGN
        || first->Type == TT_JS_BIT_XOR_ASSIGN
        || first->Type == TT_JS_BIT_AND_ASSIGN
        || first->Type == TT_JS_BIT_OR_ASSIGN     
        || first->Type == TT_JS_LEFT_SHIFT_ASSIGN
        || first->Type == TT_JS_RIGHT_SHIFT_ASSIGN,modifying_binary)
    } while (parsed_something);
    
    return expr;
    }
  }
  
  
  
  
// interpreter -----------------------------------------------------------------
interpreter::interpreter() {
  RootScope = new list_scope;
  
  ref<value> ac = new js_array_constructor();
  RootScope->addMember("Array",ac);
  }




interpreter::~interpreter() {
  }




ref<expression> interpreter::parse(string const &str) {
  // *** FIXME: this works around a bug in istrstream
  if (str.size() == 0) {
    return ref<expression>(NULL);
    }
  istrstream strm(str.data(),str.size());
  return parse(strm);
  }




ref<expression> interpreter::parse(istream &istr) {
  jsFlexLexer lexer(&istr);
  scanner scanner(lexer);
  
  scanner::token_list tokenlist = scanner.scan();
  scanner::token_iterator text = tokenlist.begin();
  return parseInstructionList(text,tokenlist.end(),false);
  }




ref<value> interpreter::execute(string const &str) {
  return execute(parse(str));
  }




ref<value> interpreter::execute(istream &istr) {
  return execute(parse(istr));
  }




ref<value> interpreter::execute(ref<expression> expr) {
  if (expr.get() == NULL) return ref<value>(NULL);
  return evaluateCatchExits(expr);
  }




ref<value> interpreter::evaluateCatchExits(ref<expression> expr) {
  ref<value> result;
  try {
    context ctx(RootScope);
    result = expr->evaluate(ctx);
    }
  catch (return_exception &re) {
    EXJS_THROWINFOLOCATION(ECJS_INVALID_NON_LOCAL_EXIT,"return",re.Location)
    }
  catch (break_exception &be) {
    if (be.HasLabel)
      EXJS_THROWINFOLOCATION(ECJS_INVALID_NON_LOCAL_EXIT,("break "+be.Label).c_str(),be.Location)
    else
      EXJS_THROWINFOLOCATION(ECJS_INVALID_NON_LOCAL_EXIT,"break",be.Location)
    }
  catch (continue_exception &ce) {
    if (ce.HasLabel)
      EXJS_THROWINFOLOCATION(ECJS_INVALID_NON_LOCAL_EXIT,("continue "+ce.Label).c_str(),ce.Location)
    else
      EXJS_THROWINFOLOCATION(ECJS_INVALID_NON_LOCAL_EXIT,"continue",ce.Location)
    }
  return result;
  }
