// ----------------------------------------------------------------------------
//  Description      : Javascript interpreter
// ----------------------------------------------------------------------------
//  (c) Copyright 2000 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#include <ixlib_js_internals.hh>
#include <ixlib_token_javascript.hh>




using namespace ixion;
using namespace javascript;




// instruction_list -----------------------------------------------------------
ref<value> 
instruction_list::evaluate(context const &ctx) const {
  ref<value> result;
  FOREACH_CONST(first,ExpressionList,expression_list)
    result = (*first)->evaluate(ctx);
  return result;
  }




void instruction_list::add(ref<expression> expr) {
  ExpressionList.push_back(expr);
  }




// scoped_instruction_list ----------------------------------------
ref<value> scoped_instruction_list::evaluate(context const &ctx) const {
  ref<list_scope,value> scope = new list_scope;
  scope->unite(ctx.LookupScope);

  ref<value> result = instruction_list::evaluate(context(scope));
  if (result.get()) return result->duplicate();
  return ref<value>(NULL);

  // ATTENTION: this is a scope cancellation point.
  }




// js_if ----------------------------------------------------------------------
js_if::js_if(ref<expression> cond,ref<expression> ifex,ref<expression> ifnotex,code_location const &loc)
  : expression(loc),Conditional(cond),IfExpression(ifex),IfNotExpression(ifnotex) {
  }




ref<value> js_if::evaluate(context const &ctx) const {
  if (Conditional->evaluate(ctx)->toBoolean())
    return IfExpression->evaluate(ctx);
  else 
    if (IfNotExpression.get()) 
      return IfNotExpression->evaluate(ctx);
    else 
      return ref<value>(NULL);
  }




// js_while -------------------------------------------------------------------
js_while::js_while(ref<expression> cond,ref<expression> loopex,code_location const &loc)
  : expression(loc),Conditional(cond),LoopExpression(loopex),HasLabel(false) {
  }




js_while::js_while(ref<expression> cond,ref<expression> loopex,string const &label,code_location const &loc)
  : expression(loc),Conditional(cond),LoopExpression(loopex),HasLabel(true),Label(label) {
  }




ref<value> js_while::evaluate(context const &ctx) const {
  ref<value> result;
  while (Conditional->evaluate(ctx)->toBoolean()) {
    try {
      result = LoopExpression->evaluate(ctx);
      }
    catch (break_exception &be) {
      if (!be.HasLabel || (HasLabel && be.HasLabel && be.Label == Label))
        break;
      else throw;
      }
    catch (continue_exception &ce) {
      if (!ce.HasLabel || (HasLabel && ce.HasLabel && ce.Label == Label))
        continue;
      else throw;
      }
    }
  return result;
  }




// js_do_while ----------------------------------------------------------------
js_do_while::js_do_while(ref<expression> cond,ref<expression> loopex,code_location const &loc)
  : expression(loc),Conditional(cond),LoopExpression(loopex),HasLabel(false) {
  }




js_do_while::js_do_while(ref<expression> cond,ref<expression> loopex,string const &label,code_location const &loc)
  : expression(loc),Conditional(cond),LoopExpression(loopex),HasLabel(true),Label(label) {
  }




ref<value> js_do_while::evaluate(context const &ctx) const {
  ref<value> result;
  do {
    try {
      result = LoopExpression->evaluate(ctx);
      }
    catch (break_exception &be) {
      if (!be.HasLabel || (HasLabel && be.HasLabel && be.Label == Label))
        break;
      else throw;
      }
    catch (continue_exception &ce) {
      if (!ce.HasLabel || (HasLabel && ce.HasLabel && ce.Label == Label))
        continue;
      else throw;
      }
  } while (Conditional->evaluate(ctx)->toBoolean());
  return result;
  }




// js_for ---------------------------------------------------------------------
js_for::js_for(ref<expression> init,ref<expression> cond,ref<expression> update,ref<expression> loop,code_location const &loc)
  : expression(loc),Initialization(init),Conditional(cond),Update(update),
  LoopExpression(loop),HasLabel(false) {
  }




js_for::js_for(ref<expression> init,ref<expression> cond,ref<expression> update,ref<expression> loop,string const &label,code_location const &loc)
  : expression(loc),Initialization(init),Conditional(cond),Update(update),LoopExpression(loop),
  HasLabel(true),Label(label) {
  }




ref<value> js_for::evaluate(context const &ctx) const {
  ref<list_scope,value> scope = new list_scope;
  scope->unite(ctx.LookupScope);
  context inner_context(scope);

  ref<value> result;
  for (Initialization->evaluate(inner_context);Conditional->evaluate(inner_context)->toBoolean();
  Update->evaluate(inner_context)) {
    try {
      result = LoopExpression->evaluate(inner_context);
      }
    catch (break_exception &be) {
      if (!be.HasLabel || (HasLabel && be.HasLabel && be.Label == Label))
        break;
      else throw;
      }
    catch (continue_exception &ce) {
      if (!ce.HasLabel || (HasLabel && ce.HasLabel && ce.Label == Label))
        continue;
      else throw;
      }
    }
  return result;
  }




// js_for_in ------------------------------------------------------------------
js_for_in::js_for_in(ref<expression> iter,ref<expression> array,ref<expression> loop,code_location const &loc)
  : expression(loc),Iterator(iter),Array(array),LoopExpression(loop),HasLabel(false) {
  }




js_for_in::js_for_in(ref<expression> iter,ref<expression> array,ref<expression> loop,string const &label,code_location const &loc)
  : expression(loc),Iterator(iter),Array(array),LoopExpression(loop),
  HasLabel(true),Label(label) {
  }




ref<value> js_for_in::evaluate(context const &ctx) const {
  ref<list_scope,value> scope = new list_scope;
  scope->unite(ctx.LookupScope);
  context inner_context(scope);

  ref<value> result;
  ref<value> iterator = Iterator->evaluate(inner_context);
  ref<value> array = Array->evaluate(inner_context);
  
  TSize size = array->lookup("length")->toInt();
  
  for (TIndex i = 0;i < size;i++) {
    try {
      iterator->assign(array->subscript(*makeConstant(i)));
      result = LoopExpression->evaluate(inner_context);
      }
    catch (break_exception &be) {
      if (!be.HasLabel || (HasLabel && be.HasLabel && be.Label == Label))
        break;
      else throw;
      }
    catch (continue_exception &ce) {
      if (!ce.HasLabel || (HasLabel && ce.HasLabel && ce.Label == Label))
        continue;
      else throw;
      }
    }
  if (result.get()) return result->duplicate();
  return ref<value>(NULL);

  // ATTENTION: this is a scope cancellation point.
  }




// js_return ------------------------------------------------------------------
js_return::js_return(ref<expression> retval,code_location const &loc)
  : expression(loc),ReturnValue(retval) {
  }




ref<value> js_return::evaluate(context const &ctx) const {
  ref<value> retval;
  if (ReturnValue.get())
    retval = ReturnValue->evaluate(ctx);

  throw return_exception(retval,getCodeLocation());
  }




// js_break -------------------------------------------------------------------
js_break::js_break(code_location const &loc)
  : expression(loc),HasLabel(false) {
  }




js_break::js_break(string const &label,code_location const &loc)
  : expression(loc),HasLabel(true),Label(label) {
  }




ref<value> js_break::evaluate(context const &ctx) const {
  throw break_exception(HasLabel,Label,getCodeLocation());
  }




// js_continue ----------------------------------------------------------------
js_continue::js_continue(code_location const &loc)
  : expression(loc),HasLabel(false) {
  }




js_continue::js_continue(string const &label,code_location const &loc)
  : expression(loc),HasLabel(true),Label(label) {
  }




ref<value> js_continue::evaluate(context const &ctx) const {
  throw continue_exception(HasLabel,Label,getCodeLocation());
  }




// break_label ----------------------------------------------------------------
break_label::break_label(string const &label,ref<expression> expr,code_location const &loc)
  : expression(loc),Label(label),Expression(expr) {
  }




ref<value> 
break_label::evaluate(context const &ctx) const {
  try {
    return Expression->evaluate(ctx);
    }
  catch (break_exception &be) {
    if (be.HasLabel && be.Label == Label) return ref<value>(NULL);
    else throw;
    }
  }




// js_switch  -----------------------------------------------------------------
js_switch::js_switch(ref<expression> discriminant,code_location const &loc)
  : expression(loc),HasLabel(false),Discriminant(discriminant) {
  }




js_switch::js_switch(ref<expression> discriminant,string const &label,code_location const &loc)
  : expression(loc),HasLabel(true),Label(label),Discriminant(discriminant) {
  }




ref<value> 
js_switch::
evaluate(context const &ctx) const {
  ref<list_scope,value> scope = new list_scope;
  scope->unite(ctx.LookupScope);
  context inner_context(scope);
  
  ref<value> discr = Discriminant->evaluate(inner_context);
  
  case_list::const_iterator expr,def;
  bool expr_found = false,def_found = false;
  FOREACH_CONST(first,CaseList,case_list) {
    if (first->DiscriminantValue.get()) {
      if (first->DiscriminantValue->evaluate(inner_context)->
      operatorBinary(value::OP_EQUAL,Discriminant->evaluate(inner_context))->toBoolean()) {
        expr = first;
	expr_found = true;
	break;
	}
      }
    else {
      if (!def_found) {
        def = first;
        def_found = true;
	}
      }
    }
  
  try {
    case_list::const_iterator exec,last = CaseList.end();
    if (expr_found)
      exec = expr; 
    else if (def_found) 
      exec = def;
    else
      return ref<value>(NULL);

    ref<value> result;
    while (exec != last) {
      result = exec->Expression->evaluate(inner_context);
      exec++;
      }
    if (result.get()) return result->duplicate();
    return ref<value>(NULL);
    }
  catch (break_exception &be) {
    if (!be.HasLabel || (HasLabel && be.HasLabel && be.Label == Label)) 
      return ref<value>(NULL);
    else 
      throw;
    }
  
  // ATTENTION: this is a scope cancellation point.
  }




void js_switch::addCase(ref<expression> dvalue,ref<expression> expr) {
  case_label cl;
  cl.DiscriminantValue = dvalue;
  cl.Expression = expr;
  CaseList.push_back(cl);
  }
