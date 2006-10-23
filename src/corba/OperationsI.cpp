// -*- C++ -*-
//
// $Id$

// ****  Code generated by the The ACE ORB (TAO) IDL Compiler ****
// TAO and the TAO IDL Compiler have been developed by:
//       Center for Distributed Object Computing
//       Washington University
//       St. Louis, MO
//       USA
//       http://www.cs.wustl.edu/~schmidt/doc-center.html
// and
//       Distributed Object Computing Laboratory
//       University of California at Irvine
//       Irvine, CA
//       USA
//       http://doc.ece.uci.edu/
// and
//       Institute for Software Integrated Systems
//       Vanderbilt University
//       Nashville, TN
//       USA
//       http://www.isis.vanderbilt.edu/
//
// Information about TAO is available at:
//     http://www.cs.wustl.edu/~schmidt/TAO.html

// TAO_IDL - Generated from 
// be/be_codegen.cpp:910

#include "corba/OperationsI.h"
#include "CommandC.hpp"
#include "DataSourceCondition.hpp"
#include "CommandDataSource.hpp"
#include "Logger.hpp"


using namespace RTT;
using namespace RTT::Corba;


// Implementation skeleton constructor
Orocos_Action_i::Orocos_Action_i (MethodC* orig, CommandInterface* com, PortableServer::POA_ptr the_poa )
    : morig(*orig), mcom(com), mpoa( PortableServer::POA::_duplicate(the_poa) )
{
}

// Implementation skeleton destructor
Orocos_Action_i::~Orocos_Action_i (void)
{
    delete mcom;
}

CORBA::Boolean Orocos_Action_i::execute (
    
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
    return mcom->execute();
}

CORBA::Boolean Orocos_Action_i::executeAny (
      const ::RTT::Corba::AnyArguments& args
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ,::RTT::Corba::WrongNumbArgException
    ,::RTT::Corba::WrongTypeArgException
	  )) {
      RTT::MethodC mgen = morig;
    try {
        for (size_t i =0; i != args.length(); ++i)
            mgen.arg( RTT::DataSourceBase::shared_ptr( new RTT::ValueDataSource<CORBA::Any_var>( new CORBA::Any( args[i] ) )));
        // if not ready, not enough args were given, *guess* a one off error in the exception :-(
        if ( !mgen.ready() )
            throw ::RTT::Corba::WrongNumbArgException( args.length()+1, args.length() );
        delete mcom;
        if ( dynamic_cast< DataSource<bool>* >(mgen.getDataSource().get() ) )
            mcom = new CommandDataSourceBool( dynamic_cast<DataSource<bool>*>(mgen.getDataSource().get() ));
        else
            mcom = new CommandDataSource( mgen.getDataSource() );
        return this->execute();
    } catch ( RTT::wrong_number_of_args_exception& wna ) {
        throw ::RTT::Corba::WrongNumbArgException( wna.wanted, wna.received );
    } catch ( RTT::wrong_types_of_args_exception& wta ) {
        throw ::RTT::Corba::WrongTypeArgException( wta.whicharg, wta.expected_.c_str(), wta.received_.c_str() );
    }
    return 0;
  }


void Orocos_Action_i::reset (
    
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
    mcom->reset();
}

// Implementation skeleton constructor
Orocos_Command_i::Orocos_Command_i ( CommandC& orig, CommandC& comm, PortableServer::POA_ptr the_poa)
    : morig( new CommandC(orig) ), mcomm( new CommandC(comm)), mpoa( PortableServer::POA::_duplicate(the_poa) )
{
}

// Implementation skeleton destructor
Orocos_Command_i::~Orocos_Command_i (void)
{
    delete morig;
    delete mcomm;
}

CORBA::Boolean Orocos_Command_i::execute (
    
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
    //Logger::In in("Orocos_Command_i");
    //Logger::log() <<Logger::Debug << "Executing CommandC."<<Logger::endl;
    return morig->execute();
}

CORBA::Boolean Orocos_Command_i::executeAny (
      const ::RTT::Corba::AnyArguments& args
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ,::RTT::Corba::WrongNumbArgException
    ,::RTT::Corba::WrongTypeArgException
	  )) {
      mcomm = morig;
    try {
        for (size_t i =0; i != args.length(); ++i)
            mcomm->arg( DataSourceBase::shared_ptr( new ValueDataSource<CORBA::Any_var>( new CORBA::Any( args[i] ) )));
        // if not ready, not enough args were given, *guess* a one off error in the exception :-(
        if ( !mcomm->ready() )
            throw ::RTT::Corba::WrongNumbArgException( args.length()+1, args.length() );
        return this->execute();
    } catch ( wrong_number_of_args_exception& wna ) {
        throw ::RTT::Corba::WrongNumbArgException( wna.wanted, wna.received );
    } catch (wrong_types_of_args_exception& wta ) {
        throw ::RTT::Corba::WrongTypeArgException( wta.whicharg, wta.expected_.c_str(), wta.received_.c_str() );
    }
    return 0;
  }


CORBA::Boolean Orocos_Command_i::done (
    
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
    //Logger::In in("Orocos_Command_i");
    //Logger::log() <<Logger::Debug << "Evaluating CommandC:"<<morig->done()<<Logger::endl;
    return morig->done();
}

CORBA::Boolean Orocos_Command_i::executed (
    
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
    return morig->executed();
}

CORBA::Boolean Orocos_Command_i::sent (
    
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
    return morig->sent();
}

CORBA::Boolean Orocos_Command_i::accepted (
    
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
    return morig->accepted();
}

CORBA::Boolean Orocos_Command_i::valid (
    
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
    return morig->valid();
}

void Orocos_Command_i::reset (
    
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here
    return morig->reset();
}

void Orocos_Command_i::destroyCommand (
    
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  // Add your implementation here

}




