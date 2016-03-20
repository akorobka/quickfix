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

#include "SocketConnection.h"
#include "SocketAcceptor.h"
#include "SocketConnector.h"
#include "SocketInitiator.h"
#include "Session.h"
#include "Utility.h"

namespace FIX
{
SocketConnection::SocketConnection( int s, Sessions sessions,
                                    SocketMonitor* pMonitor )
: m_socket( s ), m_sendLength( 0 ),
  m_sessions(sessions), m_pSession( 0 ), m_pMonitor( pMonitor )
{
  FD_ZERO( &m_fds );
  FD_SET( m_socket, &m_fds );
}

SocketConnection::SocketConnection( SocketInitiator& i,
                                    const SessionID& sessionID, int s,
                                    SocketMonitor* pMonitor )
: m_socket( s ), m_sendLength( 0 ),
  m_pSession( i.getSession( sessionID, *this ) ),
  m_pMonitor( pMonitor ) 
{
  FD_ZERO( &m_fds );
  FD_SET( m_socket, &m_fds );
  m_sessions.insert( sessionID );
}

SocketConnection::~SocketConnection()
{
  if ( m_pSession )
    Session::unregisterSession( m_pSession->getSessionID() );
}

bool SocketConnection::send( const std::string& msg )
{
  Locker l( m_mutex );

  m_sendQueue.push_back( msg );
  processQueue();
  signal();
  return true;
}

bool SocketConnection::send( Sg::sg_buf_ptr bufs, int n )
{
  {
    Locker l( m_mutex );

    if( !m_sendQueue.size() )
    {
      struct timeval timeout = { 0, 0 };
      fd_set writeset = m_fds;
      if( select( 1 + m_socket, 0, &writeset, 0, &timeout ) > 0 )
      {
        std::size_t sent = Sg::send(m_socket, bufs, n);
        for (int i = 0; i < n; i++ )
        {
          std::size_t l = IOV_LEN(bufs[i]);
          if( l > sent )
          {
            std::string s( (const char*)IOV_BUF(bufs[i]) + sent, l - sent);
            while( ++i < n )
              String::append( s, bufs[i] );
            return send( s );
          }
          sent -= l;
        }
        return true;
      }
    }
  } 
  return send( Sg::toString( bufs, n ) );
}

bool SocketConnection::processQueue()
{
  Locker l( m_mutex );

  if( !m_sendQueue.size() ) return true;

  struct timeval timeout = { 0, 0 };
  fd_set writeset = m_fds;
  if( select( 1 + m_socket, 0, &writeset, 0, &timeout ) <= 0 )
    return false;
    
  const std::string& msg = m_sendQueue.front();

  size_t result = socket_send
    ( m_socket, String::c_str(msg) + m_sendLength, String::length(msg) - m_sendLength );

  if( result > 0 )
  {
    m_sendLength += result;
  }

  if( m_sendLength == String::length(msg) )
  {
    m_sendLength = 0;
    m_sendQueue.pop_front();
  }

  return !m_sendQueue.size();
}

void SocketConnection::disconnect()
{
  if ( m_pMonitor )
    m_pMonitor->drop( m_socket );
}

bool SocketConnection::read( SocketConnector& s )
{
  if ( !m_pSession ) return false;

  try
  {
    readFromSocket();
    readMessages( s.getMonitor() );
  }
  catch( SocketRecvFailed& e )
  {
    m_pSession->getLog()->onEvent( e.what() );
    return false;
  }
  return true;
}

bool SocketConnection::read( SocketAcceptor& a, SocketServer& s )
{
  try
  {
    readFromSocket();

    if ( !m_pSession )
    {
      Sg::sg_buf_t buf;
      if ( !readMessage( buf ) ) return false;

      m_pSession = Session::lookupSession( buf, true );
      if( !isValidSession() )
      {
        m_pSession = 0;
        if( a.getLog() )
        {
          a.getLog()->onEvent( "Session not found for incoming message: " + Sg::toString(buf) );
          a.getLog()->onIncoming( &buf, 1 );
        }
      }
      if( m_pSession )
        m_pSession = a.getSession( buf, *this );
      if( m_pSession )
        m_pSession->next( buf, UtcTimeStamp() );
      if( !m_pSession )
      {
        s.getMonitor().drop( m_socket );
        return false;
      }

      Session::registerSession( m_pSession->getSessionID() );
    }

    readMessages( s.getMonitor() );
    return true;
  }
  catch ( SocketRecvFailed& e )
  {
    if( m_pSession )
      m_pSession->getLog()->onEvent( e.what() );
    s.getMonitor().drop( m_socket );
  }
  catch ( InvalidMessage& )
  {
    s.getMonitor().drop( m_socket );
  }
  return false;
}

bool SocketConnection::isValidSession()
{
  if( m_pSession == 0 )
    return false;
  SessionID sessionID = m_pSession->getSessionID();
  if( Session::isSessionRegistered(sessionID) )
    return false;
  return !( m_sessions.find(sessionID) == m_sessions.end() );
}

void SocketConnection::readFromSocket()
throw( SocketRecvFailed )
{
  Sg::sg_buf_t buf = m_parser.buffer();
  ssize_t size = recv( m_socket, IOV_BUF(buf), IOV_LEN(buf), 0 );
  if( size <= 0 ) throw SocketRecvFailed( size );
  m_parser.advance( size );
}

bool SocketConnection::readMessage( Sg::sg_buf_t& msg )
{
  while( true )
  {
    try
    {
      if( m_parser.parse() )
      {
        m_parser.retrieve( msg );
        return true;
      }
      break;
    }
    catch ( MessageParseError& e ) {
      if( m_pSession )
        m_pSession->getLog()->onEvent( e.what() );
    }
  }
  return false;
}

void SocketConnection::readMessages( SocketMonitor& s )
{
  if( !m_pSession ) return;

  Sg::sg_buf_t msg;
  while( readMessage( msg ) )
  {
    try
    {
      m_pSession->next( msg, UtcTimeStamp() );
    }
    catch ( InvalidMessage& )
    {
      if( !m_pSession->isLoggedOn() )
        s.drop( m_socket );
    }
  }
}

void SocketConnection::onTimeout()
{
  if ( m_pSession ) m_pSession->next();
}
} // namespace FIX
