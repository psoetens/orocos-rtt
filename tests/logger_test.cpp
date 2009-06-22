/***************************************************************************
  tag: Peter Soetens  Mon Jan 10 15:59:51 CET 2005  logger_test.cpp

                        logger_test.cpp -  description
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



#include "logger_test.hpp"
#include <unistd.h>
#include <iostream>
#include <boost/scoped_ptr.hpp>
#include "os/PeriodicThread.hpp"

using namespace boost;
using namespace std;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( LoggerTest );

using namespace RTT;

class Dummy {};

#define QS 10

void
LoggerTest::setUp()
{
    logger = Logger::Instance();
}


void
LoggerTest::tearDown()
{
}

void LoggerTest::testStartStop()
{
    CPPUNIT_ASSERT( logger != 0 );
    CPPUNIT_ASSERT( &Logger::log() != 0 );
}
void LoggerTest::testLogEnv()
{
    Logger::log() << Logger::Debug  << "Debug Level set + text"<< Logger::nl;
    Logger::log() << "Test Log Environment variable : Single line" << Logger::endl;
    Logger::log() << "Test Log Environment variable : Two ";
    Logger::log() << "lines on one line." << Logger::endl;
    Logger::log() << "Test Log Environment variable : Two" << Logger::nl;
    Logger::log() << "lines on two lines." << Logger::endl;

    Logger::log() << "Test Log Environment variable : nl" << Logger::nl;
    Logger::log() << "Test Log Environment variable : flush" << flush;
    Logger::log() << " and std::endl." << std::endl;
}

void LoggerTest::testNewLog()
{
    log( Debug )  << "Debug Level set + text"<< endlog();
    log() << "Test Log Environment variable : Single line" << endlog(Debug);
    log() << "Test Log Environment variable : Two ";
    log() << "lines on one line." << endlog();
    log() << "Test Log Environment variable : Two" << nlog();
    log() << "lines on two lines." << endlog();

    log() << "Test Log Environment variable : nl" << nlog();
    log() << "Test Log Environment variable : flush" << flushlog();
    log() << " and std::endl." << std::endl;
}

struct TestLog
  : public RTT::OS::RunnableInterface
{
  bool fini;
  bool initialize() { fini = false; return true; }

  void step() {
      Logger::In in("TLOG");
      log(Info) << "Hello this is the world speaking elaborately and lengthy...!" <<endlog();
  }

  void finalize() {
    fini = true;
  }
};

void LoggerTest::testThreadLog()
{
  boost::scoped_ptr<TestLog> run( new TestLog() );
  boost::scoped_ptr<RTT::OS::ThreadInterface> t( new RTT::OS::PeriodicThread(25,"ORThread", 0.001) );
  boost::scoped_ptr<TestLog> run2( new TestLog() );
  boost::scoped_ptr<RTT::OS::ThreadInterface> t2( new RTT::OS::PeriodicThread(25,"ORThread", 0.001) );

  t->run( run.get() );
  t2->run( run2.get() );

  t->start();
  t2->start();
  sleep(1);
  t->stop();
  t2->stop();

}