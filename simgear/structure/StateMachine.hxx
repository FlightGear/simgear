// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 James Turner

/**
 * @file
 * @brief Provides a finite state machine (FSM) associated with user input events.
 */

#pragma once

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

/**
 * Provides a finite state machine (FSM) associated with user input (i.e keypresses, joystick inputs).
 *
 * ## What is a state machine?
 * Conceptually, a state machine is a system that can be in exactly _one_ of a finite number of states at a given time.
 * A state machine can change from one state to another state in response to _inputs_. This changing of states is called a _transition_.
 * In practice, state machines are very commonly used to model the behavior of systems that exhibit a sequence of pre-determined actions.
 * Examples include vending machines, traffic lights, or an autopilot.
 *
 * @see https://en.wikipedia.org/wiki/Finite-state_machine
 */
class StateMachine : public SGReferenced
{
public:
    StateMachine();
    virtual ~StateMachine();

    /**
     * Responsible for the state of the FSM.
     */
    class State : public SGReferenced
    {
    public:
        virtual ~State();

        /**
         * Get the name given to a state
         * @return The human-readable name of the state
         */
        std::string name() const;

        /**
         * Add a binding that ??? upon FSM update
         * @param aBinding
         */
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
        std::unique_ptr<StatePrivate> d;
    };

    /**
     * Responsible for managing state transitions within the FSM.
     */
    class Transition : public SGReferenced
    {
    public:
        virtual ~Transition();

        /**
         * Get the name given to a state
         * @return The human-readable name of the state
         */
        std::string name() const;

        /**
         * Set if the target state should automatically be excluded
         * from the source state. Defaults to true, can be cleared
         * to allow a state to re-enter itself
         */
        void setExcludeTarget(bool aExclude);

        /**
         * @return The state we end in, after this transition is triggered.
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
        std::unique_ptr<TransitionPrivate> d;
    };

    typedef SGSharedPtr<State> State_ptr;
    typedef SGSharedPtr<Transition> Transition_ptr;

    void initFromPlist(SGPropertyNode* desc, SGPropertyNode* root);

    /**
     * Create a state machine from a property list description, while handling initialization.
     *
     * @note Creating a state machine with this function does not require calling the `init()` function.
     */
    static StateMachine* createFromPlist(SGPropertyNode* desc, SGPropertyNode* root);

    SGPropertyNode* root();

    /**
     * Initializes a state machine. If called when a state machine has already been initialized, it
     * returns immediately.
     *
     * @throw sg_range_exception If the state machine has been initialized with no states defined.
     */
    void init();
    void shutdown();

    /**
     * Update the state machine, triggering the next iteration.
     *
     * When the state machine updates, it evaluates inputs, checks for conditions that may trigger
     * a transition, and updates the state of the machine accordingly. Depending on the conditions,
     * it could maintain the current state, or transition to a new one.
     */
    void update(double dt);

    State_ptr state() const;

    /**
     * Force the state machine to transition to the specified state.
     *
     * @param aState The target state to which the state machine must transition.
     * @param aOnlyIfDifferent true, the state transition only occurs if the target
     * state is different from the current state. If false, the existing state will be
     * exited and re-entered, even if it is the same a the target state.
     * @throw sg_exception If the specified state does not exist in the state machine.
     */
    void changeToState(State_ptr aState, bool aOnlyIfDifferent=true);

    /**
     * Force the state machine to transition to a state identified by its name.
     * This is a wrapper around changeToState().
     *
     * @param aName The name of the target state to which the state machine must transition.
     * @param aOnlyIfDifferent If true, the state transition only occurs if the target
     * state is different from the current state. If false, the existing state will be
     * exited and re-entered, even if it is the same a the target state.
     * @throw sg_exception If the specified state does not exist in the state machine.
     */
    void changeToStateName(const std::string& aName, bool aOnlyIfDifferent=true);

    State_ptr findStateByName(const std::string& stateName) const;

    State_ptr stateByIndex(unsigned int aIndex) const;

    int indexOfState(State_ptr aState) const;

    // programmatic creation
    State_ptr createState(const std::string& aName);
    Transition_ptr createTransition(const std::string& aName, State_ptr aTarget);
private:
    void addState(State_ptr aState);
    void addTransition(Transition_ptr aTrans);

    void innerChangeState(State_ptr aState, Transition_ptr aTrans);

    class StateMachinePrivate;
    std::unique_ptr<StateMachinePrivate> d;
};

typedef SGSharedPtr<StateMachine> StateMachine_ptr;

}