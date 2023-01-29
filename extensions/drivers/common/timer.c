/*!
 * \file      timer.c
 *
 * \brief     Timer objects and scheduling management implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */

#include "timer.h"

#include "smtc_hal_rtc.h"
#include "smtc_hal_mcu.h"

/*!
 * Safely execute call back
 */
#define ExecuteCallBack( _callback_, context ) \
    do                                         \
    {                                          \
        if( _callback_ == NULL )               \
        {                                      \
            while( 1 );                        \
        }                                      \
        else                                   \
        {                                      \
            _callback_( context );             \
        }                                      \
    }while( 0 );

/*!
 * Begins critical section
 */
#define CRITICAL_SECTION_BEGIN( ) uint32_t mask; hal_mcu_critical_section_begin( &mask )

/*!
 * Ends critical section
 */
#define CRITICAL_SECTION_END( ) hal_mcu_critical_section_end( &mask )

/*!
 * Timers list head pointer
 */
static TimerEvent_t* TimerListHead = NULL;

/*!
 * \brief Adds or replace the head timer of the list.
 *
 * \remark The list is automatically sorted. The list head always contains the
 *         next timer to expire.
 *
 * \param [IN]  obj Timer object to be become the new head
 * \param [IN]  remainingTime Remaining time of the previous head to be replaced
 */
static void TimerInsertNewHeadTimer( TimerEvent_t* obj );

/*!
 * \brief Adds a timer to the list.
 *
 * \remark The list is automatically sorted. The list head always contains the
 *         next timer to expire.
 *
 * \param [IN]  obj Timer object to be added to the list
 * \param [IN]  remainingTime Remaining time of the running head after which the object may be added
 */
static void TimerInsertTimer( TimerEvent_t* obj );

/*!
 * \brief Sets a timeout with the duration "timestamp"
 *
 * \param [IN] timestamp Delay duration
 */
static void TimerSetTimeout( TimerEvent_t* obj );

/*!
 * \brief Check if the Object to be added is not already in the list
 *
 * \param [IN] timestamp Delay duration
 * \retval true (the object is already in the list) or false
 */
static bool TimerExists( TimerEvent_t* obj );

void TimerInit( TimerEvent_t* obj, void ( *callback )( void* context ) ) {
    obj->Timestamp = 0;
    obj->ReloadValue = 0;
    obj->IsStarted = false;
    obj->IsNext2Expire = false;
    obj->Callback = callback;
    obj->Context = NULL;
    obj->Next = NULL;
}

void TimerSetContext( TimerEvent_t* obj, void* context ) {
    obj->Context = context;
}

void TimerStart( TimerEvent_t* obj ) {
    uint32_t elapsedTime = 0;

    CRITICAL_SECTION_BEGIN( );

    if( ( obj == NULL ) || ( TimerExists( obj ) == true ) ) {
        CRITICAL_SECTION_END( );
        return;
    }

    obj->Timestamp = obj->ReloadValue;
    obj->IsStarted = true;
    obj->IsNext2Expire = false;

    if( TimerListHead == NULL ) {
        hal_rtc_set_timer_context( );
        // Inserts a timer at time now + obj->Timestamp
        TimerInsertNewHeadTimer( obj );
    } else {
        elapsedTime = hal_rtc_get_timer_elapsed_time( );
        obj->Timestamp += elapsedTime;

        if( obj->Timestamp < TimerListHead->Timestamp ) {
            TimerInsertNewHeadTimer( obj );
        } else {
            TimerInsertTimer( obj );
        }
    }

    CRITICAL_SECTION_END( );
}

void TimerStartNoReload( TimerEvent_t* obj ) {
    uint32_t elapsedTime = 0;

    CRITICAL_SECTION_BEGIN( );

    if( ( obj == NULL ) || ( TimerExists( obj ) == true ) ) {
        CRITICAL_SECTION_END( );
        return;
    }

    // obj->Timestamp = obj->ReloadValue;
    obj->IsStarted = true;
    obj->IsNext2Expire = false;

    if( TimerListHead == NULL ) {
        hal_rtc_set_timer_context( );
        // Inserts a timer at time now + obj->Timestamp
        TimerInsertNewHeadTimer( obj );
    } else {
        elapsedTime = hal_rtc_get_timer_elapsed_time( );
        obj->Timestamp += elapsedTime;

        if( obj->Timestamp < TimerListHead->Timestamp ) {
            TimerInsertNewHeadTimer( obj );
        } else {
            TimerInsertTimer( obj );
        }
    }

    CRITICAL_SECTION_END( );
}

static void TimerInsertTimer( TimerEvent_t* obj ) {
    TimerEvent_t* cur = TimerListHead;
    TimerEvent_t* next = TimerListHead->Next;

    while( cur->Next != NULL ) {
        if( obj->Timestamp > next->Timestamp ) {
            cur = next;
            next = next->Next;
        } else {
            cur->Next = obj;
            obj->Next = next;
            return;
        }
    }

    cur->Next = obj;
    obj->Next = NULL;
}

static void TimerInsertNewHeadTimer( TimerEvent_t* obj ) {
    TimerEvent_t* cur = TimerListHead;

    if( cur != NULL ) {
        cur->IsNext2Expire = false;
    }

    obj->Next = cur;
    TimerListHead = obj;
    TimerSetTimeout( TimerListHead );
}

bool TimerIsStarted( TimerEvent_t* obj ) {
    return obj->IsStarted;
}

void TimerIrqHandler( void ) {
    TimerEvent_t* cur;
    TimerEvent_t* next;

    uint32_t old =  hal_rtc_get_timer_context( );
    uint32_t now =  hal_rtc_set_timer_context( );
    uint32_t deltaContext = now - old; // intentional wrap around

    // Update timeStamp based upon new Time Reference
    // because delta context should never exceed 2^32
    if( TimerListHead != NULL ) {
        for( cur = TimerListHead; cur->Next != NULL; cur = cur->Next ) {
            next = cur->Next;

            if( next->Timestamp > deltaContext ) {
                next->Timestamp -= deltaContext;
            } else {
                next->Timestamp = 0;
            }
        }
    }

    // Execute immediately the alarm callback
    if( TimerListHead != NULL ) {
        cur = TimerListHead;
        TimerListHead = TimerListHead->Next;
        cur->IsStarted = false;
        ExecuteCallBack( cur->Callback, cur->Context );
    }

    // Remove all the expired object from the list
    while( ( TimerListHead != NULL ) && ( TimerListHead->Timestamp < hal_rtc_get_timer_elapsed_time( ) ) ) {
        cur = TimerListHead;
        TimerListHead = TimerListHead->Next;
        cur->IsStarted = false;
        ExecuteCallBack( cur->Callback, cur->Context );
    }

    // Start the next TimerListHead if it exists AND NOT running
    if( ( TimerListHead != NULL ) && ( TimerListHead->IsNext2Expire == false ) ) {
        TimerSetTimeout( TimerListHead );
    }
}

void TimerStop( TimerEvent_t* obj ) {
    CRITICAL_SECTION_BEGIN( );

    TimerEvent_t* prev = TimerListHead;
    TimerEvent_t* cur = TimerListHead;

    // List is empty or the obj to stop does not exist
    if( ( TimerListHead == NULL ) || ( obj == NULL ) ) {
        CRITICAL_SECTION_END( );
        return;
    }

    obj->IsStarted = false;

    if( TimerListHead == obj ) { // Stop the Head
        if( TimerListHead->IsNext2Expire == true ) { // The head is already running
            TimerListHead->IsNext2Expire = false;

            if( TimerListHead->Next != NULL ) {
                TimerListHead = TimerListHead->Next;
                TimerSetTimeout( TimerListHead );
            } else {
                hal_rtc_wakeup_timer_stop( );
                TimerListHead = NULL;
            }
        } else { // Stop the head before it is started
            if( TimerListHead->Next != NULL ) {
                TimerListHead = TimerListHead->Next;
            } else {
                TimerListHead = NULL;
            }
        }
    } else { // Stop an object within the list
        while( cur != NULL ) {
            if( cur == obj ) {
                if( cur->Next != NULL ) {
                    cur = cur->Next;
                    prev->Next = cur;
                } else {
                    cur = NULL;
                    prev->Next = cur;
                }

                break;
            } else {
                prev = cur;
                cur = cur->Next;
            }
        }
    }

    CRITICAL_SECTION_END( );
}

static bool TimerExists( TimerEvent_t* obj ) {
    TimerEvent_t* cur = TimerListHead;

    while( cur != NULL ) {
        if( cur == obj ) {
            return true;
        }

        cur = cur->Next;
    }

    return false;
}

void TimerReset( TimerEvent_t* obj ) {
    TimerStop( obj );
    TimerStart( obj );
}

void TimerSetValue( TimerEvent_t* obj, uint32_t value ) {
    uint32_t minValue = 0;
    uint32_t ticks = rtc_ms_2_ticks( value );

    TimerStop( obj );

    minValue = hal_rtc_get_minimum_timeout( );

    if( ticks < minValue ) {
        ticks = minValue;
    }

    obj->Timestamp = ticks;
    obj->ReloadValue = ticks;
}

TimerTime_t TimerGetCurrentTime( void ) {
    return hal_rtc_get_time_ms();
}

TimerTime_t TimerGetElapsedTime( TimerTime_t past ) {
    if( past == 0 ) {
        return 0;
    }

    uint32_t nowInTicks = hal_rtc_get_time_ticks( );
    uint32_t pastInTicks = rtc_ms_2_ticks( past );

    // Intentional wrap around. Works Ok if tick duration below 1ms
    return rtc_tick_2_ms( nowInTicks - pastInTicks );
}

static void TimerSetTimeout( TimerEvent_t* obj ) {
    int32_t minTicks = hal_rtc_get_minimum_timeout( );
    obj->IsNext2Expire = true;

    uint32_t timeout = obj->Timestamp - hal_rtc_get_timer_elapsed_time( );

    // In case deadline too soon
    if( timeout  < minTicks ) {
        timeout = minTicks;
    }

    hal_rtc_wakeup_timer_set_ms( timeout );
}

// TimerTime_t TimerTempCompensation( TimerTime_t period, float temperature )
// {
//     return RtcTempCompensation( period, temperature );
// }

// void TimerProcess( void )
// {
//     RtcProcess( );
// }

static TimerEvent_t* backup[8];
static uint8_t backuped_timers = 0;

void TimerBackup( void ) {
    TimerEvent_t* cur = TimerListHead;

    while( cur != NULL ) {
        backup[backuped_timers] = cur;
        backup[backuped_timers]->Timestamp -= hal_rtc_get_timer_elapsed_time();
        TimerStop( backup[backuped_timers] );
        backuped_timers++;
        cur = cur->Next;
    }
}

void TimerRestore( void ) {
    for( uint8_t i = 0; i < backuped_timers; i++ ) {
        TimerStartNoReload( backup[i] );
    }

    backuped_timers = 0;
}
