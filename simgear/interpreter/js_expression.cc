// ----------------------------------------------------------------------------
//  Description      : Javascript interpreter
// ----------------------------------------------------------------------------
//  (c) Copyright 2000 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#include <ixlib_js_internals.hh>
#include <ixlib_token_javascript.hh>




#define EXJS_ADD_CODE_LOCATION \
  catch (no_location_javascript_exception &half) { \
    throw javascript_exception(half,getCodeLocation()); \
    }




using namespace ixion;
using namespace javascript;




// expression -----------------------------------------------------------------
expression::expression(code_location const &loc)
  : Location(loc) {
  }




expression::~expression() {
  }




// constant -------------------------------------------------------------------
constant::constant(ref<value> val,code_location const &loc) 
  : expression(loc),Value(val) {
  }




ref<value> 
constant::
evaluate(context const &ctx) const {
  return Value;
  }




// unary_operator -------------------------------------------------
unary_operator::unary_operator(value::operator_id opt,ref<expression> opn,code_location const &loc)
  : expression(loc),Operator(opt),Operand(opn) {
  }




ref<value> 
unary_operator::
evaluate(context const &ctx) const {
  try {
    return Operand->evaluate(ctx)->operatorUnary(Operator);
    }
  EXJS_ADD_CODE_LOCATION
  }




// modifying_unary_operator ---------------------------------------------------
modifying_unary_operator::
modifying_unary_operator(value::operator_id opt,ref<expression> opn,code_location const &loc)
  : expression(loc),Operator(opt),Operand(opn) {
  }




ref<value> 
modifying_unary_operator::
evaluate(context const &ctx) const {
  try {
    return Operand->evaluate(ctx)->operatorUnaryModifying(Operator);
    }
  EXJS_ADD_CODE_LOCATION    
  }




// binary_operator ------------------------------------------------------------
binary_operator::binary_operator(value::operator_id opt,ref<expression> opn1,ref<expression> opn2,code_location const &loc)
  : expression(loc),Operator(opt),Operand1(opn1),Operand2(opn2) {
  }



  
ref<value> binary_operator::evaluate(context const &ctx) const {
  try {
    return Operand1->evaluate(ctx)->operatorBinary(Operator,Operand2->evaluate(ctx));
    }
  EXJS_ADD_CODE_LOCATION
  }




// binary_shortcut_operator ---------------------------------------------------
binary_shortcut_operator::binary_shortcut_operator(value::operator_id opt,ref<expression> opn1,ref<expression> opn2,code_location const &loc)
  : expression(loc),Operator(opt),Operand1(opn1),Operand2(opn2) {
  }



  
ref<value> binary_shortcut_operator::evaluate(context const &ctx) const {
  try {
    return Operand1->evaluate(ctx)->operatorBinaryShortcut(Operator,*Operand2,ctx);
    }
  EXJS_ADD_CODE_LOCATION
  }




// modifying_binary_operator --------------------------------------
modifying_binary_operator::
modifying_binary_operator(value::operator_id opt,ref<expression> opn1,ref<expression> opn2,code_location const &loc)
  : expression(loc),Operator(opt),Operand1(opn1),Operand2(opn2) {
  }



  
ref<value> 
modifying_binary_operator::
evaluate(context const &ctx) const {
  try {
    return Operand1->evaluate(ctx)->operatorBinaryModifying(Operator,Operand2->evaluate(ctx));
    }
  EXJS_ADD_CODE_LOCATION
  }




// ternary_operator -----------------------------------------------------------
ternary_operator::
ternary_operator(ref<expression> opn1,ref<expression> opn2,ref<expression> opn3,code_location const &loc)
  : expression(loc),Operand1(opn1),Operand2(opn2),Operand3(opn3) {
  }




ref<value> 
ternary_operator::
evaluate(context const &ctx) const {
  try {
    if (Operand1->evaluate(ctx)->toBoolean())
      return Operand2->evaluate(ctx);
    else
      return Operand3->evaluate(ctx);
    }
  EXJS_ADD_CODE_LOCATION
  }




// subscript_operation --------------------------------------------------------
subscript_operation::subscript_operation(ref<expression> opn1,ref<expression> opn2,code_location const &loc)
  : expression(loc),Operand1(opn1),Operand2(opn2) {
  }



  
ref<value> subscript_operation::evaluate(context const &ctx) const {
  try {
    ref<value> op2 = Operand2->evaluate(ctx);
    return Operand1->evaluate(ctx)->subscript(*op2);
    }
  EXJS_ADD_CODE_LOCATION
  }




// lookup_operation -----------------------------------------------------------
lookup_operation::lookup_operation(string const &id,code_location const &loc)
  : expression(loc),Identifier(id) {
  }




lookup_operation::lookup_operation(ref<expression> opn,string const &id,code_location const &loc)
  : expression(loc),Operand(opn),Identifier(id) {
  }




ref<value> lookup_operation::evaluate(context const &ctx) const {
  try {
    ref<value> scope(ctx.LookupScope);
    if (Operand.get() != NULL)
      scope = Operand->evaluate(ctx);
    return scope->lookup(Identifier);
    }
  EXJS_ADD_CODE_LOCATION
  }




// assignment -----------------------------------------------------------------
assignment::
assignment(ref<expression> opn1,ref<expression> opn2,code_location const &loc)
  : expression(loc),Operand1(opn1),Operand2(opn2) {
  }



  
ref<value> 
assignment::evaluate(context const &ctx) const {
  try {
    return Operand1->evaluate(ctx)->assign(Operand2->evaluate(ctx)->eliminateWrappers()->duplicate());
    }
  EXJS_ADD_CODE_LOCATION
  }




// basic_call -----------------------------------------------------------------
basic_call::basic_call(parameter_expression_list const &pexps,code_location const &loc) 
  : expression(loc),ParameterExpressionList(pexps) {
  }




void basic_call::makeParameterValueList(context const &ctx,parameter_value_list &pvalues) const {
  FOREACH_CONST(first,ParameterExpressionList,parameter_expression_list) {
    pvalues.push_back((*first)->evaluate(ctx));
    }
  }




// function_call --------------------------------------------------------------
function_call::function_call(ref<expression> fun,parameter_expression_list const &pexps,code_location const &loc)
  : super(pexps,loc),Function(fun) {
  }




ref<value> function_call::evaluate(context const &ctx) const {
  try {
    ref<value> func_value = Function->evaluate(ctx);
    
    value::parameter_list pvalues;
    makeParameterValueList(ctx,pvalues);
    ref<value> result = func_value->call(pvalues);
    
    if (result.get() == NULL) return makeNull();
    else return result;
    }
  EXJS_ADD_CODE_LOCATION
  }




// construction ---------------------------------------------------------------
construction::construction(ref<expression> cls,parameter_expression_list const &pexps,code_location const &loc)
  : super(pexps,loc),Class(cls) {
  }




ref<value> construction::evaluate(context const &ctx) const {
  try {
    ref<value> class_value = Class->evaluate(ctx);
    
    value::parameter_list pvalues;
    makeParameterValueList(ctx,pvalues);
    
    return class_value->construct(pvalues);
    }
  EXJS_ADD_CODE_LOCATION
  }
