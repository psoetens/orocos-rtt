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

#include <PeriodicActivity.hpp>
#include <TimeService.hpp>

#include <boost/scoped_ptr.hpp>

using namespace std;
using namespace boost;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ActivitiesTest );

using namespace RTT;
using namespace RTT::detail;

struct TestOverrun
  : public RTT::OS::RunnableInterface
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
    : public RTT::OS::RunnableInterface
{
    int overfail, underfail, succ;
    bool stepped;

    TimeService::ticks ts;

    TestPeriodic()
        : overfail(0), underfail(0), succ(0), stepped(false)
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
            if ( s < this->getThread()->getPeriod() *0.9 ) { // if elapsed time is smaller than 10% of period, something went wrong
                ++underfail;
                //rtos_printf("UnderFailPeriod: %f \n", s);
            }
            else if ( s > this->getThread()->getPeriod() *1.1 ) { // if elapsed time is smaller than 10% of period, something went wrong
                ++overfail;
                //rtos_printf("OverFailPeriod: %f \n", s);
            }
            else {
                ++succ;
                //rtos_printf("SuccPeriod: %f \n", s);
            }
            ts = TimeService::Instance()->getTicks();
        }
    }
    void finalize() {
        if (overfail || underfail)
            cerr <<"overfail is:"<<overfail<<", underfail is:"<<underfail<< " success is: "<<succ<<endl;
    }

    void reset() {
        overfail = 0;
        underfail = 0;
        succ = 0;
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
    bool breakLoop() {
        return true;
    }
    void loop() {
        this->getActivity()->stop();
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
    t_task_prio = new PeriodicActivity( 15, 0.01 );

    t_run_int_prio = new TestRunnableInterface(true);
    t_run_int_fail = new TestRunnableInterface(false);

    t_run_allocate = new TestAllocate();
    t_self_remove  = new TestSelfRemove();

}


void
ActivitiesTest::tearDown()
{
    delete t_task_prio;

    delete t_run_int_prio;
    delete t_run_int_fail;

    delete t_run_allocate;
    delete t_self_remove;
}

void ActivitiesTest::testFailInit()
{
    t_task_prio->run( t_run_int_fail );

    CPPUNIT_ASSERT( !t_task_prio->start() );
    CPPUNIT_ASSERT( !t_task_prio->stop() );

}

void ActivitiesTest::testOverrun()
{
  bool r = false;
  // create
  boost::scoped_ptr<TestOverrun> run( new TestOverrun() );
  boost::scoped_ptr<RTT::OS::ThreadInterface> t( new RTT::OS::PeriodicThread(25,"ORThread", 0.1) );
  CPPUNIT_ASSERT_EQUAL(25,t->getPriority() );
  CPPUNIT_ASSERT_EQUAL(0.1,t->getPeriod() );

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

  boost::scoped_ptr<RTT::OS::ThreadInterface> t( new RTT::OS::PeriodicThread(RTT::OS::HighestPriority,"PThread", 0.1) );
  t->run( run.get() );

  r = t->start();
  CPPUNIT_ASSERT_MESSAGE( "Failed to start Thread", r);
  r = t->stop();
  CPPUNIT_ASSERT_MESSAGE( "Failed to stop Thread", r);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Periodic Failure: period of step() to long !", run->overfail, 0);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Periodic Failure: period of step() to short!", run->underfail, 0);
  r = t->start();
  CPPUNIT_ASSERT_MESSAGE( "Failed to start Thread", r);
  sleep(1);
  r = t->stop();
  CPPUNIT_ASSERT_MESSAGE( "Failed to stop Thread", r);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Periodic Failure: period of step() to long !", run->overfail, 0);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Periodic Failure: period of step() to short!", run->underfail, 0);
  t->run(0);
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
        CPPUNIT_ASSERT( t_task_nonper->stop() );
        // stop() should be fully synchronising...
        CPPUNIT_ASSERT( !t_task_nonper->isRunning() );
    }
}

void ActivitiesTest::testSelfRemove()
{
    scoped_ptr<TestSelfRemove> t_run_int_nonper
        ( new TestSelfRemove() );
    scoped_ptr<NonPeriodicActivity> t_task_nonper
        ( new NonPeriodicActivity( 14 ) );
    CPPUNIT_ASSERT( t_task_nonper->run( t_run_int_nonper.get() ) );
    CPPUNIT_ASSERT( t_task_nonper->start() );
    CPPUNIT_ASSERT( t_task_prio->run(t_self_remove) );
    CPPUNIT_ASSERT( t_task_prio->start() );
    sleep(1);
    CPPUNIT_ASSERT( !t_task_prio->isRunning() );
    CPPUNIT_ASSERT( t_self_remove->fini );
    CPPUNIT_ASSERT( !t_task_nonper->isRunning() );
    CPPUNIT_ASSERT( t_run_int_nonper->fini );
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
    bool adding_prio = t_task_prio->run( t_run_int_prio );
    CPPUNIT_ASSERT( adding_prio );
}

void ActivitiesTest::testRemoveRunnableInterface()
{
    CPPUNIT_ASSERT( t_run_int_prio->fini );
    CPPUNIT_ASSERT( t_task_prio->run( 0 ) );
}

void ActivitiesTest::testStart()
{
    CPPUNIT_ASSERT( t_task_prio->start());

    CPPUNIT_ASSERT( t_task_prio->isRunning() );
}

void ActivitiesTest::testPause()
{
    sleep(1);
}

void ActivitiesTest::testRunnableInterfaceInit() {
    CPPUNIT_ASSERT( t_run_int_prio->init );
}

void ActivitiesTest::testRunnableInterfaceExecution() {

    CPPUNIT_ASSERT( t_run_int_prio->stepped );
}

void ActivitiesTest::testStop()
{
    CPPUNIT_ASSERT( t_task_prio->stop());

    CPPUNIT_ASSERT( !t_task_prio->isRunning() );
}

void ActivitiesTest::testAddAllocate()
{
    CPPUNIT_ASSERT( t_task_prio->run( t_run_allocate ) );
}

void ActivitiesTest::testRemoveAllocate()
{
    CPPUNIT_ASSERT( t_task_prio->run( 0 ) );
}

