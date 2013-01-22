/* -*-c++-*-
 *
 * Copyright (C) 2013 James Turner
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif
     
#include "StateMachine.hxx"

#include <algorithm>
#include <cassert>
#include <set>
#include <boost/foreach.hpp>

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGBinding.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/structure/exception.hxx>
                    
namespace simgear
{

typedef std::vector<StateMachine::State_ptr> StatePtrVec;

static void readBindingList(SGPropertyNode* desc, const std::string& name, 
    SGPropertyNode* root, SGBindingList& result)
{
    BOOST_FOREACH(SGPropertyNode* b, desc->getChildren(name)) {
        SGBinding* bind = new SGBinding;
        bind->read(b, root);
        result.push_back(bind);
    }
}

///////////////////////////////////////////////////////////////////////////

class StateMachine::State::StatePrivate
{
public:
    std::string _name;
    SGBindingList _updateBindings, 
        _entryBindings, 
        _exitBindings;
};

///////////////////////////////////////////////////////////////////////////

class StateMachine::Transition::TransitionPrivate
{
public:
    std::string _name;
    SGBindingList _bindings;
    std::set<State*> _sourceStates; ///< weak refs to source states
    State* _target;
    SGSharedPtr<SGCondition> _condition;
};
    
///////////////////////////////////////////////////////////////////////////

class StateMachine::StateMachinePrivate : public SGPropertyChangeListener
{
public:
    StateMachinePrivate(StateMachine* p) : _p(p) { }
    
    void computeEligibleTransitions()
    {
        _eligible.clear();
        BOOST_FOREACH(Transition_ptr t, _transitions) {
            if (t->applicableForState(_currentState)) {
                _eligible.push_back(t.ptr());
            }
        }
    }
    
    StateMachine* _p;
    State_ptr _currentState;
    StatePtrVec _states;
    std::vector<Transition_ptr> _transitions;
    std::vector<Transition*> _eligible;
    SGTimeStamp _timeInState;
    
    bool _listenerLockout; ///< block our listener when self-updating props
    virtual void valueChanged(SGPropertyNode* changed)
    {
        if (_listenerLockout) {
            return;
        }
        
        if (changed == _currentStateIndex) {
            State_ptr s = _p->stateByIndex(changed->getIntValue());
            _p->changeToState(s);
        } else if (changed == _currentStateName) {
            _p->changeToStateName(changed->getStringValue());
        }
    }
    
// exposed properties
    SGPropertyNode_ptr _root;
    SGPropertyNode_ptr _currentStateIndex;
    SGPropertyNode_ptr _currentStateName;
    SGPropertyNode_ptr _timeInStateProp;
};

///////////////////////////////////////////////////////////////////////////

StateMachine::State::State(const std::string& aName) :
    d(new StatePrivate)
{
    d->_name = aName;
}

StateMachine::State::~State()
{
}

std::string StateMachine::State::name() const
{
    return d->_name;
}

void StateMachine::State::update()
{
    fireBindingList(d->_updateBindings);
}

void StateMachine::State::fireEntryBindings()
{
    fireBindingList(d->_entryBindings);
}   

void StateMachine::State::fireExitBindings()
{
    fireBindingList(d->_exitBindings);
}   

void StateMachine::State::addUpdateBinding(SGBinding* aBinding)
{
    d->_updateBindings.push_back(aBinding);
}

void StateMachine::State::addEntryBinding(SGBinding* aBinding)
{
    d->_entryBindings.push_back(aBinding);
}

void StateMachine::State::addExitBinding(SGBinding* aBinding)
{
    d->_exitBindings.push_back(aBinding);
}

///////////////////////////////////////////////////////////////////////////

StateMachine::Transition::Transition(const std::string& aName, State* aTarget) :
    d(new TransitionPrivate)
{
    assert(aTarget);
    d->_name = aName;
    d->_target = aTarget;
}

StateMachine::Transition::~Transition()
{
}

StateMachine::State* StateMachine::Transition::target() const
{
    return d->_target;
}    

void StateMachine::Transition::addSourceState(State* aSource)
{
    if (aSource == d->_target) { // should this be disallowed outright?
        SG_LOG(SG_GENERAL, SG_WARN, d->_name << ": adding target state as source");
    }
    
    d->_sourceStates.insert(aSource);
}    

bool StateMachine::Transition::applicableForState(State* aCurrent) const
{
    return d->_sourceStates.count(aCurrent);
} 
    
bool StateMachine::Transition::evaluate() const
{
    return d->_condition->test();
}    

void StateMachine::Transition::fireBindings()
{
    fireBindingList(d->_bindings);
}   

std::string StateMachine::Transition::name() const
{
    return d->_name;
}

void StateMachine::Transition::setTriggerCondition(SGCondition* aCondition)
{
    d->_condition = aCondition;
}

void StateMachine::Transition::addBinding(SGBinding* aBinding)
{
    d->_bindings.push_back(aBinding);
}
    
///////////////////////////////////////////////////////////////////////////

StateMachine::StateMachine() :
    d(new StateMachinePrivate(this))
{
    d->_root = new SGPropertyNode();
}

StateMachine::~StateMachine()
{
    
}

void StateMachine::init()
{
    
    
    d->_currentStateIndex = d->_root->getChild("current-index", 0, true);
    d->_currentStateIndex->setIntValue(0);
    
    d->_currentStateName = d->_root->getChild("current-name", 0, true);
    d->_currentStateName->setStringValue("");
    
    d->_currentStateIndex->addChangeListener(d.get());
    d->_currentStateName->addChangeListener(d.get());
    
    d->_timeInStateProp = d->_root->getChild("elapsed-time-msec", 0, true);
    d->_timeInStateProp->setIntValue(0);
    
    // TODO go to default state if found
    d->computeEligibleTransitions();
    
}

void StateMachine::shutdown()
{
    d->_currentStateIndex->removeChangeListener(d.get());
    d->_currentStateName->removeChangeListener(d.get());
    
}

void StateMachine::innerChangeState(State_ptr aState, Transition_ptr aTrans)
{
    d->_currentState->fireExitBindings();
        
// fire bindings before we change the state, hmmmm    
    if (aTrans) {
        aTrans->fireBindings();
    }
    
    // update our private state and properties
    d->_listenerLockout = true;
    d->_currentState = aState;
    d->_timeInState.stamp();
    d->_currentStateName->setStringValue(d->_currentState->name());
    d->_currentStateIndex->setIntValue(indexOfState(aState));
    d->_timeInStateProp->setIntValue(0);
    d->_listenerLockout = false;
    
    // fire bindings
    d->_currentState->fireEntryBindings();
    d->_currentState->update();
        
    d->computeEligibleTransitions();
}

void StateMachine::changeToState(State_ptr aState, bool aOnlyIfDifferent)
{
    assert(aState != NULL);
    if (std::find(d->_states.begin(), d->_states.end(), aState) == d->_states.end()) {
        throw sg_exception("Requested change to state not in machine");
    }
    
    if (aOnlyIfDifferent && (aState == d->_currentState)) {
        return;
    }
    
    innerChangeState(aState, NULL);
}

void StateMachine::changeToStateName(const std::string& aName, bool aOnlyIfDifferent)
{
    State_ptr st = findStateByName(aName);
    if (!st) {
        throw sg_range_exception("unknown state:" + aName);
    }
    
    changeToState(st, aOnlyIfDifferent);
}

StateMachine::State_ptr StateMachine::state() const
{
    return d->_currentState;
}
    
SGPropertyNode* StateMachine::root()
{
    return d->_root;
}

void StateMachine::update(double aDt)
{
    // do this first, for triggers which depend on time in current state
    // (spring-loaded transitions)
    d->_timeInStateProp->setIntValue(d->_timeInState.elapsedMSec());
    
    Transition_ptr trigger;
    
    BOOST_FOREACH(Transition* trans, d->_eligible) {
        if (trans->evaluate()) {
            if (trigger != Transition_ptr()) {
                SG_LOG(SG_GENERAL, SG_WARN, "ambiguous transitions! " 
                    << trans->name() << " or "  << trigger->name());
            }
            
            trigger = trans;
        }
    }
    
    if (trigger != Transition_ptr()) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "firing transition:" << trigger->name());
        innerChangeState(trigger->target(), trigger);
    }
    
    d->_currentState->update();
}    

StateMachine::State_ptr StateMachine::findStateByName(const std::string& aName) const
{
    BOOST_FOREACH(State_ptr sp, d->_states) {
        if (sp->name() == aName) {
            return sp;
        }
    }
    
    SG_LOG(SG_GENERAL, SG_WARN, "unknown state:" << aName);
    return State_ptr();
}

StateMachine::State_ptr StateMachine::stateByIndex(unsigned int aIndex) const
{
    if (aIndex >= d->_states.size()) {
        throw sg_range_exception("invalid state index, out of bounds");
    }
    
    return d->_states[aIndex];
}
    
int StateMachine::indexOfState(State_ptr aState) const
{
    StatePtrVec::const_iterator it = std::find(d->_states.begin(), d->_states.end(), aState);
    if (it == d->_states.end()) {
        return -1;
    }
    
    return it - d->_states.begin();
}

StateMachine::State_ptr StateMachine::createState(const std::string& aName)
{
    if (findStateByName(aName) != NULL) {
        throw sg_range_exception("duplicate state name");
    }
    
    State_ptr st = new State(aName);
    addState(st);
    return st;
}

StateMachine::Transition_ptr
StateMachine::createTransition(const std::string& aName, State_ptr aTarget)
{
    Transition_ptr t = new Transition(aName, aTarget);
    addTransition(t);
    return t;
}

StateMachine* StateMachine::createFromPlist(SGPropertyNode* desc, SGPropertyNode* root)
{
    StateMachine* sm = new StateMachine;
    
    
    BOOST_FOREACH(SGPropertyNode* stateDesc, desc->getChildren("state")) {
        std::string nm = stateDesc->getStringValue("name");
        State_ptr st(new State(nm));
        
        readBindingList(stateDesc, "enter", root, st->d->_updateBindings);
        readBindingList(stateDesc, "exit", root, st->d->_entryBindings);
        readBindingList(stateDesc, "update", root, st->d->_exitBindings);
        
        sm->addState(st);
    } // of states iteration
    
    BOOST_FOREACH(SGPropertyNode* tDesc, desc->getChildren("transition")) {
        std::string nm = tDesc->getStringValue("name");
        State_ptr target = sm->findStateByName(tDesc->getStringValue("target"));
        
        SGCondition* cond = sgReadCondition(root, tDesc->getChild("condition"));
        
        Transition_ptr t(new Transition(nm, target));
        t->setTriggerCondition(cond);
        
        BOOST_FOREACH(SGPropertyNode* src, desc->getChildren("source")) {
            State_ptr srcState = sm->findStateByName(src->getStringValue());
            t->addSourceState(srcState);
        }
        
        readBindingList(tDesc, "binding", root, t->d->_bindings);
        
        sm->addTransition(t);
    } // of states iteration
    
    return sm;
}

void StateMachine::addState(State_ptr aState)
{
    bool wasEmpty = d->_states.empty();
    d->_states.push_back(aState);
    if (wasEmpty) {
        d->_currentState = aState;
    }
}

void StateMachine::addTransition(Transition_ptr aTrans)
{
    d->_transitions.push_back(aTrans);
}

} // of namespace simgear
