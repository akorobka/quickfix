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
#define _XOPEN_SOURCE 500
#define _LARGEFILE64_SOURCE
#include "config.h"
#endif

#include "FileStore.h"
#include "SessionID.h"
#include "Parser.h"
#include "Utility.h"
#include <fstream>
#include <stdio.h>

namespace FIX
{
FileStore::FileStore( std::string path, const SessionID& s )
: m_msgFileHandle( INVALID_FILE_HANDLE_VALUE ), m_headerFile( 0 ),
  m_seqNumsFile( 0 ), m_sessionFile( 0 )
{
  file_mkdir( path.c_str() );

  if ( path.empty() ) path = ".";
  const std::string& begin =
    s.getBeginString().dupString();
  const std::string& sender =
    s.getSenderCompID().dupString();
  const std::string& target =
    s.getTargetCompID().dupString();
  const std::string& qualifier =
    s.getSessionQualifier();

  std::string sessionid = begin + "-" + sender + "-" + target;
  if( qualifier.size() )
    sessionid += "-" + qualifier;

  std::string prefix
    = file_appendpath(path, sessionid + ".");

  m_msgFileName = prefix + "body";
  m_headerFileName = prefix + "header";
  m_seqNumsFileName = prefix + "seqnums";
  m_sessionFileName = prefix + "session";

  try
  {
    open( false );
  }
  catch ( IOException & e )
  {
    throw ConfigError( e.what() );
  }
}

FileStore::~FileStore()
{
  if( m_msgFileHandle != INVALID_FILE_HANDLE_VALUE)
    file_handle_close( m_msgFileHandle );

  if( m_headerFile ) ::fclose( m_headerFile );
  if( m_seqNumsFile ) ::fclose( m_seqNumsFile );
  if( m_sessionFile ) ::fclose( m_sessionFile );
}

void FileStore::open( bool deleteFile )
{
  if ( m_msgFileHandle != INVALID_FILE_HANDLE_VALUE )
    file_handle_close( m_msgFileHandle );

  if ( m_headerFile ) ::fclose( m_headerFile );
  if ( m_seqNumsFile ) ::fclose( m_seqNumsFile );
  if ( m_sessionFile ) ::fclose( m_sessionFile );

  m_msgFileHandle = INVALID_FILE_HANDLE_VALUE;
  m_headerFile = 0;
  m_seqNumsFile = 0;
  m_sessionFile = 0;

  if ( deleteFile )
  {
    file_unlink( m_msgFileName.c_str() );
    file_unlink( m_headerFileName.c_str() );
    file_unlink( m_seqNumsFileName.c_str() );
    file_unlink( m_sessionFileName.c_str() );
  }

  populateCache();
  m_msgFileHandle = file_handle_open( m_msgFileName.c_str() );
  if ( m_msgFileHandle == INVALID_FILE_HANDLE_VALUE )
    throw ConfigError( "Could not open body file: " + m_msgFileName );
  FILE_OFFSET_TYPE offset;
  FILE_OFFSET_TYPE_SET(offset, 0);
  file_handle_seek( m_msgFileHandle, offset, FILE_POSITION_END );

  m_headerFile = file_fopen( m_headerFileName.c_str(), "r+" );
  if ( !m_headerFile ) m_headerFile = file_fopen( m_headerFileName.c_str(), "w+" );
  if ( !m_headerFile ) throw ConfigError( "Could not open header file: " + m_headerFileName );

  m_seqNumsFile = file_fopen( m_seqNumsFileName.c_str(), "r+" );
  if ( !m_seqNumsFile ) m_seqNumsFile = file_fopen( m_seqNumsFileName.c_str(), "w+" );
  if ( !m_seqNumsFile ) throw ConfigError( "Could not open seqnums file: " + m_seqNumsFileName );

  bool setCreationTime = false;
  m_sessionFile = file_fopen( m_sessionFileName.c_str(), "r" );
  if ( !m_sessionFile ) setCreationTime = true;
  else fclose( m_sessionFile );

  m_sessionFile = file_fopen( m_sessionFileName.c_str(), "r+" );
  if ( !m_sessionFile ) m_sessionFile = file_fopen( m_sessionFileName.c_str(), "w+" );
  if ( !m_sessionFile ) throw ConfigError( "Could not open session file" );
  if ( setCreationTime ) setSession();

  setNextSenderMsgSeqNum( getNextSenderMsgSeqNum() );
  setNextTargetMsgSeqNum( getNextTargetMsgSeqNum() );
}

void FileStore::populateCache()
{
  FILE* headerFile;
  headerFile = file_fopen( m_headerFileName.c_str(), "r+" );
  if ( headerFile )
  {
    int num;
    FILE_OFFSET_TYPE offset;
	size_t size;
    while ( FILE_FSCANF( headerFile, "%d,%" FILE_OFFSET_TYPE_MOD "d,%d ",
                        &num, FILE_OFFSET_TYPE_ADDR(offset), &size ) == 3 )
      m_offsets[ num ] = std::make_pair( offset, size );
    fclose( headerFile );
  }

  FILE* seqNumsFile = file_fopen( m_seqNumsFileName.c_str(), "r+" );
  if ( seqNumsFile )
  {
    int sender, target;
    if ( FILE_FSCANF( seqNumsFile, "%d : %d", &sender, &target ) == 2 )
    {
      m_cache.setNextSenderMsgSeqNum( sender );
      m_cache.setNextTargetMsgSeqNum( target );
    }
    fclose( seqNumsFile );
  }

  FILE* sessionFile = file_fopen( m_sessionFileName.c_str(), "r+" );
  if ( sessionFile )
  {
    char time[ 22 ];
#ifdef HAVE_FSCANF_S
    int result = FILE_FSCANF( sessionFile, "%s", time, 22 );
#else
    int result = FILE_FSCANF( sessionFile, "%s", time );
#endif
    if( result == 1 )
    {
      UtcTimeStamp uts = UtcTimeStampConvertor::convert( time, true );
      setCreationTime( m_cache.setCreationTime( uts ) );
    }
    else
      setCreationTime( m_cache.getCreationTime() );
    fclose( sessionFile );
  }
  else
    setCreationTime( m_cache.getCreationTime() );
}

MessageStore* FileStoreFactory::create( const SessionID& s )
{
  if ( m_path.size() ) return new FileStore( m_path, s );

  std::string path;
  Dictionary settings = m_settings.get( s );
  path = settings.getString( FILE_STORE_PATH );
  return new FileStore( path, s );
}

void FileStoreFactory::destroy( MessageStore* pStore )
{
  delete pStore;
}

bool FileStore::set( int msgSeqNum, const std::string& msg )
throw ( IOException )
{
  if ( fseek( m_headerFile, 0, SEEK_END ) ) 
    throw IOException( "Cannot seek to end of " + m_headerFileName );
  FILE_OFFSET_TYPE offset;
  FILE_OFFSET_TYPE_SET(offset, 0);
  offset = file_handle_seek( m_msgFileHandle, offset, FILE_POSITION_END );
  if ( FILE_OFFSET_TYPE_VALUE(offset) < 0 ) 
    throw IOException( "Unable to get file pointer position from " +
                       m_msgFileName );
  int size = msg.size();

  if ( fprintf( m_headerFile, "%d,%" FILE_OFFSET_TYPE_MOD "d,%d ",
                msgSeqNum, FILE_OFFSET_TYPE_VALUE(offset), size ) < 0 )
    throw IOException( "Unable to write to file " + m_headerFileName );
  m_offsets[ msgSeqNum ] = std::make_pair( offset, size );
  if ( size != file_handle_write( m_msgFileHandle, msg.c_str(), size ) )
    throw IOException( "Unable to write to file " + m_msgFileName );
  if ( fflush( m_headerFile ) == EOF ) 
    throw IOException( "Unable to flush file " + m_headerFileName );
  return true;
}

bool FileStore::set( int msgSeqNum, Sg::sg_buf_ptr b, int n )
throw ( IOException )
{
  if ( fseek( m_headerFile, 0, SEEK_END ) ) 
    throw IOException( "Cannot seek to end of " + m_headerFileName );
  FILE_OFFSET_TYPE offset;
  FILE_OFFSET_TYPE_SET(offset, 0);
  offset = file_handle_seek( m_msgFileHandle, offset, FILE_POSITION_END );
  if ( FILE_OFFSET_TYPE_VALUE(offset) < 0 ) 
    throw IOException( "Unable to get file pointer position from " + m_msgFileName );
  int size = Sg::size(b, n);

  if ( fprintf( m_headerFile, "%d,%" FILE_OFFSET_TYPE_MOD "d,%d ",
                msgSeqNum, FILE_OFFSET_TYPE_VALUE(offset), size ) < 0 )
    throw IOException( "Unable to write to file " + m_headerFileName );
  m_offsets[ msgSeqNum ] = std::make_pair( offset, size );

  if ( Sg::writev( m_msgFileHandle, b, n ) < size )
    throw IOException( "Unable to write to file " + m_msgFileName );

  if ( fflush( m_headerFile ) == EOF ) 
    throw IOException( "Unable to flush file " + m_headerFileName );
  return true;
}

void FileStore::get( int begin, int end,
                     std::vector < std::string > & result ) const
throw ( IOException )
{
  result.clear();
  for ( int i = begin; i <= end; ++i )
  {
    std::string msg;
    if ( get( i, msg ) ) {
      result.push_back( std::string() );
      result.back().swap(msg);
    }
  }
}

int FileStore::getNextSenderMsgSeqNum() const throw ( IOException )
{
  return m_cache.getNextSenderMsgSeqNum();
}

int FileStore::getNextTargetMsgSeqNum() const throw ( IOException )
{
  return m_cache.getNextTargetMsgSeqNum();
}

void FileStore::setNextSenderMsgSeqNum( int value ) throw ( IOException )
{
  m_cache.setNextSenderMsgSeqNum( value );
  setSeqNum();
}

void FileStore::setNextTargetMsgSeqNum( int value ) throw ( IOException )
{
  m_cache.setNextTargetMsgSeqNum( value );
  setSeqNum();
}

void FileStore::incrNextSenderMsgSeqNum() throw ( IOException )
{
  m_cache.incrNextSenderMsgSeqNum();
  setSeqNum();
}

void FileStore::incrNextTargetMsgSeqNum() throw ( IOException )
{
  m_cache.incrNextTargetMsgSeqNum();
  setSeqNum();
}

void FileStore::reset() throw ( IOException )
{
  m_cache.reset();
  open( true );
  setSession();
}

void FileStore::refresh() throw ( IOException )
{
  m_cache.reset();
  open( false );
}

void FileStore::setSeqNum()
{
  rewind( m_seqNumsFile );
  fprintf( m_seqNumsFile, "%10.10d : %10.10d",
           getNextSenderMsgSeqNum(), getNextTargetMsgSeqNum() );
  if ( ferror( m_seqNumsFile ) ) 
    throw IOException( "Unable to write to file " + m_seqNumsFileName );
  if ( fflush( m_seqNumsFile ) ) 
    throw IOException( "Unable to flush file " + m_seqNumsFileName );
}

void FileStore::setSession()
{
  rewind( m_sessionFile );
  fprintf( m_sessionFile, "%s",
           UtcTimeStampConvertor::convert( m_cache.getCreationTime() ).c_str() );
  if ( ferror( m_sessionFile ) ) 
    throw IOException( "Unable to write to file " + m_sessionFileName );
  if ( fflush( m_sessionFile ) ) 
    throw IOException( "Unable to flush file " + m_sessionFileName );
}

bool FileStore::get( int msgSeqNum, std::string& msg ) const
throw ( IOException )
{
  NumToOffset::const_iterator find = m_offsets.find( msgSeqNum );
  if ( find == m_offsets.end() ) return false;
  const OffsetSize& offset = find->second;
  msg.resize(offset.second);
  if ( file_handle_read_at( m_msgFileHandle, const_cast<char*>(msg.c_str()),
                            offset.second, offset.first) < offset.second )
    throw IOException( "Unable to read from file " + m_msgFileName );
  return true;
}

} //namespace FIX
