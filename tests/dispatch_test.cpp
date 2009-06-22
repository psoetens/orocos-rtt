/***************************************************************************
  tag: Peter Soetens  Mon Jan 10 15:59:51 CET 2005  dispatch_test.cpp

                        dispatch_test.cpp -  description
                           -------------------
    begin                : Mon January 10 2005
    copyright            : (C) 2005 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/



#include "dispatch_test.hpp"
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <FunctionGraph.hpp>
#include <SimulationThread.hpp>
#include <Method.hpp>
#include <Command.hpp>
#include <TaskObject.hpp>

using namespace std;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( DispatchTest );

DispatchTest::DispatchTest()
    : gtc("root"),
      mtc("space"),
      ltc("subspace"),
      gtask(0.1, gtc.engine() ),
      mtask(0.05, mtc.engine()),
      ltask(0.01, ltc.engine())
{}


void
DispatchTest::setUp()
{
    ltc.clear();
    mtc.clear();
    gtc.clear();
    // ltc has a test object
    ltc.addObject( this->createObject("test", ltc.engine()->commands()) );
    // mtc has two methods.
    mtc.addObject( this->createObject("test", mtc.engine()->commands()) );

    gtc.addPeer( &mtc );
    mtc.connectPeers( &ltc );

}


void
DispatchTest::tearDown()
{
    gtc.removePeer( "space" );
    ltc.disconnectPeers( "subspace" );
}


bool DispatchTest::assertBool( bool b) {
    return b;
}
bool DispatchTest::assertMsg( bool b, const std::string& msg) {
    if ( b == false )
        cout << "Asserted :" << b << " with :" << msg << endl;
    return b;
}


TaskObject* DispatchTest::createObject(string a, CommandProcessor* cp)
{
    TaskObject* dat = new TaskObject(a);
    dat->methods()->addMethod( method( "assert", &DispatchTest::assertBool, this), "Assert", "bool", "" );
    dat->methods()->addMethod( method( "assertMsg", &DispatchTest::assertMsg, this), "Assert message", "bool", "", "text", "text"  );
    dat->methods()->addMethod( method( "isTrue", &DispatchTest::assertBool, this), "Identity function", "bool", "" );
    dat->commands()->addCommand( command( "instantDone", &DispatchTest::true_genCom,
                                      &DispatchTest::true_gen, this, cp),
                                      "returns immediately" );
    dat->commands()->addCommand( command( "neverDone", &DispatchTest::true_genCom,
                                    &DispatchTest::false_gen, this, cp),
                                    "returns never" );
    dat->commands()->addCommand( command( "instantNotDone", &DispatchTest::true_genCom,
                                         &DispatchTest::true_gen, this, cp, false),
                                         "returns never");
    dat->commands()->addCommand( command( "instantFail", &DispatchTest::false_genCom,
                                      &DispatchTest::true_gen, this, cp),
                                      "fails immediately" );
    dat->commands()->addCommand( command( "totalFail", &DispatchTest::false_genCom,
                                    &DispatchTest::false_gen, this, cp),
                                    "fails in command and condition" );
    return dat;
}


void DispatchTest::testParseDispatch()
{
    // this is a global program requesting a method on a local
    // task/processor (ie assert) and a command (instantDone)
    string prog = string("program x { do space.subspace.test.assertMsg(true,\"tpdtrue\") \n")
        + " if space.subspace.test.assert(true) then \n"
        + "       do nothing\n"
        + " do space.subspace.test.instantDone() \n"
//         + " do space.assertMsg(true,\"tpdfail\")\n"
//         + " do this.space.assertMsg(true,\"donotreach\")\n"
        + "}";

    this->doDispatch( prog, &gtc );

    CPPUNIT_ASSERT( gtc.engine()->programs()->getProgramStatus("x") != ProgramInterface::Status::error );
    CPPUNIT_ASSERT( gtc.engine()->programs()->getProgramStatus("x") != ProgramInterface::Status::running );
    CPPUNIT_ASSERT( gtc.engine()->programs()->getProgramStatus("x") == ProgramInterface::Status::stopped );

    this->finishDispatch( &gtc, "x");
}

void DispatchTest::testDispatchFailure()
{
    // this is a global program requesting a command on a local
    // task/processor (ie instantFail).
    string prog = string("program x { do space.subspace.test.instantFail() \n")
        + "}";

    this->doDispatch( prog, &gtc );

    CPPUNIT_ASSERT( gtc.engine()->programs()->getProgramStatus("x") == ProgramInterface::Status::error );

    this->finishDispatch( &gtc, "x");
}
void DispatchTest::testDispatchCondition()
{
    // see if checking a remote condition works
    // also tests peerparser in expressions
    string prog = string("program x { if ( space.subspace.test.assert(true) ) then \n")
        + "do space.subspace.test.instantDone() \n"
        + "else \n"
        + "do space.subspace.test.instantFail() \n"
        + "if ( space.subspace.test.assert(false) ) then \n"
        + "do space.subspace.test.instantFail() \n"
        + "else \n"
        + "do space.subspace.test.instantDone() \n"
        + " }";
    this->doDispatch( prog, &gtc );

    stringstream msg;
    msg <<  "Status was not 'stopped', but "+gtc.engine()->programs()->getProgramStatusStr("x");
    msg << " on line " << gtc.engine()->programs()->getProgram("x")->getLineNumber();
    CPPUNIT_ASSERT_MESSAGE(msg.str(),
                           gtc.engine()->programs()->getProgramStatus("x") == ProgramInterface::Status::stopped);

    this->finishDispatch( &gtc, "x");
}

void DispatchTest::testDispatchAnd()
{
    // see if checking a remote condition works
    string prog = string("program x { do space.subspace.test.assert(true)\n")
        + "and space.subspace.test.assert(true) \n"
        + "and space.subspace.test.assert(true) \n"
        + "do space.subspace.test.instantDone() \n"
        + "and space.subspace.test.instantDone() \n"
        + "and space.subspace.test.instantDone() \n"
        + " }";
    this->doDispatch( prog, &gtc );

    stringstream msg;
    msg <<  "Status was not 'stopped', but "+gtc.engine()->programs()->getProgramStatusStr("x");
    msg << " on line " << gtc.engine()->programs()->getProgram("x")->getLineNumber();
    CPPUNIT_ASSERT_MESSAGE(msg.str(),
                           gtc.engine()->programs()->getProgramStatus("x") == ProgramInterface::Status::stopped);

    this->finishDispatch( &gtc, "x");
}

void DispatchTest::testDispatchTry()
{
    // see if checking a remote condition works
    string prog = string("program x { try space.subspace.test.assert(false)\n")
        + "try space.subspace.test.assert(true) \n"
        + "and space.subspace.test.assert(false) \n"
        + "and space.subspace.test.assert(true) \n"
        + "try space.subspace.test.instantFail()\n"
        + "try space.subspace.test.instantDone() \n"
        + "and space.subspace.test.instantFail() \n"
        + "and space.subspace.test.instantDone() \n"
        + " }";
    this->doDispatch( prog, &gtc );

    CPPUNIT_ASSERT( gtc.engine()->programs()->getProgramStatus("x") != ProgramInterface::Status::error );
    stringstream msg;
    msg <<  "Status was not 'stopped', but "+gtc.engine()->programs()->getProgramStatusStr("x");
    msg << " on line " << gtc.engine()->programs()->getProgram("x")->getLineNumber();
    CPPUNIT_ASSERT_MESSAGE(msg.str(),
                           gtc.engine()->programs()->getProgramStatus("x") == ProgramInterface::Status::stopped);

    this->finishDispatch( &gtc, "x");
}

void DispatchTest::testDispatchUntil()
{
    // see if checking a remote condition works
    string prog = string("program x { do space.subspace.test.instantDone()\n")
        + "until { \n"
        + " if time > 10 ms then continue \n" //  test in simulation takes far less than 1 second
        + "} \n"
        + "do space.subspace.test.instantDone()\n"
        + "until { \n"
        + " if done then continue \n"
        + "} \n"
        + " }";
    this->doDispatch( prog, &gtc );

    stringstream msg;
    msg <<  "Status was not 'stopped', but "+gtc.engine()->programs()->getProgramStatusStr("x");
    msg << " on line " << gtc.engine()->programs()->getProgram("x")->getLineNumber();
    CPPUNIT_ASSERT_MESSAGE(msg.str(),
                           gtc.engine()->programs()->getProgramStatus("x") == ProgramInterface::Status::stopped);

    this->finishDispatch( &gtc, "x");
}

void DispatchTest::testDispatchUntilFail()
{
    // see if checking a remote condition works
    string prog = string("program x { do space.subspace.test.instantFail()\n")
        + "until { \n"
        + " if done then continue \n" //  program should go into error
        + "} \n"
        + " }";
    this->doDispatch( prog, &gtc );

    CPPUNIT_ASSERT( gtc.engine()->programs()->getProgramStatus("x") == ProgramInterface::Status::error );

    this->finishDispatch( &gtc, "x");
}

void DispatchTest::testDispatchMany()
{
    // XXX not a valid test. send not present in Orocos, this looks like 'try'
    // a program which must not fail, even if the command failes.
    string prog = string("program x { ")
        +"  do space.subspace.test.instantDone()\n"
        +"  do space.subspace.test.instantDone()\n"
        +"  do space.subspace.test.instantDone()\n"
        +"  do space.subspace.test.instantDone()\n"
        +" }";
    this->doDispatch( prog, &gtc );

    CPPUNIT_ASSERT( gtc.engine()->programs()->getProgramStatus("x") != ProgramInterface::Status::error );
    CPPUNIT_ASSERT( gtc.engine()->programs()->getProgramStatus("x") == ProgramInterface::Status::stopped );

    this->finishDispatch( &gtc, "x" );
}


void DispatchTest::doDispatch( const std::string& prog, TaskContext* tc )
{
    Parser::ParsedPrograms pg_list;
    try {
        pg_list = parser.parseProgram( prog, tc );
    }
    catch( const file_parse_exception& exc )
        {
            CPPUNIT_ASSERT_MESSAGE( exc.what(), false );
        }
    if ( pg_list.empty() )
        {
            CPPUNIT_ASSERT( false );
        }
    CPPUNIT_ASSERT( tc->engine()->programs()->loadProgram( *pg_list.begin() ) );
    CPPUNIT_ASSERT(ltask.start());
    CPPUNIT_ASSERT(mtask.start());
    CPPUNIT_ASSERT(gtask.start());
    CPPUNIT_ASSERT( tc->engine()->programs()->getProgram( (*pg_list.begin())->getName() )->start() );

    SimulationThread::Instance()->run(1000);
}

void DispatchTest::finishDispatch(TaskContext* tc, std::string prog_name)
{
    CPPUNIT_ASSERT(gtask.stop());
    CPPUNIT_ASSERT(mtask.stop());
    CPPUNIT_ASSERT(ltask.stop());
    tc->engine()->programs()->getProgram( prog_name )->stop();
    CPPUNIT_ASSERT( tc->engine()->programs()->unloadProgram( prog_name ) );

}


