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

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#include "MessageSorters.h"

namespace FIX
{
message_order::message_order( int first, ... )
: m_mode( group ), m_largest( 0 )
{
  int field = first;
  int size = 0;
  m_largest = first;

  va_list arguments;
  va_start( arguments, first );
  while( field != 0 )
  {
    m_largest = m_largest > field ? m_largest : field;
      
    size++;
    field = va_arg( arguments, int );
  }

  if ( size )
  {
    m_groupOrder = group_order_array::create(m_largest);
    int *p = m_groupOrder;

    va_start( arguments, first );
    field = first;
    int i = 0;
    while( field != 0 )
    {
      p[ field ] = i++ - size; // < 0 for ordered fields
      field = va_arg( arguments, int );
    }
  }
  else
  {
    m_largest = 0;
  }

  va_end( arguments );
}

message_order::message_order( const int order[] )
: m_mode( group ), m_largest( 0 )
{
  int size = 0;
  while( order[size] != 0 ) { ++size; }
  if ( size )
  {
    m_largest = order[0];

    // collect all fields and find the largest field number
    for (int i = 1; i < size; ++i )
    {
      int field = order[i];
      m_largest = m_largest > field ? m_largest : field;
    }

    m_groupOrder = group_order_array::create(m_largest);
    int* p = m_groupOrder;

    for (int i = 0; i < size; ++i )
      p[ order[ i ] ] = i - size; // < 0 for ordered fields
  }
}

} // namespace FIX

