/***************************************************************************
  tag: Peter Soetens  Mon May 10 19:10:29 CEST 2004  StateMachine.cxx

                        StateMachine.cxx -  description
                           -------------------
    begin                : Mon May 10 2004
    copyright            : (C) 2004 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/
#include "execution/StateMachine.hpp"
#include "boost/tuple/tuple.hpp"

#include <functional>

#include <assert.h>

namespace ORO_Execution
{
    using boost::tuples::get;

    StateMachine::StateMachine(StateMachine* parent, const std::string& name )
        : _parent (parent) , _name(name),
          initstate(0), finistate(0), current( 0 ), next(0), initc(0),
          currentProg(0), currentExit(0), currentHandle(0), currentEntry(0), error(false)
    {}

    bool StateMachine::requestInitialState()
    {
        // if we are inactive, don't do anything.
        if ( current == 0 || this->inTransition() )
            return false;
        // first check if we are in initstate, so this
        // even works if current == initstate == finistate,
        // which is legal.
        if ( current == initstate )
        {
            handleState( current );
            this->executePending();
            return true;
        }
        else if ( current == finistate )
        {
            leaveState( current );
            enterState( initstate );
            this->executePending();
            return true;
        }
        return false;
    }

    void StateMachine::requestFinalState()
    {
        // if we are inactive, don't do anything.
        if ( current == 0 || this->inTransition() )
            return;
        // If we are already in Final state, just handle again.
        if ( current == finistate )
        {
            handleState( current );
            this->executePending();
        }
        else
        {
            leaveState( current );
            enterState( finistate );
            this->executePending();
        }
    }

    StateInterface* StateMachine::requestNextState(bool stepping)
    {
        // bad idea, user, don't run this if we're not active...
        assert ( current != 0 );

        if ( this->inTransition() ) {
            return current; // can not accept request, still in transition
        }

        while ( reqstep != stateMap[ current ].end() ) {
            if ( get<0>(*reqstep)->evaluate() )
                if ( get<1>(*reqstep) == current )
                {
                    // execute the default action (handle current)
                    handleState( current );
                    // reset the reqstep iterator
                    reqstep = stateMap[ current ].begin();
                    break;
                }
                else
                {
                    // override default action :
                    leaveState(current);
                    enterState( get<1>(*reqstep) );
                    break;
                }
            ++reqstep;
            if ( stepping ) {
                break; // only do one evaluation.
            }
        }

        // check for wrapping
        if ( reqstep == stateMap[ current ].end() )
            reqstep = stateMap[ current ].begin();

        // execute as much as possible of exit, entry, handle()
        this->executePending( stepping );

        return current;
    }

    StateInterface* StateMachine::nextState()
    {
        // bad idea, user, don't run this if we're not active...
        assert ( current != 0 );
        TransList::iterator it1, it2;
        it1 = stateMap[ current ].begin();
        it2 = stateMap[ current ].end();

        for ( ; it1 != it2; ++it1 )
            if ( get<0>(*it1)->evaluate() )
                return get<1>(*it1);

        return current;
    }

    bool StateMachine::requestState( StateInterface * s_n )
    {
        // bad idea, user, don't run this if we're not active...
        assert ( current != 0 );

        if ( this->inTransition() ) {
            return false; // can not accept request, still in transition
        }

        // we may make transition to next state :

        // to current state
        if ( current == s_n )
        {
            handleState( current );
            this->executePending();
            return true;
        }

        // between 2 specific states
        TransList::iterator it1, it2;
        it1 = stateMap[ current ].begin();
        it2 = stateMap[ current ].end();

        for ( ; it1 != it2; ++it1 )
            if ( get<1>(*it1) == s_n
                 && get<0>(*it1)->evaluate() )
            {
                leaveState( current );
                enterState( s_n );
                // now try to do as much as possible
                this->executePending();
                // the request was accepted
                return true;
            }
        // the request has failed.
        return false;
    }

    int StateMachine::getLineNumber() const {
        // if not activated, return first line.
        // work around race condition that current[Prog] may become zero,
        // thus first store in local variable :
        StateInterface* statecopy = current;
        if ( statecopy == 0 )
            return 1;
        ProgramInterface* copy = currentProg;
        if ( copy )
            return copy->getLineNumber();
        if ( reqstep != stateMap.find(statecopy)->second.end() )
            return get<3>(*reqstep); // if valid, return it.

        // if none of the above, return entry point :
        return statecopy->getEntryPoint();
    }


    void StateMachine::transitionSet( StateInterface* from, StateInterface* to, ConditionInterface* cnd, int priority, int line )
    {
        TransList::iterator it;
        for ( it= stateMap[from].begin(); it != stateMap[from].end() && get<2>(*it) >= priority; ++it);
        stateMap[from].insert(it, boost::make_tuple( cnd, to, priority, line ) );
    }

    StateInterface* StateMachine::currentState() const
    {
        return current;
    }

    void StateMachine::leaveState( StateInterface* s )
    {
        currentExit = s->getExitProgram();
        if ( currentExit )
            currentExit->reset();
    }

    void StateMachine::handleState( StateInterface* s )
    {
        currentHandle = s->getHandleProgram();
        if ( currentHandle )
            currentHandle->reset();
    }

    void StateMachine::enterState( StateInterface* s )
    {
        // Before a state is entered, all transitions are reset !
        TransList::iterator it;
        for ( it= stateMap[s].begin(); it != stateMap[s].end(); ++it)
            get<0>(*it)->reset();

        next = s;
        currentEntry = s->getEntryProgram();
        if ( currentEntry )
            currentEntry->reset();
        currentHandle = s->getHandleProgram();
        if ( currentHandle )
            currentHandle->reset();
    }

    bool StateMachine::executePending( bool stepping )
    {
        // This function has great resposibility, since it acts like
        // a scheduler for pending requests. It tries to devise what to
        // do on basis of the contents of variables (like current*, next,...).
        // This is a somewhat
        // fragile implementation but requires very little bookkeeping.

        if ( error )
            return false;
        // if returns true : a transition (exit/entry) is done
        // and a new state may be requested.

        // first try exit
        if( currentExit ) {
            currentProg = currentExit;
            if ( (stepping && currentExit->executeStep() == false )
                 || (!stepping && currentExit->executeUntil() == false) ) {
                error = true;
                return false;
            }
            if ( currentExit->isFinished() ) {
                currentExit = 0;
                // move on
                current = next;
                if ( current )
                    reqstep = stateMap[ current ].begin();
                else {
                    return true; // done if current == 0 !
                }
            }
            else
                return false; // transition in progress
        } else // if no currentExit, go to the next (or same) state right away.
            current = next;
        
        // if exit done :
        if( currentEntry ) {
            currentProg = currentEntry;
            if ( (stepping && currentEntry->executeStep() == false )
                 || (!stepping && currentEntry->executeUntil() == false) ) {
                error = true;
                return false;
            }
            if ( currentEntry->isFinished() )
                currentEntry = 0;
            else 
                return false; // transition in progress
        }
        // handle current :
        if( currentHandle ) {
            currentProg = currentHandle;
            if ( (stepping && currentHandle->executeStep() == false )
                 || (!stepping && currentHandle->executeUntil() == false) ) {
                error = true;
                return false;
            }
            if ( currentHandle->isFinished() )
                currentHandle = 0;
            else
                return false; 
        }
        // only reach here if currentHandle == 0
        currentProg = 0;
        return true; // all pending is done
    }

    bool StateMachine::inTransition() {
        return currentProg != 0;
    }

    void StateMachine::setInitialState( StateInterface* s )
    {
        initstate = s;
    }

    void StateMachine::setFinalState( StateInterface* s )
    {
        finistate = s;
    }

    bool StateMachine::activate()
    {
        if ( current == 0 )
        {
            current = getInitialState();
            if ( initc )
                initc->execute();
            enterState( current );
            reqstep = stateMap[ current ].begin();
            next = current;
            this->executePending();
            return true;
        }
        return false;
    }

    bool StateMachine::deactivate()
    {
        if ( current != 0 && next != 0 )
        {
            // whatever state we are in, leave it.
            leaveState( current );
            // do not call enterState( 0 )
            currentEntry = 0;
            currentHandle  = 0;
            next = 0;
            if ( initc )
                initc->reset();
            // this will execute the exitFunction (if any) and, if successfull,
            // set current to zero (using next).
            this->executePending();
            return true;
        }
        return false;
    }
}

