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

#ifndef ATOMIC_COUNT_H
#define ATOMIC_COUNT_H

#include "Mutex.h"

namespace FIX
{

  class AtomicCount
  {
    public:
      typedef long value_type;

      explicit AtomicCount( value_type v ) { m_value = v; }

#ifdef HAVE_TBB

      value_type operator++() { return ++m_value; }
      value_type operator--() { return --m_value; }
      operator value_type () const { return m_value; }

    private:
      typedef tbb::atomic<value_type> counter_type;

#elif defined (_MSC_VER)

      value_type operator++() { return ::InterlockedIncrement( &m_value ); }
      value_type operator--() { return ::InterlockedDecrement( &m_value ); }
      operator value_type () const
      { return (value_type volatile &)m_value; }

    private:
      typedef value_type counter_type;

#elif defined (__GNUC__)

      value_type operator++()
      { return __sync_add_and_fetch( &m_value, 1); }
      value_type operator--()
      { return __sync_sub_and_fetch( &m_value, 1); }
      operator value_type () const
      { return (value_type volatile &)m_value; }

    private:
      typedef value_type counter_type;
#else

      value_type operator++()
      { Locker l(m_mutex); return ++m_value; }
      value_type operator--()
      { Locker l(m_mutex); return --m_value; }
      operator value_type () const
      { return (value_type volatile &)m_value; }

    private:
      typedef value_type counter_type;
      Mutex m_mutex;
#endif

      AtomicCount( AtomicCount const & );
      AtomicCount & operator=( AtomicCount const & );

      counter_type m_value;
  };

}

#endif

