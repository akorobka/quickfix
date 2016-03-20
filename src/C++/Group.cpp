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

#include "Group.h"

namespace FIX
{
FieldMap& Group::addGroup( const FIX::Group& group )
{
  return FieldMap::addGroup( group.field(), group );
}

bool Group::replaceGroup( unsigned num, const FIX::Group& group )
{
  return FieldMap::replaceGroup( num, group.field(), group ); 
}

Group& Group::getGroup( unsigned num, FIX::Group& group ) const throw( FieldNotFound )
{
  return static_cast < Group& > ( FieldMap::getGroup( num, group.field(), group ) );
}

void Group::removeGroup( unsigned num, const FIX::Group& group )
{
  FieldMap::removeGroup( num, group.field() );
}

void Group::removeGroup( const FIX::Group& group )
{
  FieldMap::removeGroup( group.field() );
}

bool Group::hasGroup( unsigned num, const FIX::Group& group )
{
  return FieldMap::hasGroup( num, group.field() );
}

bool Group::hasGroup( const FIX::Group& group )
{
  return FieldMap::hasGroup( group.field() );
}

}
