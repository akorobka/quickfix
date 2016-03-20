/* -*- C++ -*- */

/****************************************************************************
** Copyright (c) 2001-2014
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifndef FIX_MUTEX_H
#define FIX_MUTEX_H

#include "Utility.h"

namespace FIX
{
#ifdef HAVE_BOOST

typedef boost::recursive_mutex	  Mutex;
typedef boost::unique_lock<Mutex> Locker;
typedef boost::unique_lock<Mutex> FlexLocker;

#else

/// Portable implementation of a mutex.
class Mutex
{
public:
  Mutex()
  {
#ifdef _MSC_VER
    InitializeCriticalSection( &m_mutex );
#else
    m_count = 0;
    m_threadID = 0;
    //pthread_mutexattr_t attr;
    //pthread_mutexattr_init(&attr);
    //pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    //pthread_mutex_init(&m_mutex, &attr);
    pthread_mutex_init( &m_mutex, 0 );
#endif
  }

  ~Mutex()
  {
#ifdef _MSC_VER
    DeleteCriticalSection( &m_mutex );
#else
    pthread_mutex_destroy( &m_mutex );
#endif
  }

  void lock()
  {
#ifdef _MSC_VER
    EnterCriticalSection( &m_mutex );
#else
    if ( m_count && m_threadID == pthread_self() )
    { ++m_count; return ; }
    pthread_mutex_lock( &m_mutex );
    ++m_count;
    m_threadID = pthread_self();
#endif
  }

  bool trylock()
  {
#ifdef _MSC_VER
    return TryEnterCriticalSection( &m_mutex ) != 0;
#else
    if ( m_count && m_threadID == pthread_self() )
    { ++m_count; return true; }
    if (pthread_mutex_trylock( &m_mutex ))
        return false;
    ++m_count;
    m_threadID = pthread_self();
    return true;
#endif
  }

  void unlock()
  {
#ifdef _MSC_VER
    LeaveCriticalSection( &m_mutex );
#else
    if ( m_count > 1 )
    { m_count--; return ; }
    --m_count;
    m_threadID = 0;
    pthread_mutex_unlock( &m_mutex );
#endif
  }

private:

#ifdef _MSC_VER
  CRITICAL_SECTION m_mutex;
#else
  pthread_mutex_t m_mutex;
  pthread_t m_threadID;
  int m_count;
#endif
};

/// Locks/Unlocks a mutex using RAII.
class Locker
{
public:
  Locker( Mutex& mutex )
  : m_mutex( mutex )
  {
    m_mutex.lock();
  }

  bool owns_lock() const {
    return true;
  }

  ~Locker()
  {
    m_mutex.unlock();
  }
private:
  Mutex& m_mutex;
};

/// Locks/Unlocks a mutex using RAII, can be unlocked.
class FlexLocker
{
public:
  FlexLocker( Mutex& mutex )
  : m_mutex( &mutex )
  {
    m_mutex->lock();
  }

  bool owns_lock() const {
    return m_mutex != NULL;
  }

  void unlock() {
    m_mutex->unlock();
    m_mutex = NULL;
  }

  ~FlexLocker()
  {
    if (m_mutex) m_mutex->unlock();
  }
private:
  Mutex* m_mutex;
};

#endif // HAVE_BOOST

#ifdef HAVE_TBB

typedef tbb::spin_mutex Spinlock;
typedef tbb::spin_mutex::scoped_lock SpinLocker;

#else

#ifdef _MSC_VER

typedef Mutex Spinlock;
typedef Locker SpinLocker;

#else

class Spinlock {

public:
  Spinlock()
  { pthread_spin_init(&m_spinlock, false); }

  ~Spinlock()
  { pthread_spin_destroy(&m_spinlock); }

  void lock()
  { pthread_spin_lock(&m_spinlock); }

  void unlock()
  { pthread_spin_unlock(&m_spinlock); }

private:
  pthread_spinlock_t m_spinlock;
};

class SpinLocker
{
public:
  SpinLocker(Spinlock& s) : m_spinlock(s)
  { m_spinlock.lock(); }

  ~SpinLocker()
  { m_spinlock.unlock(); }

private:
  Spinlock& m_spinlock;
};
#endif // _MSC_VER
#endif // HAVE_TBB

}

#endif
