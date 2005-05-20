#ifndef ORO_OS_FOSI_INTERNAL_HPP
#define ORO_OS_FOSI_INTERNAL_HPP

#include "os/ThreadInterface.hpp"
#include "os/fosi.h"
#include <iostream>

#define INTERNAL_QUAL static inline

struct GNUTask {
    GNUTask( NANO_TIME periodi ) : periodMark(0), period( periodi ) {}
    NANO_TIME periodMark;
    NANO_TIME period;
};

namespace ORO_OS
{
    namespace detail {

        INTERNAL_QUAL RTOS_TASK* rtos_task_init( ThreadInterface* thread )
        {
            return new GNUTask( thread->getPeriodNS() );
        }

        INTERNAL_QUAL void rtos_task_make_hard_real_time(RTOS_TASK*) {
        }

        INTERNAL_QUAL void rtos_task_make_soft_real_time(RTOS_TASK*) {
        }

        INTERNAL_QUAL int rtos_task_is_hard_real_time(RTOS_TASK*) {
            return 0;
        }

        INTERNAL_QUAL void rtos_task_make_periodic(RTOS_TASK* mytask, NANO_TIME nanosecs )
        {
            // set period
            mytask->period = nanosecs;
            // set next wake-up time.
            mytask->periodMark = rtos_get_time_ns() + nanosecs;
        }
        

        INTERNAL_QUAL void rtos_task_set_period( RTOS_TASK* mytask, NANO_TIME nanosecs )
        {
            mytask->period = nanosecs;
            mytask->periodMark = rtos_get_time_ns() + nanosecs;
        }

        INTERNAL_QUAL void rtos_task_wait_period( RTOS_TASK* task )
        {
            // CALCULATE in nsecs
            NANO_TIME timeRemaining = task->period - ( rtos_get_time_ns() - task->periodMark );

            //rtos_printf("Waiting for %d nsec\n",timeRemaining);

            if ( timeRemaining > 0 ) {
                TIME_SPEC ts( ticks2timespec( timeRemaining ) );
                rtos_nanosleep( &ts , NULL );
            }
            else
                ;//rtos_printf( "%s did not get deadline !", taskNameGet() );

            // next wake-up time :
            task->periodMark += task->period;
        }

        INTERNAL_QUAL void rtos_task_delete(RTOS_TASK* mytask) {
            delete mytask;
        }

        // for both SingleTread and PeriodicThread
        template<class T>
        INTERNAL_QUAL void rtos_thread_init( T* thread, const std::string& name ) {
            if ( name.empty() )
               thread->setName( "Thread" );
            else
                thread->setName( name.c_str() );
        }

        INTERNAL_QUAL int rtos_set_scheduler(int type, int priority)
        {
            // init the scheduler. The rt_task_initschmod code is broken, so we do it ourselves.
            struct sched_param mysched;
            mysched.sched_priority = sched_get_priority_max(type) - priority;
            // check lower bounds :
            if (type == SCHED_OTHER && mysched.sched_priority != 0 ) {
                mysched.sched_priority = 0; // SCHED_OTHER must be zero
            } else if (type == !SCHED_OTHER &&  mysched.sched_priority < 1 ) {
                mysched.sched_priority = 1; // !SCHED_OTHER must be 1 or higher
            }
            // check upper bound
            if ( mysched.sched_priority > 99)
                mysched.sched_priority = 99;
            // set scheduler
            return sched_setscheduler(0, type, &mysched);
        }


    }
}
#undef INTERNAL_QUAL
#endif
