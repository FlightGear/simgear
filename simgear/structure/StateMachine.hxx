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

#ifndef SIMGEAR_STATE_MACHINE_H
#define SIMGEAR_STATE_MACHINE_H

#include <memory>
#include <string>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
     
// forward decls
class SGPropertyNode;
class SGBinding;
class SGCondition;

namespace simgear
{

class StateMachine : public SGReferenced
{
public:
    StateMachine();
    virtual ~StateMachine();
    
    class State : public SGReferenced
    {
    public:    
        virtual ~State();
        
        std::string name() const;
        
        void addUpdateBinding(SGBinding* aBinding);
        void addEntryBinding(SGBinding* aBinding);
        void addExitBinding(SGBinding* aBinding);
        
    private:  
        friend class StateMachine;
        
        State(const std::string& name);
        
        void fireExitBindings();
        void fireEntryBindings();
        
        void update();
        
        class StatePrivate;
        std::auto_ptr<StatePrivate> d;
    };
    
    class Transition : public SGReferenced
    {
    public:
        virtual ~Transition();
        
        std::string name() const;
        
        /**
         * Set if the target state should automatically be excluded
         * from the source state. Defaults to true, can be cleared
         * to allow a state to re-enter itself
         */
        void setExcludeTarget(bool aExclude);
        
        
        /**
         * The state we end in, after this transition fires
         */
        State* target() const;
        
        /**
         * Add a state in which this transition is eligible to fire
         */
        void addSourceState(State* aSource);
        
        /**
         * Specify the transition trigger condition. Takes ownership
         */
        void setTriggerCondition(SGCondition* aCondition);
        
        
        void addBinding(SGBinding* aBinding);
    private:
        friend class StateMachine;
        
        Transition(const std::string& aName, State* aTarget);
        
        /**
         * predicate to determine if this transition can fire given a
         * current state.
         */
        bool applicableForState(State* aCurrent) const;
        
        /**
        * test if the transition should fire, based on current state
        */
        bool evaluate() const;
         
        void fireBindings();
    
        class TransitionPrivate;
        std::auto_ptr<TransitionPrivate> d;
    };
    
    typedef SGSharedPtr<State> State_ptr;
    typedef SGSharedPtr<Transition> Transition_ptr;
    
    void initFromPlist(SGPropertyNode* desc, SGPropertyNode* root);
    
    /**
     * create a state machine from a property list description
     */
    static StateMachine* createFromPlist(SGPropertyNode* desc, SGPropertyNode* root);
    
    SGPropertyNode* root();
    
    void init();
    void shutdown();
    
    void update(double dt);
    
    State_ptr state() const;
    
    /**
     * public API to force a change to a particular state.
     * @param aOnlyIfDifferent - only make a transition if the new state is
     *   different from the current state. Otherwise, the existing state will
     * be exited and re-entered.
     */
    void changeToState(State_ptr aState, bool aOnlyIfDifferent=true);
    
     /// wrapper to change state by looking up a name
    void changeToStateName(const std::string& aName, bool aOnlyIfDifferent=true);
    
    State_ptr findStateByName(const std::string& aName) const;
    
    State_ptr stateByIndex(unsigned int aIndex) const;
    
    int indexOfState(State_ptr aState) const;
    
    // programatic creation
    State_ptr createState(const std::string& aName);
    Transition_ptr createTransition(const std::string& aName, State_ptr aTarget);
private:
    void addState(State_ptr aState);
    void addTransition(Transition_ptr aTrans);
    
    void innerChangeState(State_ptr aState, Transition_ptr aTrans);
    
    class StateMachinePrivate;
    std::auto_ptr<StateMachinePrivate> d;
};

typedef SGSharedPtr<StateMachine> StateMachine_ptr;

} // of simgear namespace

#endif // of SIMGEAR_STATE_MACHINE_H
