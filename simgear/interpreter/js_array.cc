// ----------------------------------------------------------------------------
//  Description      : Javascript interpreter
// ----------------------------------------------------------------------------
//  (c) Copyright 2000 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#include <ixlib_js_internals.hh>




using namespace ixion;
using namespace javascript;




// js_array -------------------------------------------------------------------
js_array::
js_array(TSize size) {
  Array.resize(size);

  ref<value> null = javascript::makeNull();
  for (TIndex i = 0;i < size;i++)
    Array[i] = makeLValue(null);
  }




string js_array::stringify() const {
  value_array::const_iterator first = Array.begin(),last = Array.end();
  
  string result = "{ ";
  bool at_first = true;
  while (first != last) {
    if (!at_first) result += ',';
    else at_first = false;
    result += (*first)->stringify();
    first++;
    }
  return result + " }";
  }




ref<javascript::value> 
js_array::
duplicate() { 
  ref<value> result = new js_array(*this);
  return result;
  }




ref<javascript::value> 
js_array::
lookup(string const &identifier) {
  if (identifier == "length") return javascript::makeConstant(Array.size());
  return super::lookup(identifier);
  }




ref<javascript::value> 
js_array::
subscript(value const &index) {
  TIndex idx = index.toInt();
  return operator[](idx);
  }




ref<javascript::value> 
js_array::
callMethod(string const &id,parameter_list const &parameters) {
  if (id == "pop" && parameters.size() == 0) {
    if (Array.size() == 0) return javascript::makeNull();
    else {
      ref<value> back = Array.back();
      Array.pop_back();
      return back;
      }
    }
  else if (id == "push") {
    FOREACH_CONST(first,parameters,parameter_list) {
      Array.push_back((*first)->duplicate());
      }
    return javascript::makeConstant(Array.size());
    }
  else if (id == "reverse" && parameters.size() == 0) {
    reverse(Array.begin(),Array.end());
    return this;
    }
  else if (id == "shift" && parameters.size() == 0) {
    if (Array.size() == 0) return javascript::makeNull();
    else {
      ref<value> front = Array.front();
      Array.erase(Array.begin());
      return front;
      }
    }
  else if (id == "slice" && parameters.size() == 2) {
    value_array::const_iterator first = Array.begin() + parameters[0]->toInt();
    value_array::const_iterator last = Array.begin() + parameters[1]->toInt();
    
    auto_ptr<js_array> array(new js_array(first,last));
    return array.release();
    }
  else if (id == "unshift") {
    TIndex i = 0;
    FOREACH_CONST(first,parameters,parameter_list) {
      Array.insert(Array.begin() + i++,(*first)->duplicate());
      }
    return javascript::makeConstant(Array.size());
    }
  else if (id == "join" && parameters.size() == 1) {
    string sep = parameters[0]->toString();
    string result;

    for( TIndex i = 0; i < Array.size(); ++i ) {
      if (i != 0)
	result += sep;

      result += Array[i]->toString();
      }

    return javascript::makeValue(result);
    }
  // *** FIXME: implement splice and sort
  
  EXJS_THROWINFO(ECJS_UNKNOWN_IDENTIFIER,("Array."+id).c_str())
  }




void js_array::resize(TSize size) {
  if (size >= Array.size()) {
    TSize prevsize = Array.size();
    
    Array.resize(size);
    
    ref<value> null = javascript::makeNull();
    for (TIndex i = prevsize;i < size;i++)
      Array[i] = makeLValue(null);
    }
  }




ref<value> &js_array::operator[](TIndex idx) {
  if (idx >= Array.size())
    resize((Array.size()+1)*2);
    
  return Array[idx];
  }





void js_array::push_back(ref<value> val) {
  Array.push_back(val);
  }




// js_array_constructor -------------------------------------------------------
ref<javascript::value> js_array_constructor::duplicate() {
  // array_constructor is not mutable
  return this;
  }




ref<javascript::value> 
js_array_constructor::
construct(parameter_list const &parameters) {
  if (parameters.size() == 0) return makeArray();
  else if (parameters.size() == 1) return makeArray(parameters[0]->toInt());
  else /* parameters.size() >= 2 */ {
    auto_ptr<js_array> result(new js_array(parameters.size()));
    
    TIndex i = 0;
    FOREACH_CONST(first,parameters,parameter_list) {
      (*result)[i++] = (*first)->duplicate();
      }
    return result.release();
    }
  }
