/***************************************************************************
  tag: Peter Soetens  Wed Jan 18 14:09:49 CET 2006  ControlTaskProxy.cxx 

                        ControlTaskProxy.cxx -  description
                           -------------------
    begin                : Wed January 18 2006
    copyright            : (C) 2006 Peter Soetens
    email                : peter.soetens@fmtc.be
 
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
 
 


#include "corba/ControlTaskProxy.hpp"
#include "corba/ControlTaskServer.hpp"
#include "corba/ControlTaskC.h"
#include "corba/CorbaMethodFactory.hpp"
#include "corba/CorbaCommandFactory.hpp"
#include "corba/CORBAExpression.hpp"
#include "corba/ScriptingAccessProxy.hpp"
#include "corba/CorbaPort.hpp"

#include "CommandInterface.hpp"
#include "Types.hpp"
#include "orbsvcs/CosNamingC.h"
#include <iostream>

#include <ace/String_Base.h>

using namespace std;

namespace RTT
{namespace Corba
{
    IllegalServer::IllegalServer() : reason("This server does not exist or has the wrong type.") {}

    IllegalServer::~IllegalServer() throw() {}

    const char* IllegalServer::what() const throw() { return reason.c_str(); }


    std::map<Corba::ControlTask_ptr, ControlTaskProxy*> ControlTaskProxy::proxies;

    PortableServer::POA_var ControlTaskProxy::proxy_poa;

    ControlTaskProxy::~ControlTaskProxy()
    {
        if ( this->attributes()->properties() ) {
            flattenPropertyBag( *this->attributes()->properties() );
            deleteProperties( *this->attributes()->properties() );
        }
        this->attributes()->clear();
    }

    ControlTaskProxy::ControlTaskProxy(std::string name, bool is_ior) 
        : TaskContext("NotFound") 
    {
        try {
            if (is_ior) {
                // Use the first argument to create the task object reference,
                // in real applications we use the naming service, but let's do
                // the easy part first!
                CORBA::Object_var task_object =
                    orb->string_to_object ( name.c_str() );

                // Now downcast the object reference to the appropriate type
                mtask = Corba::ControlTask::_narrow (task_object.in ());
            } else {
                // NameService
                CORBA::Object_var rootObj = orb->resolve_initial_references("NameService");
                CosNaming::NamingContext_var rootContext = CosNaming::NamingContext::_narrow(rootObj.in());
                if (CORBA::is_nil(rootContext.in() )) {
                    log(Error) << "ControlTaskProxy could not acquire NameService."<<endlog();
                    throw IllegalServer();
                }
                Logger::log() <<Logger::Debug << "ControlTaskProxy found CORBA NameService."<<endlog();
                CosNaming::Name serverName;
                serverName.length(2);
                serverName[0].id = CORBA::string_dup("ControlTasks");
                serverName[1].id = CORBA::string_dup( name.c_str() );

                // Get object reference
                CORBA::Object_var task_object = rootContext->resolve(serverName);
                mtask = Corba::ControlTask::_narrow (task_object.in ());
            }
            if ( CORBA::is_nil( mtask.in() ) ) {
                Logger::log() << Logger::Error << "Failed to acquire ControlTaskServer '"+name+"'."<<endlog();
                throw IllegalServer();
            }
            CORBA::String_var nm = mtask->getName(); // force connect to object.
            std::string newname( nm.in() );
            this->mtask_name = newname;
            Logger::log() << Logger::Info << "Successfully connected to ControlTaskServer '"+newname+"'."<<endlog();
            proxies[mtask] = this;
        }
        catch (CORBA::Exception &e) {
            log(Error)<< "CORBA exception raised when resolving Object !" << endlog();
            Logger::log() << e._info().c_str() << endlog();
        }
        catch (...) {
            log(Error) <<"Unknown Exception in ControlTaskProxy construction!"<<endlog();
            throw;
        }

        this->synchronize();
    }

    ControlTaskProxy::ControlTaskProxy( ::RTT::Corba::ControlTask_ptr taskc) 
        : TaskContext("CORBAProxy"), mtask( taskc )
    {
        try {
            CORBA::String_var nm = mtask->getName(); // force connect to object.
            std::string name( nm.in() );
            this->mtask_name = name;
            proxies[mtask] = this;
        }
        catch (CORBA::Exception &e) {
            log(Error) << "CORBA exception raised when creating ControlTaskProxy!" << Logger::nl;
            Logger::log() << e._info().c_str() << endlog();
        }
        catch (...) {
            throw;
        }
        this->synchronize();
    }

    void ControlTaskProxy::synchronize()
    {
        if (!mtask)
            return;

        // load command and method factories.
        // methods:
        log(Debug) << "Fetching Methods."<<endlog();
        MethodInterface_var mfact = mtask->methods();
        if (mfact) {
            MethodList_var objs;
            objs = mfact->getMethods();
            for ( size_t i=0; i < objs->length(); ++i) {
                this->methods()->add( objs[i].in(), new CorbaMethodFactory( objs[i].in(), mfact.in(), ProxyPOA() ) );
            }
        }
        // commands:
        log(Debug) << "Fetching Commands."<<endlog();
        CommandInterface_var cfact = mtask->commands();
        if (cfact) {
            CommandList_var objs;
            objs = cfact->getCommands();
            for ( size_t i=0; i < objs->length(); ++i) {
                this->commands()->add( objs[i].in(), new CorbaCommandFactory( objs[i].in(), cfact.in(), ProxyPOA() ) );
            }
        }

        // first do properties:
      log(Debug) << "Fetching Properties."<<endlog();
        AttributeInterface::PropertyNames_var props = mtask->attributes()->getPropertyList();

        for (size_t i=0; i != props->length(); ++i) {
            if ( this->attributes()->hasProperty( string(props[i].name.in()) ) )
                continue; // previously added.
            Expression_var expr = mtask->attributes()->getProperty( props[i].name.in() );
            if ( CORBA::is_nil( expr ) ) {
                log(Error) <<"Property "<< string(props[i].name.in()) << " present in getPropertyList() but not accessible."<<endlog();
                continue; 
            }
#if 0 // This code may trigger endless recurse if server has recursive prop bags.
      // By using Property<PropertyBag>::narrow( ... ) this is no longer needed.
            // See if it is a PropertyBag:
            CORBA::Any_var any = expr->get();
            PropertyBag bag;
            if ( AnyConversion<PropertyBag>::update( *any, bag ) ) {
                Property<PropertyBag>* pbag = new Property<PropertyBag>( string(props[i].name.in()), string(props[i].description.in()), bag);
                this->attributes()->addProperty( pbag );
                continue;
            }
#endif
            AssignableExpression_var as_expr = AssignableExpression::_narrow( expr.in() );
            // addProperty also adds as attribute...
            if ( CORBA::is_nil( as_expr ) ) {
                log(Error) <<"Property "<< string(props[i].name.in()) << " was not writable !"<<endlog();
            } else {
                // If the type is known, immediately build the correct property and datasource,
                // otherwise, build a property of CORBA::Any.
                CORBA::String_var tn = as_expr->getTypeName();
                TypeInfo* ti = TypeInfoRepository::Instance()->type( tn.in() );
                Logger::log() <<Logger::Info << "Looking up Property " << tn.in();
                if ( ti ) {
                    this->attributes()->addProperty( ti->buildProperty( props[i].name.in(), props[i].description.in(), 
                                                                        ti->buildCorbaProxy( as_expr.in() ) ) );
                    Logger::log() <<Logger::Info <<" found!"<<endlog();
                }
                else {
                    this->attributes()->addProperty( new Property<CORBA::Any_ptr>( string(props[i].name.in()), string(props[i].description.in()), new CORBAAssignableExpression<Property<CORBA::Any_ptr>::DataSourceType>( as_expr.in() ) ) );
                    Logger::log()  <<Logger::Info<<" not found :-("<<endlog();
                }
            }
        }

      log(Debug) << "Fetching Attributes."<<endlog();
        // add attributes not yet added by properties:
        AttributeInterface::AttributeNames_var attrs = mtask->attributes()->getAttributeList();
        
        for (size_t i=0; i != attrs->length(); ++i) {
            if ( this->attributes()->hasAttribute( string(attrs[i].in()) ) )
                continue; // previously added.
            Expression_var expr = mtask->attributes()->getAttribute( attrs[i].in() );
            if ( CORBA::is_nil( expr ) ) {
                log(Error) <<"Attribute "<< string(attrs[i].in()) << " present in getAttributeList() but not accessible."<<endlog();
                continue; 
            }
            AssignableExpression_var as_expr = AssignableExpression::_narrow( expr.in()  );
            // If the type is known, immediately build the correct attribute and datasource,
            // otherwise, build a attribute of CORBA::Any.
            CORBA::String_var tn = expr->getTypeName();
            TypeInfo* ti = TypeInfoRepository::Instance()->type( tn.in() );
            log(Info) << "Looking up Attribute " << tn.in();
            if ( ti ) {
                Logger::log() <<": found!"<<endlog();
                if ( CORBA::is_nil( as_expr ) ) {
                    this->attributes()->setValue( ti->buildConstant( attrs[i].in(), ti->buildCorbaProxy( expr.in() ) ) );
                }
                else {
                    this->attributes()->setValue( ti->buildAttribute( attrs[i].in(), ti->buildCorbaProxy( as_expr.in() ) ) );
                }
            } else {
                Logger::log() <<": not found :-("<<endlog();
                if ( CORBA::is_nil( as_expr ) )
                    this->attributes()->setValue( new Constant<CORBA::Any_ptr>( attrs[i].in(), new CORBAExpression<CORBA::Any_ptr>( expr.in() ) ) );
                else
                this->attributes()->setValue( new Attribute<CORBA::Any_ptr>( attrs[i].in(), new CORBAAssignableExpression<CORBA::Any_ptr>( as_expr.in() ) ) );
            }
        }

        log(Debug) << "Fetching ScriptingAccess."<<endlog();
        Corba::ScriptingAccess_var saC = mtask->scripting();
        if ( saC ) {
            delete mscriptAcc;
            mscriptAcc = new ScriptingAccessProxy( saC.in() );
        }

        log(Debug) << "Fetching Ports."<<endlog();
        DataFlowInterface_var dfact = mtask->ports();
        if (dfact) {
            DataFlowInterface::PortNames_var objs;
            objs = dfact->getPorts();
            for ( size_t i=0; i < objs->length(); ++i) {
                this->ports()->addPort( new CorbaPort( objs[i].in(), dfact.in(), ProxyPOA() ) );
            }
        }

        log(Debug) << "Fetching Objects:"<<endlog();
        Corba::ObjectList_var plist = mtask->getObjectList();

        for( size_t i =0; i != plist->length(); ++i) {
            if ( string( plist[i] ) == "this")
                continue;
            ControlObject_var cobj = mtask->getObject(plist[i]);
            CORBA::String_var descr = cobj->getDescription();
            TaskObject* tobj = new TaskObject( std::string(plist[i]), std::string(descr.in()) );
            
            // methods:
            log(Info) << plist[i] << ": fetching Methods."<<endlog();
            MethodInterface_var mfact = cobj->methods();
            if (mfact) {
                MethodList_var objs;
                objs = mfact->getMethods();
                for ( size_t i=0; i < objs->length(); ++i) {
                    tobj->methods()->add( objs[i].in(), new CorbaMethodFactory( objs[i].in(), mfact.in(), ProxyPOA() ) );
                }
            }
            // commands:
            log(Info) << plist[i] << ": fetching Commands."<<endlog();
            CommandInterface_var cfact = cobj->commands();
            if (cfact) {
                CommandList_var objs;
                objs = cfact->getCommands();
                for ( size_t i=0; i < objs->length(); ++i) {
                    tobj->commands()->add( objs[i].in(), new CorbaCommandFactory( objs[i].in(), cfact.in(), ProxyPOA() ) );
                }
            }
            this->addObject( tobj );
        }

        log(Debug) << "All Done."<<endlog();
    }

    bool ControlTaskProxy::InitOrb(int argc, char* argv[] ) {
        if ( orb.in() )
            return false;

        try {
            // First initialize the ORB, that will remove some arguments...
            orb =
                CORBA::ORB_init (argc, const_cast<char**>(argv),
                                 "" /* the ORB name, it can be anything! */);
            // Also activate the POA Manager, since we may get call-backs !
            CORBA::Object_var poa_object =
                orb->resolve_initial_references ("RootPOA");
            rootPOA =
                PortableServer::POA::_narrow (poa_object.in ());
            PortableServer::POAManager_var poa_manager =
                rootPOA->the_POAManager ();
            poa_manager->activate ();

            return true;
        }
        catch (CORBA::Exception &e) {
            log(Error) << "Orb Init : CORBA exception raised!" << Logger::nl;
            Logger::log() << e._info().c_str() << endlog();
        }
        return false;
    }

    void ControlTaskProxy::DestroyOrb()
    {
        try {
            // Destroy the POA, waiting until the destruction terminates
            //poa->destroy (1, 1);
            orb->destroy();
            std::cerr <<"Orb destroyed."<<std::endl;
        }
        catch (CORBA::Exception &e) {
            log(Error) << "Orb Init : CORBA exception raised!" << Logger::nl;
            Logger::log() << e._info().c_str() << endlog();
        }
    }


    ControlTaskProxy* ControlTaskProxy::Create(std::string name, bool is_ior /*=false*/) {
        if ( CORBA::is_nil(orb) || name.empty() )
            return 0;

        // create new:
        try {
            ControlTaskProxy* ctp = new ControlTaskProxy( name, is_ior );
            return ctp;
        }
        catch( IllegalServer& is ) {
            cerr << is.what() << endl;
        }
        return 0;
    }

    ControlTaskProxy* ControlTaskProxy::Create(::RTT::Corba::ControlTask_ptr t) {
        if ( CORBA::is_nil(orb) || t == 0 )
            return 0;

        // proxy present for this object ?
        // is_equivalent is actually our best try.
        for (PMap::iterator it = proxies.begin(); it != proxies.end(); ++it)
            if ( (it->first)->_is_equivalent( t ) )
                 return proxies[t];

        // create new:
        try {
            ControlTaskProxy* ctp = new ControlTaskProxy( t );
            return ctp;
        }
        catch( IllegalServer& is ) {
            cerr << is.what() << endl;
        }
        return 0;
    }

    bool ControlTaskProxy::executeCommand( CommandInterface* c)
    {
        return false;
    }

    int ControlTaskProxy::queueCommand( CommandInterface* c)
    {
        return 0;
    }

    void ControlTaskProxy::setName(const std::string& n)
    {
        //mtask->setName( n.c_str() );
    }

    bool ControlTaskProxy::addPeer( TaskContext* peer, std::string alias /*= ""*/ )
    {
        if (!mtask)
            return false;

        // if peer is a proxy, add the proxy, otherwise, create new server.
        ControlTaskProxy* ctp = dynamic_cast<ControlTaskProxy*>( peer );
        if (ctp) {
            if ( mtask->addPeer( ctp->server(), alias.c_str() ) ) {
                this->synchronize();
                return true;
            }
            return false;
        }
        // no server yet, create it.
        ControlTaskServer* newpeer = ControlTaskServer::Create(peer);
        if ( mtask->addPeer( newpeer->server(), alias.c_str() ) ) {
            this->synchronize();
            return true;
        }
        return false;
    }

    void ControlTaskProxy::removePeer( const std::string& name )
    {
        if (!mtask)
            return;
        mtask->removePeer( name.c_str() );
    }

    void ControlTaskProxy::removePeer( TaskContext* peer )
    {
        if (!mtask)
            return;
        mtask->removePeer( peer->getName().c_str() );
    }

    bool ControlTaskProxy::connectPeers( TaskContext* peer )
    {
        if (!mtask)
            return false;
        ControlTaskServer* newpeer = ControlTaskServer::Create(peer);
        return mtask->connectPeers( newpeer->server() );
    }

    void ControlTaskProxy::disconnectPeers( const std::string& name )
    {
        if (mtask)
            mtask->disconnectPeers( name.c_str() );
    }

    TaskContext::PeerList ControlTaskProxy::getPeerList() const
    {
        Corba::ControlTask::ControlTaskNames_var plist = mtask->getPeerList();
        TaskContext::PeerList vlist;
        for( size_t i =0; i != plist->length(); ++i)
            vlist.push_back( std::string( plist[i] ) );
        return vlist;
    }

    bool ControlTaskProxy::hasPeer( const std::string& peer_name ) const
    {
        return mtask && mtask->hasPeer( peer_name.c_str() );
    }

    TaskContext* ControlTaskProxy::getPeer(const std::string& peer_name ) const
    {
        if ( !mtask )
            return 0;
        Corba::ControlTask_ptr ct = mtask->getPeer( peer_name.c_str() );
        if ( !ct )
            return 0;
        return ControlTaskProxy::Create( ct );
    }

    Corba::ControlTask_ptr ControlTaskProxy::server() const {
        if ( !mtask )
            return 0;
        return Corba::ControlTask::_duplicate(mtask);
    }

    CosPropertyService::PropertySet_ptr ControlTaskProxy::propertySet() {
        if ( !mtask )
            return 0;
        return mtask->propertySet();
    }

    PortableServer::POA_ptr ControlTaskProxy::ProxyPOA() {
        if ( !orb.in() )
            return 0;
        if (!proxy_poa.in() ) {
            CORBA::Object_var poa_object =
                orb->resolve_initial_references ("RootPOA");

            // new POA for the proxies:
            // Use default manager, is already activated !
            //PortableServer::POAManager_var proxy_manager = poa->the_POAManager ();
            //CORBA::PolicyList pol;
            //proxy_poa = poa->create_POA( "ProxyPOA", proxy_manager, pol );
            proxy_poa =
                PortableServer::POA::_narrow (poa_object.in ());
        }
        // note: do not use _retn(): user must duplicate in constructor.
        return proxy_poa.in();
    }
}}

