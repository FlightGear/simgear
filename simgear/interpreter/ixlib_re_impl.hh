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




// regex::backref_stack -------------------------------------------------------
template<class T>
void ixion::regex<T>::backref_stack::open(TIndex index) {
  backref_entry entry = { backref_entry::OPEN,index };
  Stack.push_back(entry);
  }




template<class T>
void ixion::regex<T>::backref_stack::close(TIndex index) {
  backref_entry entry = { backref_entry::CLOSE,index };
  Stack.push_back(entry);
  }




template<class T>
ixion::regex<T>::backref_stack::rewind_info 
ixion::regex<T>::backref_stack::getRewindInfo() const {
  return Stack.size();
  }




template<class T>
void ixion::regex<T>::backref_stack::rewind(rewind_info ri) {
  Stack.erase(Stack.begin()+ri,Stack.end());
  }




template<class T>
void ixion::regex<T>::backref_stack::clear() {
  Stack.clear();
  }




template<class T>
ixion::TSize ixion::regex<T>::backref_stack::size() {
  TSize result = 0;
  FOREACH_CONST(first,Stack,internal_stack)
    if (first->Type == backref_entry::OPEN) result++;
  return result;
  }




template<class T>
T ixion::regex<T>::backref_stack::get(TIndex number,T const &candidate) const {
  TIndex level = 0,next_index = 0;
  TIndex start;
  TIndex startlevel;
  
  internal_stack::const_iterator first = Stack.begin(),last = Stack.end();
  while (first != last) {
    if (first->Type == backref_entry::OPEN) {
      if (number == next_index) {
        start = first->Index;
	startlevel = level;
 	level++;
        break;
        }
      next_index++;
      level++;
      }
    if (first->Type == backref_entry::CLOSE) 
      level--;
    first++;
    }
  
  if (first == last)
    EX_THROW(regex,ECRE_INVBACKREF)

  first++;
    
  while (first != last) {
    if (first->Type == backref_entry::OPEN) 
      level++;
    if (first->Type == backref_entry::CLOSE) {
      level--;
      if (startlevel == level)
        return candidate.substr(start,first->Index - start);
      }
    first++;
    }
  EX_THROW(regex,ECRE_UNBALBACKREF)
  }




// regex::matcher -------------------------------------------------------------
template<class T>
ixion::regex<T>::matcher::matcher()
  : Next(NULL) { 
  }




template<class T>
ixion::regex<T>::matcher::~matcher() { 
  if (Next && OwnNext) 
    delete Next;
  }




template<class T>
ixion::TSize ixion::regex<T>::matcher::subsequentMatchLength() const {
  TSize totalml = 0;
  matcher const *object = this;
  while (object) {
    totalml += object->MatchLength;
    object = object->Next;
    }
  return totalml;
  }




template<class T>
ixion::TSize ixion::regex<T>::matcher::minimumSubsequentMatchLength() const  {
  TSize totalml = 0;
  matcher const *object = this;
  while (object) {
    totalml += object->minimumMatchLength();
    object = object->Next;
    }
  return totalml;
  }




template<class T>
void ixion::regex<T>::matcher::copy(matcher const *src) {
  if (src->Next && src->OwnNext)
    setNext(src->Next->duplicate(),src->OwnNext);
  else
    setNext(NULL);
  }




// regex::quantifier ----------------------------------------------------------
template<class T>
ixion::regex<T>::quantifier::quantifier(bool greedy,TSize mincount)
  : Greedy(greedy),MaxValid(false),MinCount(mincount) { 
  }




template<class T>
ixion::regex<T>::quantifier::quantifier(bool greedy,TSize mincount,TSize maxcount)
  : Greedy(greedy),MaxValid(true),MinCount(mincount),MaxCount(maxcount) { 
  }




template<class T>
ixion::regex<T>::quantifier::~quantifier() { 
  if (Quantified) 
    delete Quantified; 
  }




template<class T>
ixion::regex<T>::matcher *ixion::regex<T>::quantifier::duplicate() const {
  quantifier *dupe = new quantifier();
  dupe->copy(this);
  return dupe;
  }




template<class T>
ixion::TSize ixion::regex<T>::quantifier::minimumMatchLength() const {
  if (Quantified) 
    return MinCount * Quantified->minimumMatchLength();
  else 
    return 0;
  }




template<class T>
bool ixion::regex<T>::quantifier::match(backref_stack &brstack,T const &candidate,TIndex at) {
  // this routine does speculative matching, so it must pay close attention
  // to rewind the backref stack appropriately.
  // NB: matchNext does the rewinding automatically, whereas speculative
  // matches of the quantified portion must be rewound.
  
  // There should be at least one character in each match, we'd
  // run to Baghdad otherwise.
  
  if (!Quantified) 
    return matchNext(brstack,candidate,at);
  
  // calculate accurate maximum match count
  TSize quant_min = Quantified->minimumSubsequentMatchLength();
  if (quant_min == 0) quant_min = 1;
  
  TSize max_count = candidate.size() - at;
  if (Next) max_count -= Next->minimumSubsequentMatchLength();
  max_count = max_count/quant_min + 1;
  
  if (MaxValid) max_count = NUM_MIN(max_count,MaxCount);
  
  // check that at least MinCount matches take place (non-speculative)
  TIndex idx = at;
  for (TSize c = 1;c <= MinCount;c++)
    if (Quantified->match(brstack,candidate,idx))
      idx += Quantified->subsequentMatchLength();
    else 
      return false;
  
  // determine number of remaining matches
  TSize remcount = max_count-MinCount;
  
  // test for the remaining matches in a way that depends on Greedy flag
  if (Greedy) {
    // try to gobble up as many matches of quantified part as possible
    // (speculative)
    
    std::stack<backtrack_stack_entry> successful_indices;
    { backtrack_stack_entry entry = { idx,brstack.getRewindInfo() };
      successful_indices.push(entry);
      }
    
    while (Quantified->match(brstack,candidate,idx) && successful_indices.size()-1 < remcount) {
      idx += Quantified->subsequentMatchLength();
      backtrack_stack_entry entry = { idx,brstack.getRewindInfo() };
      successful_indices.push(entry);
      }
    
    // backtrack until rest of sequence also matches
    while (successful_indices.size() && !matchNext(brstack,candidate,successful_indices.top().Index)) {
      brstack.rewind(successful_indices.top().RewindInfo);
      successful_indices.pop();
      }
    
    if (successful_indices.size()) {
      MatchLength = successful_indices.top().Index - at;
      return true;
      }
    else return false;
    }
  else {
    for (TSize c = 0;c <= remcount;c++) {
      if (matchNext(brstack,candidate,idx)) {
        MatchLength = idx-at;
        return true;
        }
      // following part runs once too much, effectively: 
      // if c == remcount, idx may be increased, but the search fails anyway
      // => no problem
      if (Quantified->match(brstack,candidate,idx))
        idx += Quantified->subsequentMatchLength();
      else 
        return false;
      }
    return false;
    }
  }




template<class T>
void ixion::regex<T>::quantifier::copy(quantifier const *src) {
  super::copy(src);
  Greedy = src->Greedy;
  MaxValid = src->MaxValid;
  MinCount = src->MinCount;
  MaxCount = src->MaxCount;
  Quantified = src->Quantified->duplicate();
  }




// regex::sequence_matcher ------------------------------------------------------
template<class T>
ixion::regex<T>::sequence_matcher::sequence_matcher(T const &matchstr)
  : MatchStr(matchstr) { 
  MatchLength = MatchStr.size(); 
  }




template<class T>
ixion::regex<T>::matcher *ixion::regex<T>::sequence_matcher::duplicate() const {
  sequence_matcher *dupe = new sequence_matcher(MatchStr);
  dupe->copy(this);
  return dupe;
  }




template<class T>
bool ixion::regex<T>::sequence_matcher::match(backref_stack &brstack,T const &candidate,TIndex at) {
  if (at+MatchStr.size() > candidate.size()) return false;
  return (T(candidate.begin()+at,candidate.begin()+at+MatchStr.size()) == MatchStr) &&
    matchNext(brstack,candidate,at+MatchStr.size());
  }




// regex::any_matcher ---------------------------------------------------------
template<class T>
ixion::regex<T>::matcher *ixion::regex<T>::any_matcher::duplicate() const {
  any_matcher *dupe = new any_matcher();
  dupe->copy(this);
  return dupe;
  }




// regex::start_matcher ---------------------------------------------------------
template<class T>
ixion::regex<T>::matcher *ixion::regex<T>::start_matcher::duplicate() const {
  start_matcher *dupe = new start_matcher();
  dupe->copy(this);
  return dupe;
  }




template<class T>
bool ixion::regex<T>::start_matcher::match(backref_stack &brstack,T const &candidate,TIndex at) { 
  return (at == 0) && matchNext(brstack,candidate,at);
  }




// regex::end_matcher ---------------------------------------------------------
template<class T>
ixion::regex<T>::matcher *ixion::regex<T>::end_matcher::duplicate() const {
  end_matcher *dupe = new end_matcher();
  dupe->copy(this);
  return dupe;
  }




template<class T>
bool ixion::regex<T>::end_matcher::match(backref_stack &brstack,T const &candidate,TIndex at) { 
  return (at == candidate.size()) && matchNext(brstack,candidate,at);
  }




// regex::backref_open_matcher ------------------------------------------------
template<class T>
ixion::regex<T>::matcher *ixion::regex<T>::backref_open_matcher::duplicate() const {
  backref_open_matcher *dupe = new backref_open_matcher();
  dupe->copy(this);
  return dupe;
  }




template<class T>
bool ixion::regex<T>::backref_open_matcher::match(backref_stack &brstack,T const &candidate,TIndex at) {
  backref_stack::rewind_info ri = brstack.getRewindInfo();
  brstack.open(at);
  
  bool result = matchNext(brstack,candidate,at);

  if (!result)
    brstack.rewind(ri);
  return result;
  }




// regex::backref_close_matcher -----------------------------------------------
template<class T>
ixion::regex<T>::matcher *ixion::regex<T>::backref_close_matcher::duplicate() const {
  backref_close_matcher *dupe = new backref_close_matcher();
  dupe->copy(this);
  return dupe;
  }




template<class T>
bool ixion::regex<T>::backref_close_matcher::match(backref_stack &brstack,T const &candidate,TIndex at) {
  backref_stack::rewind_info ri = brstack.getRewindInfo();
  brstack.close(at);
  
  bool result = matchNext(brstack,candidate,at);

  if (!result)
    brstack.rewind(ri);
  return result;
  }




// regex::alternative_matcher::connector --------------------------------------
template<class T>
bool ixion::regex<T>::alternative_matcher::connector::match(backref_stack &brstack,T const &candidate,TIndex at) {
  return matchNext(brstack,candidate,at);
  }




// regex::alternative_matcher -------------------------------------------------
template<class T>
ixion::regex<T>::alternative_matcher::~alternative_matcher() {
  while (AltList.size()) {
    delete AltList.back();
    AltList.pop_back();
    }
  }




template<class T>
ixion::regex<T>::matcher *ixion::regex<T>::alternative_matcher::duplicate() const {
  alternative_matcher *dupe = new alternative_matcher();
  dupe->copy(this);
  return dupe;
  }




template<class T>
ixion::TSize ixion::regex<T>::alternative_matcher::minimumMatchLength() const {
  TSize result = 0;
  bool is_first = true;
  
  FOREACH_CONST(first,AltList,alt_list)
    if (is_first) {
      result = (*first)->minimumMatchLength();
      is_first = true;
      }
    else {
      TSize current = (*first)->minimumMatchLength();
      if (current < result) result = current;
      }
  return result;
  }




template<class T>
void ixion::regex<T>::alternative_matcher::setNext(matcher *next,bool ownnext = true) {
  matcher::setNext(next);
  Connector.setNext(next,false);
  }




template<class T>
void ixion::regex<T>::alternative_matcher::addAlternative(matcher *alternative) {
  AltList.push_back(alternative);
  matcher *searchlast = alternative,*last = NULL;
  while (searchlast) {
    last = searchlast;
    searchlast = searchlast->getNext();
    }
  last->setNext(&Connector,false);
  }




template<class T>
bool ixion::regex<T>::alternative_matcher::match(backref_stack &brstack,T const &candidate,TIndex at) {
  std::vector<matcher *>::iterator first = AltList.begin(),last = AltList.end();
  while (first != last) {
    if ((*first)->match(brstack,candidate,at)) {
      MatchLength = 0;
      matcher const *object = *first;
      while (object != &Connector) {
        MatchLength += object->getMatchLength();
        object = object->getNext();
        }
      return true;
      }
    first++;
    }
  return false;
  }




template<class T>
void ixion::regex<T>::alternative_matcher::copy(alternative_matcher const *src) {
  super::copy(src);
  Connector.setNext(Next,false);
  
  FOREACH_CONST(first,src->AltList,alt_list)
    addAlternative((*first)->duplicate());
  }




// regex::backref_matcher -----------------------------------------------------
template<class T>
ixion::regex<T>::backref_matcher::backref_matcher(TIndex backref)
  : Backref(backref) { 
  }




template<class T>
ixion::regex<T>::matcher *ixion::regex<T>::backref_matcher::duplicate() const {
  backref_matcher *dupe = new backref_matcher(Backref);
  dupe->copy(this);
  return dupe;
  }




template<class T>
bool ixion::regex<T>::backref_matcher::match(backref_stack &brstack,T const &candidate,TIndex at) {
  T matchstr = brstack.get(Backref,candidate);
  MatchLength = matchstr.size();

  if (at+matchstr.size() > candidate.size()) return false;
  return (T(candidate.begin()+at,candidate.begin()+at+matchstr.size()) == matchstr) &&
    matchNext(brstack,candidate,at+matchstr.size());
  }




// regex ----------------------------------------------------------------------
template<class T>
ixion::regex<T>::regex()
  : MatchIndex(0),MatchLength(0) { 
  }




template<class T>
ixion::regex<T>::regex(regex const &src)
  : ParsedRegex(src.ParsedRegex->duplicate()),
  MatchIndex(0),MatchLength(0) {
  }




template<class T>
ixion::regex<T> &ixion::regex<T>::operator=(regex const &src) {
  std::auto_ptr<matcher> regex_copy(src.ParsedRegex->duplicate());
  ParsedRegex = regex_copy;
  return *this;
  }




template<class T>
bool ixion::regex<T>::match(T const &candidate,TIndex from) {
  LastCandidate = candidate;
  BackrefStack.clear();

  if (ParsedRegex.get() == NULL)
    EX_THROW(regex,ECRE_NOPATTERN)

  for (TIndex index = from;index < candidate.size();index++)
    if (ParsedRegex->match(BackrefStack,candidate,index)) {
      MatchIndex = index;
      MatchLength = ParsedRegex->subsequentMatchLength();
      return true;
      }
  return false;
  }




template<class T>
bool ixion::regex<T>::matchAt(T const &candidate,TIndex at) {
  LastCandidate = candidate;
  BackrefStack.clear();

  if (ParsedRegex.get() == NULL)
    EX_THROW(regex,ECRE_NOPATTERN)

  if (ParsedRegex->match(BackrefStack,candidate,at)) {
    MatchIndex = at;
    MatchLength = ParsedRegex->subsequentMatchLength();
    return true;
    }
  return false;
  }
