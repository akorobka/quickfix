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

#include "ThreadedSocketAcceptor.h"
#include "Settings.h"
#include "Utility.h"

namespace FIX
{
COLDSECTION ThreadedSocketAcceptor::ThreadedSocketAcceptor(
  Application& application,
  MessageStoreFactory& factory,
  const SessionSettings& settings ) THROW_DECL( ConfigError )
: Acceptor( application, factory, settings )
{ socket_init(); }

COLDSECTION ThreadedSocketAcceptor::ThreadedSocketAcceptor(
  Application& application,
  MessageStoreFactory& factory,
  const SessionSettings& settings,
  LogFactory& logFactory ) THROW_DECL( ConfigError )
: Acceptor( application, factory, settings, logFactory )
{ 
  socket_init(); 
}

ThreadedSocketAcceptor::~ThreadedSocketAcceptor()
{ 
  socket_term(); 
}

void COLDSECTION ThreadedSocketAcceptor::onConfigure( const SessionSettings& s )
THROW_DECL( ConfigError )
{
  std::set<SessionID> sessions = s.getSessions();
  std::set<SessionID>::iterator i;
  for( i = sessions.begin(); i != sessions.end(); ++i )
  {
    const Dictionary& settings = s.get( *i );
    settings.getInt( SOCKET_ACCEPT_PORT );
    if( settings.has(SOCKET_REUSE_ADDRESS) )
      settings.getBool( SOCKET_REUSE_ADDRESS );
    if( settings.has(SOCKET_NODELAY) )
      settings.getBool( SOCKET_NODELAY );
  }
}

void COLDSECTION ThreadedSocketAcceptor::onInitialize( const SessionSettings& s )
THROW_DECL( RuntimeError )
{
  short port = 0;
  std::set<int> ports;

  std::set<SessionID> sessions = s.getSessions();
  std::set<SessionID>::iterator i = sessions.begin();
  for( ; i != sessions.end(); ++i )
  {
    const Dictionary& settings = s.get( *i );
    port = (short)settings.getInt( SOCKET_ACCEPT_PORT );

    m_portToSessions[port].insert( *i );

    if( ports.find(port) != ports.end() )
      continue;
    ports.insert( port );

    const bool reuseAddress = settings.has( SOCKET_REUSE_ADDRESS ) ? 
      settings.getBool( SOCKET_REUSE_ADDRESS ) : true;

    const bool noDelay = settings.has( SOCKET_NODELAY ) ? 
      settings.getBool( SOCKET_NODELAY ) : false;

    const int sendBufSize = settings.has( SOCKET_SEND_BUFFER_SIZE ) ?
      settings.getInt( SOCKET_SEND_BUFFER_SIZE ) : 0;

    const int rcvBufSize = settings.has( SOCKET_RECEIVE_BUFFER_SIZE ) ?
      settings.getInt( SOCKET_RECEIVE_BUFFER_SIZE ) : 0;

    const size_t affinity = (size_t)-1;
    if (settings.has( THREAD_AFFINITY ))
    {
      if (settings.getString( THREAD_AFFINITY, true ) == "SOCKET")
      {
#ifdef SO_INCOMING_CPU
        int cpu; socklen_t len = sizeof(cpu);
        if ( 0 == ::getsockopt( socket, SOL_SOCKET, SO_INCOMING_CPU, &cpu, &len ) )
        {
          affinity = (size_t)cpu;
        }
#else
        throw ConfigError( "Thread affinity via automatic socket to CPU mapping is not supported" );
#endif
      }
      else
        settings.getInt( THREAD_AFFINITY );
    }

    int socket = socket_createAcceptor( port, reuseAddress );
    if( socket < 0 )
    {
      SocketException e;
      socket_close( socket );
      throw RuntimeError( "Unable to create, bind, or listen to port " 
                         + IntConvertor::convert( (unsigned short)port ) + " (" + e.what() + ")" );
    }
    if( noDelay )
      socket_setsockopt( socket, TCP_NODELAY );
    if( sendBufSize )
      socket_setsockopt( socket, SO_SNDBUF, sendBufSize );
    if( rcvBufSize )
      socket_setsockopt( socket, SO_RCVBUF, rcvBufSize );

    m_socketThreadAttr[socket] = AcceptorThreadInfo::Attr( socket, port, affinity );
    m_sockets.insert( socket );
  }    
}

void COLDSECTION ThreadedSocketAcceptor::onStart()
{
  Sockets::iterator i;
  for( i = m_sockets.begin(); i != m_sockets.end(); ++i )
  {
    Locker l( m_mutex );
    AcceptorThreadInfo* info = new AcceptorThreadInfo( this, m_socketThreadAttr[*i] );
    const Dictionary& global = m_settings.get();
    size_t affinity = (size_t)(global.has( THREAD_AFFINITY ) ? global.getInt( THREAD_AFFINITY ) : -1);
    thread_id thread;
    thread_spawn( &socketAcceptorThread, affinity, info, thread );
    addThread( *i, thread );
  }
}

bool ThreadedSocketAcceptor::onPoll( double timeout )
{
  return false;
}

void ThreadedSocketAcceptor::onStop()
{
  SocketToThread threads;
  SocketToThread::iterator i;

  {
    Locker l(m_mutex);

    time_t start = 0;
    time_t now = 0;

    ::time( &start );
    while ( isLoggedOn() )
    {
      if( ::time(&now) -5 >= start )
        break;
    }

    threads = m_threads;
    m_threads.clear();
  }

  for ( i = threads.begin(); i != threads.end(); ++i )
    socket_close( i->first );
  for ( i = threads.begin(); i != threads.end(); ++i )
    thread_join( i->second );
}

void ThreadedSocketAcceptor::addThread(sys_socket_t s, thread_id t )
{
  Locker l(m_mutex);

  m_threads[ s ] = t;
}

void ThreadedSocketAcceptor::removeThread(sys_socket_t s )
{
  Locker l(m_mutex);
  SocketToThread::iterator i = m_threads.find( s );
  if ( i != m_threads.end() )
  {
    thread_detach( i->second );
    m_threads.erase( i );
  }
}

THREAD_PROC ThreadedSocketAcceptor::socketAcceptorThread( void* p )
{
  AcceptorThreadInfo * info = reinterpret_cast < AcceptorThreadInfo* > ( p );

  ThreadedSocketAcceptor* pAcceptor = info->m_pAcceptor;
  sys_socket_t s = info->m_attr.m_socket;
  int port = info->m_attr.m_port;
  size_t affinity = info->m_attr.m_affinity;
  delete info;

  int noDelay = 0;
  int sendBufSize = 0;
  int rcvBufSize = 0;
  socket_getsockopt( s, TCP_NODELAY, noDelay );
  socket_getsockopt( s, SO_SNDBUF, sendBufSize );
  socket_getsockopt( s, SO_RCVBUF, rcvBufSize );

  sys_socket_t socket = 0;
  while ( ( !pAcceptor->isStopped() && ( socket = socket_accept( s ) ) >= 0 ) )
  {
    if( noDelay )
      socket_setsockopt( socket, TCP_NODELAY );
    if( sendBufSize )
      socket_setsockopt( socket, SO_SNDBUF, sendBufSize );
    if( rcvBufSize )
      socket_setsockopt( socket, SO_RCVBUF, rcvBufSize );

    Sessions sessions = pAcceptor->m_portToSessions[port];

    ThreadedSocketConnection * pConnection =
      new ThreadedSocketConnection
        ( socket, sessions, pAcceptor->getLog() );

    ConnectionThreadInfo* info = new ConnectionThreadInfo( pAcceptor, pConnection );

    {
      Locker l( pAcceptor->m_mutex );

      std::stringstream stream;
      stream << "Accepted connection from " << socket_peername( socket ) << " on port " << port;

      if( pAcceptor->getLog() )
        pAcceptor->getLog()->onEvent( stream.str() );

      thread_id thread;
      if ( !thread_spawn( &socketConnectionThread, affinity, info, thread ) )
      {
        delete info;
        delete pConnection;
      }
      else
        pAcceptor->addThread( socket, thread );
    }
  }

  if( !pAcceptor->isStopped() )
    pAcceptor->removeThread( s );

  return 0;
}

THREAD_PROC HOTSECTION ThreadedSocketAcceptor::socketConnectionThread( void* p )
{
  ConnectionThreadInfo * info = reinterpret_cast < ConnectionThreadInfo* > ( p );

  ThreadedSocketAcceptor* pAcceptor = info->m_pAcceptor;
  ThreadedSocketConnection* pConnection = info->m_pConnection;
  delete info;

  sys_socket_t socket = pConnection->getSocket();

  while ( pConnection->read() ) {}
  delete pConnection;
  if( !pAcceptor->isStopped() )
    pAcceptor->removeThread( socket );
  return 0;
}
}
