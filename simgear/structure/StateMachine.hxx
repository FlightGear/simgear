// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 James Turner <james@flightgear.org>

/**
 * @file
 * @brief Provides a finite state machine (FSM) associated with user input events.
 */

#pragma once

#include <memory>
#include <string>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

// Forward declarations
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
         *
         * @return The human-readable name of the state
         */
        std::string name() const;

        /**
         * Adds a new binding to the list of update bindings for this state.
         *
         * The added binding will be fired (evaluated and triggered) upon each
         * update of the state machine.
         *
         * @param aBinding A pointer to the binding to be added.
         */
        void addUpdateBinding(SGBinding* aBinding);

        /**
         * Adds a new binding to the list of entry bindings for this state.
         *
         * An exit binding will be fired (evaluated and triggered) upon entering 
         * a state.
         *
         * @param aBinding A pointer to the binding to be added.
         */
        void addEntryBinding(SGBinding* aBinding);

        /**
         * Adds a new binding to the list of exit bindings for this state.
         *
         * An exit binding will be fired (evaluated and triggered) upon exiting
         * a state.
         *
         * @param aBinding A pointer to the binding to be added.
         */
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
         * Get the name given to the state transition.
         *
         * @return The human-readable name of the state transition.
         */
        std::string name() const;

        /**
         * Set if the target state should automatically be excluded
         * from the source state. Defaults to true, can be cleared
         * to allow a state to re-enter itself
         */
        void setExcludeTarget(bool aExclude);

        /**
         * Get the target state after this transition is triggered.
         *
         * @return The state we end in, after this transition is triggered.
         */
        State* target() const;

        /**
         * Add a state in which this transition is eligible to fire
         *
         * @param aSource The state in which the transition is eligible to fire.
         */
        void addSourceState(State* aSource);

        /**
         * Specify the trigger condition associated with this state transition.
         *
         * @param aCondition The trigger condition for state transition.
         * @warning This function takes ownership (of?)
         */
        void setTriggerCondition(SGCondition* aCondition);

        /**
         * Add a binding that will fire upon this state transition.
         *
         * @param aBinding The binding to fire
         */
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

    /**
     * Initializes the state machine from a property list description, while handling initialization.
     *
     * @param desc The property list description for the state machine.
     * @param root The root node of the property list description.
     * @note Creating a state machine with this function does not require calling the `init()` function.
     * @throw sg_exception If the property list description is invalid or describes an impossible state machine.
     */
    void initFromPlist(SGPropertyNode* desc, SGPropertyNode* root);

    /**
     * Create a new state machine from a property list description, while handling initialization.
     *
     * @param desc The property list description for the state machine.
     * @param root The root node of the property list description.
     * @note Creating a state machine with this function does not require calling the `init()` function.
     * @throw sg_exception If the property list description is invalid or describes an impossible state machine.
     */
    static StateMachine* createFromPlist(SGPropertyNode* desc, SGPropertyNode* root);

    /**
     * @return The root property node for the state machine.
     */
    SGPropertyNode* root();

    /**
     * Initializes a state machine. If called when a state machine has already been initialized, it
     * returns immediately.
     *
     * @throw sg_range_exception If the state machine has been initialized with no states defined.
     */
    void init();

    /**
     * Shuts down the state machine.
     */
    void shutdown();

    /**
     * Update the state machine, triggering the next iteration.
     *
     * When the state machine updates, it evaluates inputs, checks for conditions that may trigger
     * a transition, and updates the state of the machine accordingly. Depending on the conditions,
     * it could maintain the current state, or transition to a new one.
     *
     * @param dt Time delta from the last state machine update (unused).
     */
    void update(double dt);

    /**
     * Gets the current state of the state machine
     *
     * @return The current state of the state machine.
     */
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

    /**
     * Search for and retrieve a state in the state machine by its name.
     *
     * @param stateName The name of the state to search for.
     * @return A pointer to the corresponding state if the state is found, or a default-constructed
     * `State_ptr` if no state is found.
     */
    State_ptr findStateByName(const std::string& stateName) const;

    /**
     * Get a state in the state machine given its index.
     *
     * @param aIndex The index of the state to retrieve.
     * @return A pointer to the corresponding state if the state is found.
     * @throw sg_range_exception If the index requested is out of bounds.
     */
    State_ptr stateByIndex(unsigned int aIndex) const;

    /**
     * Get the index of a given state in the state machine.
     *
     * @param aState A state to get the index of.
     * @return The index of the state if it exists, otherwise -1.
     */
    int indexOfState(State_ptr aState) const;

    /**
     * Programmatically create a new state for the state machine.
     *
     * @param aName Human-readable name for the state.
     * @return Pointer to the newly created state
     * @throw sg_range_exception If a state by the same name already exists
     * within the state machine.
     */
    State_ptr createState(const std::string& aName);

    /**
     * Programmatically create a new state transition for the state machine.
     *
     * @param aName Human-readable name for the state.
     * @param aTarget The target state for the state transition.
     * @return Pointer to the newly created state transition.
     */
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