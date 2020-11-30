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

#include <simgear_config.h>

#include "StateMachine.hxx"

#include <algorithm>
#include <cassert>
#include <set>

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
    for (auto b : desc->getChildren(name)) {
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
    bool _excludeTarget;
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
        for (Transition_ptr t : _transitions) {
            if (t->applicableForState(_currentState)) {
                _eligible.push_back(t.ptr());
            }
        }
    }

    StateMachine* _p;
    bool _initialised;
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
    d->_excludeTarget = true;
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
    if (d->_excludeTarget && (aCurrent == d->_target)) {
        return false;
    }

    if (d->_sourceStates.empty()) {
        return true;
    }
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

void StateMachine::Transition::setExcludeTarget(bool aExclude)
{
    d->_excludeTarget = aExclude;
}

///////////////////////////////////////////////////////////////////////////

StateMachine::StateMachine() :
    d(new StateMachinePrivate(this))
{
    d->_root = new SGPropertyNode();
    d->_listenerLockout = false;
    d->_initialised = false;
}

StateMachine::~StateMachine()
{

}

void StateMachine::init()
{
    if (d->_initialised) {
	    return;
    }

    if (d->_states.empty()) {
        throw sg_range_exception("StateMachine::init: no states defined");
    }

    d->_currentStateIndex = d->_root->getChild("current-index", 0, true);
    d->_currentStateIndex->setIntValue(0);

    d->_currentStateName = d->_root->getChild("current-name", 0, true);
    d->_currentStateName->setStringValue("");

    d->_currentStateIndex->addChangeListener(d.get());
    d->_currentStateName->addChangeListener(d.get());

    d->_timeInStateProp = d->_root->getChild("elapsed-time-msec", 0, true);
    d->_timeInStateProp->setIntValue(0);

    // TODO go to default state if found
    innerChangeState(d->_states[0], NULL);
    d->_initialised = true;
}

void StateMachine::shutdown()
{
    d->_currentStateIndex->removeChangeListener(d.get());
    d->_currentStateName->removeChangeListener(d.get());

}

void StateMachine::innerChangeState(State_ptr aState, Transition_ptr aTrans)
{
    if (d->_currentState) {
        d->_currentState->fireExitBindings();
        SG_LOG(SG_GENERAL, SG_INFO, "Changing from state " << d->_currentState->name() << " to state:" << aState->name());
    } else {
        SG_LOG(SG_GENERAL, SG_INFO, "Initializing to state:" << aState->name());      
    }

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

    for (auto trans : d->_eligible) {
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
    for (auto sp : d->_states) {
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

void StateMachine::initFromPlist(SGPropertyNode* desc, SGPropertyNode* root)
{
    std::string path = desc->getStringValue("branch");
    if (!path.empty()) {
        d->_root = root->getNode(path, 0, true);
        assert(d->_root);
    }

    int stateCount = 0;
    for (auto stateDesc : desc->getChildren("state")) {
        stateCount++;
        std::string nm = stateDesc->getStringValue("name");

        if (nm.empty()) {
          SG_LOG(SG_GENERAL, SG_DEV_ALERT, "No name found for state in branch " << path);
          throw sg_exception("No name element in state");
        }

        State_ptr st(new State(nm));

        readBindingList(stateDesc, "enter", root, st->d->_entryBindings);
        readBindingList(stateDesc, "update", root, st->d->_updateBindings);
        readBindingList(stateDesc, "exit", root, st->d->_exitBindings);

        addState(st);
    } // of states iteration

    if (stateCount < 2) {
      SG_LOG(SG_GENERAL, SG_ALERT, "Fewer than two state elements found in branch " << path);
      throw sg_exception("Fewer than two state elements found.");
    }

    for (auto tDesc : desc->getChildren("transition")) {
        std::string nm = tDesc->getStringValue("name");
        std::string target_id = tDesc->getStringValue("target");

        if (nm.empty()) {
          SG_LOG(SG_GENERAL, SG_DEV_WARN, "No name found for transition in branch " << path);
          nm = "transition-to-" + target_id;
        }

        if (target_id.empty()) {
          SG_LOG(SG_GENERAL, SG_ALERT, "No target element in transition "
              << nm << " in state branch " << path);
          throw sg_exception("No target element in transition");
        }

        State_ptr target = findStateByName(target_id);

        if (target == NULL) {
          SG_LOG(SG_GENERAL, SG_ALERT, "Unknown target state " << target_id << " in transition "
              << nm << " in state branch " << path);
          throw sg_exception("No condition element in transition");
        }

        if (tDesc->getChild("condition") == NULL) {
          SG_LOG(SG_GENERAL, SG_ALERT, "No condition element in transition "
              << nm << " in state branch " << path);
          throw sg_exception("No condition element in transition");
        }

        SGCondition* cond = sgReadCondition(root, tDesc->getChild("condition"));

        Transition_ptr t(new Transition(nm, target));
        t->setTriggerCondition(cond);

        t->setExcludeTarget(tDesc->getBoolValue("exclude-target", true));
        for (auto src : tDesc->getChildren("source")) {
            State_ptr srcState = findStateByName(src->getStringValue());

            if (srcState == NULL) {
              SG_LOG(SG_GENERAL, SG_ALERT, "Unknown source state " << src->getStringValue() << " in transition "
                  << nm << " in state branch " << path);
              throw sg_exception("No condition element in transition");
            }

            t->addSourceState(srcState);
        }

        readBindingList(tDesc, "binding", root, t->d->_bindings);

        addTransition(t);
    } // of states iteration

    init();
}

StateMachine* StateMachine::createFromPlist(SGPropertyNode* desc, SGPropertyNode* root)
{
    StateMachine* sm = new StateMachine;
    sm->initFromPlist(desc, root);
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
