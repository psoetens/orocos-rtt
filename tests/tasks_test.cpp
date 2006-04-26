/***************************************************************************
  tag: Peter Soetens  Mon Jan 10 15:59:49 CET 2005  tasks_test.cpp 

                        tasks_test.cpp -  description
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
 
 

#include "tasks_test.hpp"
#include <unistd.h>
#include <iostream>

#include <corelib/ZeroTimeThread.hpp>
#include <corelib/ZeroLatencyThread.hpp>
#include <corelib/NonRealTimeThread.hpp>
#include <corelib/PeriodicActivity.hpp>
#include <corelib/TimerOneShot.hpp>
#include <corelib/TimeService.hpp>

#include <boost/scoped_ptr.hpp>

using namespace std;
using namespace boost;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ActivitiesTest );

using namespace ORO_CoreLib;
using namespace ORO_CoreLib::detail;

struct TestOverrun
  : public ORO_OS::RunnableInterface
{
  bool fini;
  bool initialize() { fini = false; return true; }

  void step() {
    sleep(1);
  }

  void finalize() {
    fini = true;
  }
};

struct TestPeriodic
    : public ORO_OS::RunnableInterface
{
    int fail;
    bool stepped;

  TimeService::ticks ts;

  TestPeriodic()
    {
    }

    bool initialize() {
        this->reset();
	return true;
    }
    void step() {
      if (stepped == false ) {
	ts = TimeService::Instance()->getTicks();
	stepped = true;
      } else {
	TimeService::Seconds s = TimeService::Instance()->secondsSince( ts );
	  if ( s < this->getThread()->getPeriod() *0.5 ) // if elapsed time is smaller than 50% of period, something went wrong
	    ++fail;
      }
    }
    void finalize() {
    }

    void reset() {
      fail = 0;
      stepped = false;
    }
};

struct TestRunnableInterface
    : public RunnableInterface
{
    bool result;
    bool init, stepped, fini;

    TestRunnableInterface(bool res)
    {
        this->reset(res);
    }

    bool initialize() {
        init    = true;
        return result;
    }
    void step() {
        stepped = true;
    }
    void finalize() {
        fini   = true;
    }

    void reset(bool res) {
        result = res;
        init = false;
        stepped = false;
        fini = false;
    }
};

struct TestAllocate
    : public RunnableInterface
{
    std::vector<std::string> v;
    char*       c;
    std::string s;

    bool initialize() {
        c = 0;
        return true;
    }
    void step() {
        v.resize( 0 );
        v.resize( 1025, std::string("Goodbye Memory") );
        delete[] c;
        c = new char[1025];
        s = "Hello World ";
        s += s;
        s += s;
    }
    void finalize() {
        delete[] c;
        v.resize(0);
    }
};

struct TestSelfRemove
    : public RunnableInterface
{
    int c;
    bool fini;
    bool initialize() {
        c = 0;
        fini = false;
        return true;
    }
    void step() {
        ++c;
        if (c == 5)
            this->getActivity()->stop();
    }
    void finalize() {
        fini = true;
    }
};

void 
ActivitiesTest::setUp()
{
    t_task_np = new NonPreemptibleActivity( ZeroTimeThread::Instance()->getPeriod() );
    t_task_p = new PreemptibleActivity( ZeroLatencyThread::Instance()->getPeriod() );
    t_task_nrt = new NonRealTimeActivity( NonRealTimeThread::Instance()->getPeriod() );
    t_task_prio = new PeriodicActivity( 15, 0.01 );

    t_run_int_np =   new TestRunnableInterface(true);
    t_run_int_p =    new TestRunnableInterface(true);
    t_run_int_nrt =  new TestRunnableInterface(true);
    t_run_int_prio = new TestRunnableInterface(true);
    t_run_int_fail = new TestRunnableInterface(false);

    t_run_allocate = new TestAllocate();
    t_self_remove  = new TestSelfRemove();

    tti = new TimerOneShot(1);
}


void 
ActivitiesTest::tearDown()
{
    delete t_task_np;
    delete t_task_p;
    delete t_task_nrt;
    delete t_task_prio;

    delete t_run_int_np;
    delete t_run_int_p;
    delete t_run_int_nrt;
    delete t_run_int_prio;
    delete t_run_int_fail;

    delete t_run_allocate;
    delete t_self_remove;
    delete tti;
}

void ActivitiesTest::testFailInit()
{
    t_task_np->run( t_run_int_fail );
    t_task_p->run( t_run_int_fail );
    t_task_nrt->run( t_run_int_fail );
    t_task_prio->run( t_run_int_fail );

    CPPUNIT_ASSERT( !t_task_np->start() );
    CPPUNIT_ASSERT( !t_task_p->start() );
    CPPUNIT_ASSERT( !t_task_nrt->start() );
    CPPUNIT_ASSERT( !t_task_prio->start() );

    CPPUNIT_ASSERT( !t_task_np->stop() );
    CPPUNIT_ASSERT( !t_task_p->stop() );
    CPPUNIT_ASSERT( !t_task_nrt->stop() );
    CPPUNIT_ASSERT( !t_task_prio->stop() );

}

void ActivitiesTest::testOverrun()
{
  bool r = false;
  // create
  boost::scoped_ptr<TestOverrun> run( new TestOverrun() );
  boost::scoped_ptr<ORO_OS::ThreadInterface> t( new ORO_OS::PeriodicThread(25,"ORThread", 0.1) );
  t->run( run.get() );

  t->start();
  sleep(2);

  r = !t->isRunning();

  t->run(0);

  CPPUNIT_ASSERT_MESSAGE( "Failed to detect step overrun in Thread", r);

  CPPUNIT_ASSERT_MESSAGE( "Failed to execute finalize in emergencyStop", run->fini );
  
}


void ActivitiesTest::testThreads()
{
  bool r = false;
  // create
  boost::scoped_ptr<TestPeriodic> run( new TestPeriodic() );

  boost::scoped_ptr<ORO_OS::ThreadInterface> t( new ORO_OS::PeriodicThread(25,"PThread", 0.1) );
  t->run( run.get() );

  r = t->start();
  CPPUNIT_ASSERT_MESSAGE( "Failed to start Thread", r);
  r = t->stop();
  CPPUNIT_ASSERT_MESSAGE( "Failed to stop Thread", r);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Periodic Failure !", run->fail, 0);
  r = t->start();
  CPPUNIT_ASSERT_MESSAGE( "Failed to start Thread", r);
  sleep(1);
  r = t->stop();
  CPPUNIT_ASSERT_MESSAGE( "Failed to stop Thread", r);
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Periodic Failure !", run->fail, 0);
  r = t->start();
  CPPUNIT_ASSERT_MESSAGE( "Failed to start Thread", r);
  sleep(1);
  r = t->stop();
  CPPUNIT_ASSERT_MESSAGE( "Failed to stop Thread", r);
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Periodic Failure !", run->fail, 0);
  sleep(1);
  r = t->start();
  CPPUNIT_ASSERT_MESSAGE( "Failed to start Thread after long stop", r);
  sleep(1);
  r = t->stop();
  CPPUNIT_ASSERT_MESSAGE("Failed to stop Thread", r);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Periodic Failure !",  run->fail, 0);
  t->run(0);
}

void ActivitiesTest::testTimer()
{
    PreemptibleActivity t_task_lst( ZeroLatencyThread::Instance()->getPeriod() );

    // Add 5 tasks.
    CPPUNIT_ASSERT( tti->addActivity( t_task_np ) );
    CPPUNIT_ASSERT( tti->addActivity( t_task_p ) );
    CPPUNIT_ASSERT( tti->addActivity( t_task_nrt ) );
    CPPUNIT_ASSERT( tti->addActivity( t_task_prio ) );
    CPPUNIT_ASSERT( tti->addActivity( &t_task_lst ) );

    // Remove last :
    CPPUNIT_ASSERT( tti->removeActivity( &t_task_lst ) );
    tti->tick();

    // Remove First:
    CPPUNIT_ASSERT( tti->removeActivity( t_task_np ) );
    tti->tick();

    // Remove middle :
    CPPUNIT_ASSERT( tti->removeActivity( t_task_nrt ) );
    tti->tick();

    CPPUNIT_ASSERT( tti->addActivity( t_task_np ) );
    CPPUNIT_ASSERT( tti->addActivity( t_task_nrt ) );
    CPPUNIT_ASSERT( tti->addActivity( &t_task_lst ) );

    //Remove 2 in middle :
    CPPUNIT_ASSERT( tti->removeActivity( t_task_prio ) );
    CPPUNIT_ASSERT( tti->removeActivity( t_task_nrt ) );
    tti->tick();

    //Remove all :
    CPPUNIT_ASSERT( tti->removeActivity( t_task_np ) );
    CPPUNIT_ASSERT( tti->removeActivity( t_task_p ) );
    CPPUNIT_ASSERT( tti->removeActivity( &t_task_lst ) );
    tti->tick();

}

void ActivitiesTest::testNonPeriodic()
{
    scoped_ptr<TestRunnableInterface> t_run_int_nonper
        ( new TestRunnableInterface(true) );
    // force ordering of scoped_ptr destruction.
    {
        scoped_ptr<NonPeriodicActivity> t_task_nonper
            ( new NonPeriodicActivity( 14 ) );

        CPPUNIT_ASSERT( t_task_nonper->run( t_run_int_nonper.get() ) );
        CPPUNIT_ASSERT( t_task_nonper->start() );
        sleep(1);
        CPPUNIT_ASSERT( t_run_int_nonper->stepped );
        CPPUNIT_ASSERT( t_run_int_nonper->init );
        CPPUNIT_ASSERT( t_task_nonper->stop() );
        CPPUNIT_ASSERT( t_run_int_nonper->fini );
        CPPUNIT_ASSERT( !t_task_nonper->isRunning() );
        CPPUNIT_ASSERT( t_task_nonper->run( 0 ) );
        CPPUNIT_ASSERT( t_task_nonper->start() );
        // now blocks on events :
        CPPUNIT_ASSERT( t_task_nonper->isActive() );
        sleep(1);
        CPPUNIT_ASSERT( t_task_nonper->isRunning() );
        CPPUNIT_ASSERT( t_task_nonper->stop() );
        // stop() should be fully synchronising...
        CPPUNIT_ASSERT( !t_task_nonper->isRunning() );
    }
}

void ActivitiesTest::testSelfRemove()
{
    CPPUNIT_ASSERT( t_task_np->run(t_self_remove) );
    CPPUNIT_ASSERT( t_task_np->start() );
    sleep(1);
    CPPUNIT_ASSERT( !t_task_np->isRunning() );
    CPPUNIT_ASSERT( t_self_remove->fini );
}

void ActivitiesTest::testStartStop()
{
    testStart();
    testPause();
    testStop();
}

void ActivitiesTest::testRunnableInterface()
{
    testAddRunnableInterface();
    testStart();
    testRunnableInterfaceInit();
    testPause();
    testRunnableInterfaceExecution();
    testStop();
    testRemoveRunnableInterface();
}

void ActivitiesTest::testAllocation()
{
    testAddAllocate();
    testStart();
    testPause();
    testStop();
    testRemoveAllocate();
}

void ActivitiesTest::testAddRunnableInterface()
{
    bool adding_np = t_task_np->run( t_run_int_np );
    CPPUNIT_ASSERT( adding_np );
    bool adding_p =  t_task_p->run( t_run_int_p );
    CPPUNIT_ASSERT( adding_p );
    bool adding_nrt = t_task_nrt->run( t_run_int_nrt );
    CPPUNIT_ASSERT( adding_nrt );
    bool adding_prio = t_task_prio->run( t_run_int_prio );
    CPPUNIT_ASSERT( adding_prio );
}

void ActivitiesTest::testRemoveRunnableInterface()
{

    CPPUNIT_ASSERT( t_run_int_np->fini );
    CPPUNIT_ASSERT( t_run_int_p->fini );
    CPPUNIT_ASSERT( t_run_int_nrt->fini );
    CPPUNIT_ASSERT( t_run_int_prio->fini );

    CPPUNIT_ASSERT( t_task_np->run( 0 ) );
    CPPUNIT_ASSERT( t_task_p->run( 0 ) );
    CPPUNIT_ASSERT( t_task_nrt->run( 0 ) );
    CPPUNIT_ASSERT( t_task_prio->run( 0 ) );
}

void ActivitiesTest::testStart()
{
    CPPUNIT_ASSERT( t_task_np->start() );
    CPPUNIT_ASSERT( t_task_p->start());
    CPPUNIT_ASSERT( t_task_nrt->start());
    CPPUNIT_ASSERT( t_task_prio->start());
    
    CPPUNIT_ASSERT( t_task_np->isRunning() );
    CPPUNIT_ASSERT( t_task_p->isRunning() );
    CPPUNIT_ASSERT( t_task_nrt->isRunning() );
    CPPUNIT_ASSERT( t_task_prio->isRunning() );
}

void ActivitiesTest::testPause()
{
    sleep(1);
}

void ActivitiesTest::testRunnableInterfaceInit() {
    CPPUNIT_ASSERT( t_run_int_np->init );
    CPPUNIT_ASSERT( t_run_int_p->init );
    CPPUNIT_ASSERT( t_run_int_nrt->init );
    CPPUNIT_ASSERT( t_run_int_prio->init );
}

void ActivitiesTest::testRunnableInterfaceExecution() {

    CPPUNIT_ASSERT( t_run_int_np->stepped );
    CPPUNIT_ASSERT( t_run_int_p->stepped );
    CPPUNIT_ASSERT( t_run_int_nrt->stepped );
    CPPUNIT_ASSERT( t_run_int_prio->stepped );
}

void ActivitiesTest::testStop()
{
    CPPUNIT_ASSERT( t_task_np->stop());
    CPPUNIT_ASSERT( t_task_p->stop());
    CPPUNIT_ASSERT( t_task_nrt->stop());
    CPPUNIT_ASSERT( t_task_prio->stop());
    
    CPPUNIT_ASSERT( !t_task_np->isRunning() );
    CPPUNIT_ASSERT( !t_task_p->isRunning() );
    CPPUNIT_ASSERT( !t_task_nrt->isRunning() );
    CPPUNIT_ASSERT( !t_task_prio->isRunning() );
}

void ActivitiesTest::testAddAllocate()
{
    CPPUNIT_ASSERT( t_task_np->run( t_run_allocate ) );
}

void ActivitiesTest::testRemoveAllocate()
{
    CPPUNIT_ASSERT( t_task_np->run( 0 ) );
}
    

