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




// variable_declaration -------------------------------------------------------
variable_declaration::variable_declaration(string const &id,ref<expression> def_value,code_location const &loc)
  : expression(loc),Identifier(id),DefaultValue(def_value) {
  }




ref<value> variable_declaration::evaluate(context const &ctx) const {
  try {
    ref<value> def;
    if (DefaultValue.get() != NULL) def = DefaultValue->evaluate(ctx)->eliminateWrappers()->duplicate();
    else def = makeNull();
    
    ref<value> lv = makeLValue(def);
    ctx.DeclarationScope->addMember(Identifier,lv);
    return lv;
    }
  EXJS_ADD_CODE_LOCATION
  }




// constant_declaration -------------------------------------------------------
constant_declaration::constant_declaration(string const &id,ref<expression> def_value,code_location const &loc)
  : expression(loc),Identifier(id),DefaultValue(def_value) {
  }




ref<value> constant_declaration::evaluate(context const &ctx) const {
  try {
    ref<value> def;
    if (DefaultValue.get() != NULL) def = DefaultValue->evaluate(ctx)->eliminateWrappers()->duplicate();
    else def = makeNull();
    
    ref<value> cns = wrapConstant(def);
    ctx.DeclarationScope->addMember(Identifier,cns);
    return cns;
    }
  EXJS_ADD_CODE_LOCATION
  }




// function_declaration -------------------------------------------------------
function_declaration::
function_declaration(string const &id,parameter_name_list const &pnames,
ref<expression> body,code_location const &loc)
  : expression(loc),Identifier(id),ParameterNameList(pnames),Body(body) {
  }




ref<value> function_declaration::evaluate(context const &ctx) const {
  try {
    ref<value> fun = new function(ParameterNameList,Body,ctx.LookupScope);
    ctx.DeclarationScope->addMember(Identifier,fun);
    return ref<value>(NULL);
    }
  EXJS_ADD_CODE_LOCATION
  }




// method_declaration ---------------------------------------------------------
method_declaration::
method_declaration(string const &id,parameter_name_list const &pnames,
ref<expression> body,code_location const &loc)
  : expression(loc),Identifier(id),ParameterNameList(pnames),Body(body) {
  }




ref<value> method_declaration::evaluate(context const &ctx) const {
  try {
    ref<value> fun = new method(ParameterNameList,Body,ctx.LookupScope);
    ctx.DeclarationScope->addMember(Identifier,fun);
    return ref<value>(NULL);
    }
  EXJS_ADD_CODE_LOCATION
  }




// constructor_declaration ---------------------------------------------------------
constructor_declaration::
constructor_declaration(parameter_name_list const &pnames,
ref<expression> body,code_location const &loc)
  : expression(loc),ParameterNameList(pnames),Body(body) {
  }




ref<value> constructor_declaration::evaluate(context const &ctx) const {
  try {
    ref<value> fun = new constructor(ParameterNameList,Body,ctx.LookupScope);
    return fun;
    }
  EXJS_ADD_CODE_LOCATION
  }




// js_class_declaration -------------------------------------------------------
js_class_declaration::js_class_declaration(string const &id,ref<expression> superclass,code_location const &loc) 
  : expression(loc),Identifier(id),SuperClass(superclass) {
  }




ref<value> js_class_declaration::evaluate(context const &ctx) const {
  try {
    ref<list_scope,value> sml(new list_scope);
    ref<list_scope,value> ml(new list_scope);
    ref<list_scope,value> svl(new list_scope);
  
    ref<value> sc;
    if (SuperClass.get())
      sc = SuperClass->evaluate(ctx);
    
    ref<value> constructor;
    if (ConstructorDeclaration.get())
      constructor = ConstructorDeclaration->evaluate(ctx);
    ref<value> cls(new js_class(ctx.LookupScope,sc,constructor,sml,ml,svl,VariableList));
  
    ref<list_scope,value> static_scope(new list_scope);
    static_scope->unite(ctx.LookupScope);
    static_scope->unite(cls);
    
    FOREACH_CONST(first,StaticMethodList,declaration_list)
      (*first)->evaluate(context(sml,static_scope));
    FOREACH_CONST(first,MethodList,declaration_list)
      (*first)->evaluate(context(ml,ctx.LookupScope));
    FOREACH_CONST(first,StaticVariableList,declaration_list)
      (*first)->evaluate(context(svl,static_scope));
  
    ctx.DeclarationScope->addMember(Identifier,cls);
    return cls;
    }
  EXJS_ADD_CODE_LOCATION
  }




void js_class_declaration::setConstructor(ref<expression> decl) {
  ConstructorDeclaration = decl;
  }




void js_class_declaration::addStaticMethod(ref<expression> decl) {
  StaticMethodList.push_back(decl);
  }




void js_class_declaration::addMethod(ref<expression> decl) {
  MethodList.push_back(decl);
  }




void js_class_declaration::addStaticVariable(ref<expression> decl) {
  StaticVariableList.push_back(decl);
  }




void js_class_declaration::addVariable(ref<expression> decl) {
  VariableList.push_back(decl);
  }
