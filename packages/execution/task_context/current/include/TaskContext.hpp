/***************************************************************************
  tag: Peter Soetens  Tue Dec 21 22:43:08 CET 2004  TaskContext.hpp 

                        TaskContext.hpp -  description
                           -------------------
    begin                : Tue December 21 2004
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
 
 
#ifndef ORO_TASK_CONTEXT_HPP
#define ORO_TASK_CONTEXT_HPP

#include "AttributeRepository.hpp"

#include "pkgconf/system.h"
#ifdef OROPKG_CORELIB_EVENTS
#include "EventService.hpp"
#endif

#include "DataFlowInterface.hpp"
#include "ExecutionEngine.hpp"
#include "ScriptingAccess.hpp"
#include "TaskCore.hpp"
#include "PropertyBag.hpp"

#include <string>
#include <map>

namespace RTT
{
    class CommandProcessor;
    class ScriptingAccess;
    class TaskObject;
    class EventService;

    /**
     * A TaskContext exports the commands, methods, events, properties and ports
     * a task has. Furthermore, it allows to visit its peer tasks.
     *
     * @par TaskContext interface
     * When a command is exported, one can access it using commands(). A similar
     * mechanism is available for properties(), methods(), events() and ports().
     * The commands of this TaskContext are executed by its
     * ExecutionEngine.
     *
     * @par Executing a TaskContext
     * In order to run the ExecutionEngine, the ExecutionEngine must
     * be invoked from an ActivityInterface implementation. As long as
     * there is no activity or the activity is not started, this
     * TaskContext will not accept any commands, nor process events,
     * nor execute programs or state machines.  In this way, the user
     * of this class can determine himself at which point and at which
     * moment commands and programs can be executed.
     *
     * @par Connecting TaskContexts
     * TaskContexts are connected using the unidirectional addPeer() or bidirectional
     * connectPeers() methods. These methods setup data connections and allow
     * 'peer' TaskContexts to use each other's interface.
     * In order to disconnect this task from its peers, use disconnect(), which
     * will disconnect all the Data Flow Ports and remove this object from its
     * Peers.
     */
    class TaskContext
        : public OperationInterface,
          public TaskCore
    {
        // non copyable
        TaskContext( TaskContext& );
    protected:
        std::string mdescription;
    
        typedef std::map< std::string, TaskContext* > PeerMap;
        typedef std::vector< TaskContext* > Users;
        typedef std::vector< OperationInterface* > Objects;
        /// map of the tasks we are using
        PeerMap         _task_map;
        /// map of the tasks that are using us.
        Users         musers;
        /// the TaskObjects.
        Objects mobjects;

        ScriptingAccess* mscriptAcc;

        void exportPorts();

        /**
         * Inform this TaskContext that \a user is using
         * our services.
         */
        void addUser(TaskContext* user);

        /**
         * Inform this TaskContext that \a user is no longer using
         * our services.
         */
        void removeUser(TaskContext* user);
    public:
        typedef std::vector< std::string > PeerList;
        typedef std::vector< std::string > ObjectList;

        /**
         * Create a TaskContext visible with \a name.
         * It's ExecutionEngine will be newly constructed with private 
         * ExecutionEngine processing its commands, events,
         * programs and state machines.
         */
        TaskContext( const std::string& name );

        /**
         * Create a TaskContext visible with \a name. Its commands
         * programs and state machines are processed by \a parent.
         * Use this constructor to share execution engines among task contexts, such that
         * the execution of their functionality is serialised (executed in the same thread).
         */
        TaskContext(const std::string& name, ExecutionEngine* parent );

        virtual ~TaskContext();

        virtual const std::string& getName() const;

        virtual const std::string& getDescription() const;

        virtual void setDescription(const std::string& descr);

        /**
         * Queue a command.
         * @return True if the Processor accepted the command.
         */
        bool executeCommand( CommandInterface* c);

        /**
         * Queue a command. If the Processor is not running or not accepting commands
         * this will fail and return zero.
         * @return The command id the processor returned.
         */
        int queueCommand( CommandInterface* c);

        /**
         * Add a one-way connection from this task to a peer task.
         * @param peer The peer to add.
         * @param alias An optional alias (another name) for the peer.
         * defaults to \a peer->getName()
         */
        virtual bool addPeer( TaskContext* peer, std::string alias = "" );

        /**
         * Remove a one-way connection from this task to a peer task.
         */
        virtual void removePeer( const std::string& name );

        /**
         * Remove a one-way connection from this task to a peer task.
         */
        virtual void removePeer( TaskContext* peer );

        /**
         * Add a two-way connection from this task to a peer task.
         */
        virtual bool connectPeers( TaskContext* peer );

        /**
         * Add a data flow connection from this task's ports to a peer's ports.
         */
        virtual bool connectPorts( TaskContext* peer );

        /**
         * Disconnect this TaskContext from it's peers.
         * All its Data Flow Ports are disconnected from the connections but
         * the connections themselves may continue to exist to serve other TaskContexts.
         * This method invokes removePeer() as well on the peers listed in this->getPeerList().
         */
        virtual void disconnect();

        /**
         * Reconnect the data ports of this task, without
         * removing the peer relationship. Use this if you
         * changed a port name of an already connected task.
         */
        virtual void reconnect();

        /**
         * Remove a two-way connection from this task to a peer task.
         */
        virtual void disconnectPeers( const std::string& name );

        /**
         * Return a standard container which contains all the Peer names
         * of this TaskContext.
         */
        virtual PeerList getPeerList() const;

        /**
         * Return true if it knows a peer by that name.
         */
        virtual bool hasPeer( const std::string& peer_name ) const;

        /**
         * Get a pointer to a peer of this task.
         * @return null if no such peer.
         */
        virtual TaskContext* getPeer(const std::string& peer_name ) const;

        /** 
         * Add a new TaskObject to this TaskContext.
         * 
         * @param obj This object becomes owned by this TaskContext.
         * 
         * @return true if it cuold be added, false if such
         * object already exists.
         */
        virtual bool addObject( OperationInterface *obj );

        /** 
         * Get a pointer to a previously added TaskObject
         * 
         * @param obj_name The name of the TaskObject
         * 
         * @return the pointer
         */
        virtual OperationInterface* getObject(const std::string& obj_name );

        /** 
         * Get a list of all the object names of this TaskContext.
         * 
         * @return a list of string names.
         */
        virtual ObjectList getObjectList() const;

        /** 
         * Remove and delete a previously added TaskObject.
         * 
         * @param obj_name The name of the TaskObject
         * 
         * @return true if found and removed, false otherwise.
         */
        virtual bool removeObject(const std::string& obj_name );

        /**
         * Get access to high level controls for
         * programs, state machines and scripting
         * statements.
         */
        ScriptingAccess* scripting()
        {
            return mscriptAcc;
        }

        /**
         * Get access to high level controls for
         * programs, state machines and scripting
         * statements.
         */
        const ScriptingAccess* scripting() const
        {
            return mscriptAcc;
        }

        /**
         * The command interface of this task context.
         */
        CommandRepository* commands()
        {
            return &comms;
        }

        /**
         * The command interface of this task context.
         */
        const CommandRepository* commands() const
        {
            return &comms;
        }

        /**
         * The method interface of this task context.
         */
        MethodRepository* methods()
        {
            return &meths;
        }

        /**
         * The method interface of this task context.
         */
        const MethodRepository* methods() const
        {
            return &meths;
        }

        /**
         * The task-local values ( attributes and properties ) of this TaskContext.
         */
        AttributeRepository* attributes() {
            return &attributeRepository;
        }

        /**
         * The task-local values ( attributes and properties ) of this TaskContext.
         */
        const AttributeRepository* attributes() const {
            return &attributeRepository;
        }

        /**
         * The properties of this TaskContext.
         */
        PropertyBag* properties() {
            return attributeRepository.properties();
        }

        /**
         * The properties of this TaskContext.
         */
        const PropertyBag* properties() const {
            return attributeRepository.properties();
        }

        /**
         * The task-local events ( 'signals' ) of this TaskContext.
         */
        EventService* events() {
#ifdef OROPKG_EXECUTION_ENGINE_EVENTS
            return &eventService;
#else
            return 0;
#endif
        }

        /**
         * The task-local events ( 'signals' ) of this TaskContext.
         */
        const EventService* events() const {
#ifdef OROPKG_EXECUTION_ENGINE_EVENTS
            return &eventService;
#else
            return 0;
#endif
        }

        /**
         * Get the Data flow ports of this task.
         */
        DataFlowInterface* ports() {
            return &dataPorts;
        }
        
        /**
         * Get the Data flow ports of this task.
         */
        const DataFlowInterface* ports() const {
            return &dataPorts;
        }

    private:
        /**
         * The task-local values ( attributes ) of this TaskContext.
         */
        AttributeRepository     attributeRepository;

#ifdef OROPKG_EXECUTION_ENGINE_EVENTS
        /**
         * The task-local events ( 'signals' ) of this TaskContext.
         */
        EventService            eventService;
#endif

        /**
         * The task-local ports.
         */
        DataFlowInterface dataPorts;

        CommandRepository comms;

        MethodRepository meths;
    };

    /**
     * Connect the Data Flow Ports of A and B in both
     * directions, by matching port names.
     * @see TaskContext::connectPorts
     */
    bool connectPorts(TaskContext* A, TaskContext* B);

    /**
     * Set up the Execution Flow (who knows who)
     * between A and B in both directions. Both will be able to 
     * use each other's interface.
     * @see TaskContext::connectPeers
     */
    bool connectPeers(TaskContext* A, TaskContext* B);
}

#endif
