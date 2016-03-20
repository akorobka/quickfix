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

#include "FieldMap.h"
#include <algorithm>
#include <iterator>

namespace FIX
{
FieldMap::~FieldMap()
{
  f_clear();
  g_clear();
}

FieldMap& FieldMap::operator=( const FieldMap& rhs )
{
  f_clone( rhs );
  g_clear();
  g_copy( rhs );
  return *this;
}

FieldMap& FieldMap::addGroup( int field, const FieldMap& group, bool setCount )
{
  FieldMap * pGroup = new FieldMap( group );
  addGroupPtr( field, pGroup, setCount );
  return *pGroup;
}

bool FieldMap::replaceGroup( int num, int field, const FieldMap& group )
{
  Groups::const_iterator i = m_groups.find( field );
  if ( i == m_groups.end() ) return false;
  if ( num <= 0 ) return false;
  if ( i->second.size() < ( unsigned ) num ) return false;
  *( *( i->second.begin() + ( num - 1 ) ) ) = group;
  return true;
}

void FieldMap::removeGroup( int num, int field )
{
  Groups::iterator i = m_groups.find( field );
  if ( i == m_groups.end() ) return;
  if ( num <= 0 ) return;
  GroupItem& vector = i->second;
  if ( vector.size() < ( unsigned ) num ) return;

  GroupItem::iterator iter = vector.begin();
  std::advance( iter, ( num - 1 ) );
 
  delete (*iter);
  vector.erase( iter );

  if( vector.size() == 0 )
  {
    m_groups.erase( field );
    removeField( field );
  }
  else
  {
    IntField groupCount( field, (int)vector.size() );
    setField( groupCount, true );
  }
}

void FieldMap::removeGroup( int field )
{
  removeGroup( (int)groupCount(field), field );
}

bool FieldMap::hasGroup( int num, int field ) const
{
  return (int)groupCount(field) >= num;
}

bool FieldMap::hasGroup( int field ) const
{
  Groups::const_iterator i = m_groups.find( field );
  return i != m_groups.end();
}

size_t FieldMap::groupCount( int field ) const
{
  Groups::const_iterator i = m_groups.find( field );
  if( i == m_groups.end() )
    return 0;
  return i->second.size();
}

size_t FieldMap::totalFields() const
{
  size_t result = m_fields.size();
    
  Groups::const_iterator i;
  for ( i = m_groups.begin(); i != m_groups.end(); ++i )
  {
    GroupItem::const_iterator j, jend = i->second.end();
    for ( j = i->second.begin(); j != jend; ++j )
      result += ( *j ) ->totalFields();
  }
  return result;
}

std::string& FieldMap::calculateString( std::string& result, bool clear ) const
{

#if defined(_MSC_VER) && _MSC_VER < 1300
  if( clear ) result = "";
#else
  if( clear ) result.clear();
#endif

  if( result.empty() )
    result.reserve( totalFields() * 32 );

  return serializeTo( result );
}

int FieldMap::calculateLength( int beginStringField,
                               int bodyLengthField,
                               int checkSumField ) const
{
  int result = 0;
  Fields::const_iterator i, fe = f_end();
  for ( i = f_begin(); i != fe; ++i )
  {
    int tag = i->first;
    if ( tag != beginStringField
         && tag != bodyLengthField
         && tag != checkSumField )
    { result += i->second.getLength(); }
  }

  Groups::const_iterator j, ge = m_groups.end();
  for ( j = m_groups.begin(); j != ge; ++j )
  {
    GroupItem::const_iterator k, ke = j->second.end();
    for ( k = j->second.begin(); k != ke; ++k )
      result += ( *k ) ->calculateLength();
  }
  return result;
}

int HEAVYUSE FieldMap::calculateTotal( int checkSumField ) const
{
  int result = 0;
  Fields::const_iterator i, fe = f_end();
  for ( i = f_begin(); i != fe; ++i )
  {
    if ( i->first != checkSumField )
      result += i->second.getTotal();
  }

  Groups::const_iterator j, ge = m_groups.end();
  for ( j = m_groups.begin(); j != ge; ++j )
  {
    GroupItem::const_iterator k, ke = j->second.end();
    for ( k = j->second.begin(); k != ke; ++k ) {
      result += ( *k ) ->calculateTotal();
    }
  }
  return result;
}

void FieldMap::addGroupPtr( int field, FieldMap * group, bool setCount )
{
  GroupItem& vec = m_groups[ field ];
  vec.push_back( group );

  if( setCount )
      setField( IntField::Pack( field, vec.size() ) );
}

}
